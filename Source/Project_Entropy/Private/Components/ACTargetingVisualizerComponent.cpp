// Copyright CrograNM

#include "Components/ACTargetingVisualizerComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/SplineComponent.h" 
#include "Components/SplineMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ACStatComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillEffectModule.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h" 
#include "Containers/Queue.h"

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
				StartLoc.Z += Cap->GetScaledCapsuleHalfHeight() * 0.7f;
			StartLoc += OwnerActor->GetActorForwardVector() * 70.f;

			FVector OriginalEndLoc = FVector::ZeroVector;
			if (APE_CharacterBase* TargetChar = GridSystem->GetCharacterAtPosition(RepHoveredTile))
			{
				OriginalEndLoc = TargetChar->GetActorLocation();
				if (UCapsuleComponent* TargetCap = TargetChar->FindComponentByClass<UCapsuleComponent>())
					OriginalEndLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
			}
			else if (AACTile* HoveredTileActor = GridSystem->GetTileAtPosition(RepHoveredTile))
			{
				OriginalEndLoc = HoveredTileActor->GetActorLocation();
				OriginalEndLoc.Z += 20.f; // 바닥 마찰 방지용 미세 오프셋
			}

			// --- [핵심 수정됨: 가상 투사체 스윕(Sweep) 예측 시스템] ---
			FVector FinalEndLoc = OriginalEndLoc;

			if (RepSkillData->ProjectileSpeed > 0.f)
			{
				int32 NumSegments = 20; // 궤적 해상도
				FVector LastPos = StartLoc;

				FCollisionQueryParams Params;
				Params.AddIgnoredActor(OwnerActor);
				// 투사체 충돌 크기 반경 설정 (대략 15cm)
				FCollisionShape SweepShape = FCollisionShape::MakeSphere(15.f);

				TrajectorySpline->AddSplinePoint(StartLoc, ESplineCoordinateSpace::World, false);

				for (int32 i = 1; i <= NumSegments; ++i)
				{
					float Alpha = (float)i / (float)NumSegments;
					FVector NextPos = FMath::Lerp(StartLoc, OriginalEndLoc, Alpha);

					// 곡사일 경우 궤적 고도 반영
					if (RepSkillData->ProjectileGravity > 0.f)
					{
						NextPos.Z += FMath::Sin(Alpha * PI) * RepSkillData->ProjectileGravity;
					}

					FHitResult HitResult;
					// 직사, 곡사 관계없이 궤적 조각을 따라 구체를 훑어 충돌 판정
					if (GetWorld()->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
					{
						// 부딪혔다면 궤적 그리기를 그 자리에서 중단하고 최종 좌표 덮어쓰기
						FinalEndLoc = HitResult.Location;
						TrajectorySpline->AddSplinePoint(FinalEndLoc, ESplineCoordinateSpace::World, false);

						if (APE_CharacterBase* HitChar = Cast<APE_CharacterBase>(HitResult.GetActor()))
						{
							if (UACGridMovementComponent* HitMove = HitChar->GetGridMovementComponent())
								ActualTargetPos = HitMove->GetGridPosition();
						}
						else if (AACTile* HitTile = Cast<AACTile>(HitResult.GetActor()))
						{
							ActualTargetPos = HitTile->GetGridPosition();
						}
						break; // 중도 요격 확인, 더 이상 예측하지 않음
					}
					else
					{
						TrajectorySpline->AddSplinePoint(NextPos, ESplineCoordinateSpace::World, false);
						LastPos = NextPos;
					}
				}
				TrajectorySpline->UpdateSpline();
			}
			else
			{
				// 즉발/장판형 스킬
				TrajectorySpline->AddSplinePoint(StartLoc, ESplineCoordinateSpace::World, false);
				TrajectorySpline->AddSplinePoint(OriginalEndLoc, ESplineCoordinateSpace::World, false);
				TrajectorySpline->UpdateSpline();
			}

			// --- [예측 결과 렌더링: 착탄 지점 구체 생성] ---
			if (ImpactSphereMesh && ImpactSphereMaterial && TrajectorySpline->GetNumberOfSplinePoints() > 1)
			{
				UStaticMeshComponent* ImpactSphere = NewObject<UStaticMeshComponent>(GetOwner());
				ImpactSphere->SetMobility(EComponentMobility::Movable);
				ImpactSphere->SetStaticMesh(ImpactSphereMesh);
				ImpactSphere->SetMaterial(0, ImpactSphereMaterial);
				ImpactSphere->SetRelativeLocation(FinalEndLoc);
				ImpactSphere->SetRelativeScale3D(ImpactSphereScale);
				ImpactSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				ImpactSphere->RegisterComponent();
				GeneratedMeshes.Add(ImpactSphere);
			}

			// 타격 범위 하이라이트 (요격당한 위치 ActualTargetPos 기준)
			TSet<FIntPoint> AffectedGridPositions;
			if (RepSkillData->AoEShape != EPEAoEShape::None)
			{
				AffectedGridPositions = RepSkillData->GetAffectedGridPositions(ActualTargetPos);
				GridSystem->HighlightAoE(OwnerActor, AffectedGridPositions);
			}
			else
			{
				AffectedGridPositions.Add(ActualTargetPos);
				GridSystem->HighlightTarget(OwnerActor, ActualTargetPos);
			}

			// 연쇄 밀치기 시뮬레이션
			const UPE_SkillEffect_Push* PushModule = nullptr;
			for (const UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
			{
				if (const UPE_SkillEffect_Push* FoundPush = Cast<UPE_SkillEffect_Push>(Module))
				{
					PushModule = FoundPush;
					break;
				}
			}

			if (PushModule)
			{
				TArray<FPushSimulationResult> PushResults = PushModule->SimulatePush(GridSystem, MoveComp->GetGridPosition(), AffectedGridPositions);

				for (const FPushSimulationResult& Result : PushResults)
				{
					if (!Result.TargetActor) continue;

					AACTile* StartTile = GridSystem->GetTileAtPosition(Result.StartPos);
					if (!StartTile) continue;

					FVector PushStart = StartTile->GetActorLocation();
					if (UCapsuleComponent* Cap = Result.TargetActor->FindComponentByClass<UCapsuleComponent>())
					{
						PushStart.Z += Cap->GetScaledCapsuleHalfHeight() * 0.5f;
					}

					FVector PushEnd;
					if (Result.StartPos != Result.EndPos)
					{
						AACTile* EndTile = GridSystem->GetTileAtPosition(Result.EndPos);
						PushEnd = EndTile ? EndTile->GetActorLocation() : PushStart;
						PushEnd.Z = PushStart.Z;

						FVector Dir = (PushEnd - PushStart).GetSafeNormal();
						PushEnd += (Dir * PushArrowExtension);
					}
					else
					{
						FVector WorldPushDir = FVector(Result.PushDir.X, Result.PushDir.Y, 0).GetSafeNormal();
						PushEnd = PushStart + (WorldPushDir * 40.f);
						PushEnd.Z = PushStart.Z;
					}

					PushSpline->ClearSplinePoints();
					PushSpline->AddSplinePoint(PushStart, ESplineCoordinateSpace::World, false);
					PushSpline->AddSplinePoint(PushEnd, ESplineCoordinateSpace::World, true);

					GenerateMeshesAlongSpline(PushSpline, PushMaterial, PushThickness, PushArrowSize);
				}
				PushSpline->ClearSplinePoints();
			}

			if (TrajectorySpline->GetNumberOfSplinePoints() > 1)
			{
				GenerateMeshesAlongSpline(TrajectorySpline, TrajectoryMaterial, TrajectoryThickness, TrajectoryArrowSize);
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
			EndPos = P2 - (Dir * PullbackDist * ArrowPullbackMultiplier);
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

		SplineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SplineMesh->RegisterComponent();

		SplineMesh->SetMaterial(0, Mat);
		SplineMesh->UpdateMesh();

		GeneratedMeshes.Add(SplineMesh);
	}
}