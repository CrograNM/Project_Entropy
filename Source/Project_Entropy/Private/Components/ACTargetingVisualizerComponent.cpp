// Copyright CrograNM

#include "Components/ACTargetingVisualizerComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/SplineComponent.h" 
#include "Components/SplineMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillEffectModule.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h" 

UACTargetingVisualizerComponent::UACTargetingVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 스플라인 컴포넌트 부착
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
	PushSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PushSpline"));
}

void UACTargetingVisualizerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepTargetingMode);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepRange);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepSkillData);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepHoveredTile);
}

void UACTargetingVisualizerComponent::BeginPlay() { Super::BeginPlay(); }

void UACTargetingVisualizerComponent::SetTargetingMode(ETargetingMode NewMode, int32 InRange, const UPE_SkillData* InSkillData)
{
	RepTargetingMode = NewMode;
	RepRange = InRange;
	RepSkillData = InSkillData;
	RefreshVisuals();

	if (!GetOwner()->HasAuthority()) Server_SetTargetingState(NewMode, InRange, InSkillData);
}

void UACTargetingVisualizerComponent::Server_SetTargetingState_Implementation(ETargetingMode NewMode, int32 InRange, const UPE_SkillData* InSkillData)
{
	RepTargetingMode = NewMode;
	RepRange = InRange;
	RepSkillData = InSkillData;
	RefreshVisuals();
}

void UACTargetingVisualizerComponent::UpdateHoveredTile(FIntPoint NewPos)
{
	if (RepHoveredTile != NewPos)
	{
		RepHoveredTile = NewPos;
		RefreshVisuals();

		if (!GetOwner()->HasAuthority()) Server_UpdateHoveredTile(NewPos);
	}
}

void UACTargetingVisualizerComponent::Server_UpdateHoveredTile_Implementation(FIntPoint NewPos)
{
	RepHoveredTile = NewPos;
	RefreshVisuals();
}

void UACTargetingVisualizerComponent::ClearTargeting()
{
	SetTargetingMode(ETargetingMode::None, 0, nullptr);
	UpdateHoveredTile(FIntPoint(-999, -999));
}

bool UACTargetingVisualizerComponent::IsTileInRange(AACTile* TargetTile) const
{
	return TargetTile && CurrentValidTiles.Contains(TargetTile);
}

void UACTargetingVisualizerComponent::OnRep_TargetingState() { RefreshVisuals(); }
void UACTargetingVisualizerComponent::OnRep_HoveredTile() { RefreshVisuals(); }

void UACTargetingVisualizerComponent::ClearGeneratedMeshes()
{
	// 이전 프레임에 만들어둔 화살표 메쉬들을 청소
	for (UMeshComponent* Mesh : GeneratedMeshes)
	{
		if (Mesh) Mesh->DestroyComponent();
	}
	GeneratedMeshes.Empty();
}

void UACTargetingVisualizerComponent::RefreshVisuals()
{
	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
	UACGridMovementComponent* MoveComp = GetOwner()->FindComponentByClass<UACGridMovementComponent>();
	if (!GridSystem || !MoveComp) return;

	AActor* OwnerActor = GetOwner();
	FIntPoint CenterPos = MoveComp->GetGridPosition();

	GridSystem->ClearAllHighlightsFor(OwnerActor);
	CurrentValidTiles.Empty();
	TrajectorySpline->ClearSplinePoints();
	PushSpline->ClearSplinePoints();
	ClearGeneratedMeshes(); // 메쉬 청소

	// 1. 사거리 표시
	if (RepTargetingMode != ETargetingMode::None && RepRange > 0)
	{
		bool bIsMovement = (RepTargetingMode == ETargetingMode::Movement);
		CurrentValidTiles = GridSystem->HighlightArea(OwnerActor, CenterPos, RepRange, bIsMovement);
	}

	// 2. 조준 및 궤적 계산
	if (RepHoveredTile != FIntPoint(-999, -999) && RepTargetingMode != ETargetingMode::None)
	{
		if (RepTargetingMode == ETargetingMode::Movement)
		{
			GridSystem->HighlightPath(OwnerActor, CenterPos, RepHoveredTile, CurrentValidTiles);
		}
		else if (RepTargetingMode == ETargetingMode::Skill && RepSkillData)
		{
			FIntPoint ActualTargetPos = RepHoveredTile;

			FVector StartLoc = OwnerActor->GetActorLocation();
			if (UCapsuleComponent* Cap = OwnerActor->FindComponentByClass<UCapsuleComponent>())
				StartLoc.Z = Cap->GetScaledCapsuleHalfHeight() * 0.7f;
			StartLoc += OwnerActor->GetActorForwardVector() * 70.f;

			FVector EndLoc = FVector::ZeroVector;
			if (APE_CharacterBase* TargetChar = GridSystem->GetCharacterAtPosition(RepHoveredTile))
			{
				EndLoc = TargetChar->GetActorLocation();
				if (UCapsuleComponent* TargetCap = TargetChar->FindComponentByClass<UCapsuleComponent>())
					EndLoc.Z = TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
			}
			else if (AACTile* HoveredTileActor = GridSystem->GetTileAtPosition(RepHoveredTile))
			{
				EndLoc = HoveredTileActor->GetActorLocation();
			}

			// 직사 차단 판정
			if (RepSkillData->ProjectileSpeed > 0.f && RepSkillData->ProjectileGravity == 0.f)
			{
				FHitResult HitResult;
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(OwnerActor);

				if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
				{
					EndLoc = HitResult.Location;
					if (APE_CharacterBase* HitChar = Cast<APE_CharacterBase>(HitResult.GetActor()))
					{
						if (UACGridMovementComponent* HitMove = HitChar->GetGridMovementComponent())
							ActualTargetPos = HitMove->GetGridPosition();
					}
					else if (AACTile* HitTile = Cast<AACTile>(HitResult.GetActor()))
					{
						ActualTargetPos = HitTile->GetGridPosition();
					}
				}
			}

			// 스킬 궤적 포인트 생성
			if (RepSkillData->ProjectileGravity > 0.f)
			{
				int32 NumPoints = 15;
				for (int32 i = 0; i <= NumPoints; ++i)
				{
					float Alpha = (float)i / (float)NumPoints;
					FVector LerpPos = FMath::Lerp(StartLoc, EndLoc, Alpha);
					LerpPos.Z += FMath::Sin(Alpha * PI) * RepSkillData->ProjectileGravity;
					TrajectorySpline->AddSplinePoint(LerpPos, ESplineCoordinateSpace::World, false);
				}
				TrajectorySpline->UpdateSpline();
			}
			else
			{
				TrajectorySpline->AddSplinePoint(StartLoc, ESplineCoordinateSpace::World, false);
				TrajectorySpline->AddSplinePoint(EndLoc, ESplineCoordinateSpace::World, true);
			}

			// 타격 타일 하이라이트
			if (RepSkillData->AoEShape != EPEAoEShape::None)
			{
				TSet<FIntPoint> AoE = RepSkillData->GetAffectedGridPositions(ActualTargetPos);
				GridSystem->HighlightAoE(OwnerActor, AoE);
			}
			else
			{
				GridSystem->HighlightTarget(OwnerActor, ActualTargetPos);
			}

			// 밀치기 포인트 생성
			const UPE_SkillEffect_Push* PushModule = nullptr;
			for (const UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
			{
				if (const UPE_SkillEffect_Push* FoundPush = Cast<UPE_SkillEffect_Push>(Module))
				{
					PushModule = FoundPush;
					break;
				}
			}

			if (PushModule && PushModule->GetPushDistance() > 0)
			{
				int32 PushDist = PushModule->GetPushDistance();
				if (APE_CharacterBase* HitChar = GridSystem->GetCharacterAtPosition(ActualTargetPos))
				{
					if (HitChar->IsPushable())
					{
						FIntPoint InstPos = MoveComp->GetGridPosition();
						FIntPoint PushDir(
							FMath::Clamp(ActualTargetPos.X - InstPos.X, -1, 1),
							FMath::Clamp(ActualTargetPos.Y - InstPos.Y, -1, 1)
						);
						if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

						FIntPoint ExpectedEndPos = ActualTargetPos;
						for (int32 step = 1; step <= PushDist; ++step)
						{
							FIntPoint CheckPos = ExpectedEndPos + PushDir;
							AACTile* CheckTile = GridSystem->GetTileAtPosition(CheckPos);
							if (!CheckTile || CheckTile->IsObstacle() || GridSystem->IsTileOccupied(CheckPos, HitChar)) break;
							ExpectedEndPos = CheckPos;
						}

						if (ExpectedEndPos != ActualTargetPos)
						{
							FVector PushStart = HitChar->GetActorLocation();
							FVector PushEnd = GridSystem->GetTileAtPosition(ExpectedEndPos)->GetActorLocation();

							if (UCapsuleComponent* Cap = HitChar->FindComponentByClass<UCapsuleComponent>())
								PushStart.Z = Cap->GetScaledCapsuleHalfHeight() * 0.5f;
							PushEnd.Z = PushStart.Z;

							// 타일 경계선까지 연장 연산을 여기서 미리 처리
							FVector Dir = (PushEnd - PushStart).GetSafeNormal();
							PushEnd += (Dir * PushArrowExtension);

							PushSpline->AddSplinePoint(PushStart, ESplineCoordinateSpace::World, false);
							PushSpline->AddSplinePoint(PushEnd, ESplineCoordinateSpace::World, true);
						}
					}
				}
			}

			// --- 스플라인이 완성되었으므로, 그 길을 따라 실제 메쉬를 소환 ---
			if (TrajectorySpline->GetNumberOfSplinePoints() > 1)
			{
				GenerateMeshesAlongSpline(TrajectorySpline, TrajectoryMaterial, TrajectoryThickness, TrajectoryArrowSize);
			}
			if (PushSpline->GetNumberOfSplinePoints() > 1)
			{
				GenerateMeshesAlongSpline(PushSpline, PushMaterial, PushThickness, PushArrowSize);
			}
		}
	}
}

void UACTargetingVisualizerComponent::GenerateMeshesAlongSpline(USplineComponent* Spline, UMaterialInterface* Mat, float Thickness, float HeadSize)
{
	// 스플라인 정보를 읽어와 선(Cylinder)과 화살촉(Cone) 메쉬를 조립합니다.
	int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints < 2 || !LineMesh || !ArrowHeadMesh || !Mat) return;

	for (int32 i = 0; i < NumPoints - 1; ++i)
	{
		// 스플라인 메쉬는 Local Space 연산이 가장 안정적입니다.
		FVector P1 = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
		FVector T1 = Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
		FVector P2 = Spline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);
		FVector T2 = Spline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local);

		bool bIsLastPoint = (i == NumPoints - 2);
		FVector EndPos = P2;

		// 마지막 구간이면 화살촉이 달릴 자리를 마련하기 위해 선의 끝(P2)을 살짝 뒤로 당깁니다.
		if (bIsLastPoint)
		{
			FVector Dir = (P2 - P1).GetSafeNormal();
			float Dist = FVector::Distance(P1, P2);
			float PullbackDist = FMath::Min(HeadSize * 50.f, Dist * 0.5f); // 메쉬 기본 크기 50 보정
			EndPos = P2 - (Dir * PullbackDist);
			T2 = Dir * T2.Size(); // 곡률 탄젠트 보정

			// --- 화살표 머리(Cone) 생성 ---
			UStaticMeshComponent* ArrowHead = NewObject<UStaticMeshComponent>(GetOwner());
			ArrowHead->SetMobility(EComponentMobility::Movable);
			ArrowHead->SetStaticMesh(ArrowHeadMesh);
			ArrowHead->SetMaterial(0, Mat);
			ArrowHead->SetupAttachment(Spline);
			ArrowHead->SetRelativeLocation(EndPos);

			// Cone 메쉬는 기본적으로 Z축(위)을 향하므로, 진행 방향(Dir)을 향하도록 회전시킵니다.
			FQuat HeadQuat = FQuat::FindBetweenNormals(FVector::UpVector, Dir);
			ArrowHead->SetRelativeRotation(HeadQuat);
			ArrowHead->SetRelativeScale3D(FVector(HeadSize));

			ArrowHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ArrowHead->RegisterComponent();
			GeneratedMeshes.Add(ArrowHead);
		}

		// --- 선(Cylinder) 생성 ---
		USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(GetOwner());
		SplineMesh->SetMobility(EComponentMobility::Movable);
		SplineMesh->SetStaticMesh(LineMesh);
		SplineMesh->SetupAttachment(Spline);

		SplineMesh->SetForwardAxis(ESplineMeshAxis::Z); // Cylinder 메쉬의 축이 Z이므로 설정
		SplineMesh->SetStartScale(FVector2D(Thickness));
		SplineMesh->SetEndScale(FVector2D(Thickness));
		SplineMesh->SetStartAndEnd(P1, T1, EndPos, T2);

		SplineMesh->SetMaterial(0, Mat);

		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->RegisterComponent();
		GeneratedMeshes.Add(SplineMesh);
	}
}