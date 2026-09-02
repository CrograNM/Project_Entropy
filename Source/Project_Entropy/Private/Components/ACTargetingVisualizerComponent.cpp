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

			// 캐릭터의 현재 회전값이 아닌, 마우스 타일 방향(예정 회전 방향)을 도출하여 궤적 시작 오프셋을 잡습니다.
			FVector IntendedDir = OwnerActor->GetActorForwardVector();
			if (AACTile* HoveredTile = GridSystem->GetTileAtPosition(RepHoveredTile))
			{
				FVector TargetLoc = HoveredTile->GetActorLocation();
				FVector CalcDir = (TargetLoc - OwnerActor->GetActorLocation()).GetSafeNormal2D();
				if (!CalcDir.IsNearlyZero())
				{
					IntendedDir = CalcDir;
				}
			}
			StartLoc += IntendedDir * 70.f;

			FVector OriginalEndLoc = FVector::ZeroVector;

			// 단일 스킬 구조체에서 배열로 바뀌었으므로, 궤적(화살표)을 그릴 대표 페이즈를 첫 번째 페이즈로 지정
			const FPESkillHitPhase* RepPhase = (RepSkillData->HitPhases.Num() > 0) ? &RepSkillData->HitPhases[0] : nullptr;

			// 레이저 목표 타일이 그리드(맵) 밖을 벗어나지 않도록 최대 사거리 내의 '가장 마지막 유효 타일'로 보정합니다.
			if (RepPhase && RepPhase->AoEShape == EPEAoEShape::Line)
			{
				FVector2D CasterV(CenterPos.X, CenterPos.Y);
				FVector2D TargetV(ActualTargetPos.X, ActualTargetPos.Y);
				FVector2D Dir = (TargetV - CasterV).GetSafeNormal();

				if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

				FIntPoint LastValidPos = CenterPos;
				for (int32 i = 1; i <= RepSkillData->BaseRange; ++i)
				{
					FIntPoint TestPos = CenterPos + FIntPoint(FMath::RoundToInt(Dir.X * i), FMath::RoundToInt(Dir.Y * i));
					if (GridSystem->GetTileAtPosition(TestPos)) LastValidPos = TestPos;
					else break;
				}
				ActualTargetPos = LastValidPos;
			}

			if (APE_CharacterBase* TargetChar = GridSystem->GetCharacterAtPosition(ActualTargetPos))
			{
				OriginalEndLoc = TargetChar->GetActorLocation();
				if (UCapsuleComponent* TargetCap = TargetChar->FindComponentByClass<UCapsuleComponent>())
					OriginalEndLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
			}
			else if (AACTile* HoveredTileActor = GridSystem->GetTileAtPosition(ActualTargetPos))
			{
				OriginalEndLoc = HoveredTileActor->GetActorLocation();
				OriginalEndLoc.Z += 20.f;
			}

			FVector FinalEndLoc = OriginalEndLoc;

			// 대표 페이즈의 투사체 속성 기반으로 궤적 스플라인 렌더링[cite: 31]
			if (RepPhase && RepPhase->ProjectileSpeed > 0.f)
			{
				int32 NumSegments = 20;
				FVector LastPos = StartLoc;
				TrajectorySpline->AddSplinePoint(StartLoc, ESplineCoordinateSpace::World, false);

				if (RepPhase->bDestroyOnHit)
				{
					FCollisionQueryParams Params;
					Params.AddIgnoredActor(OwnerActor);
					FCollisionShape SweepShape = FCollisionShape::MakeSphere(5.f);

					for (int32 i = 1; i <= NumSegments; ++i)
					{
						float Alpha = (float)i / (float)NumSegments;
						FVector NextPos = FMath::Lerp(StartLoc, OriginalEndLoc, Alpha);

						if (RepPhase->ProjectileGravity > 0.f)
							NextPos.Z += FMath::Sin(Alpha * PI) * RepPhase->ProjectileGravity;

						FHitResult HitResult;
						if (GetWorld()->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
						{
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
							break;
						}
						else
						{
							TrajectorySpline->AddSplinePoint(NextPos, ESplineCoordinateSpace::World, false);
							LastPos = NextPos;
						}
					}
				}
				else
				{
					// 관통/장판: 장애물을 무시하고 사거리 끝단(OriginalEndLoc)까지 스플라인을 그립니다.
					for (int32 i = 1; i <= NumSegments; ++i)
					{
						float Alpha = (float)i / (float)NumSegments;
						FVector NextPos = FMath::Lerp(StartLoc, OriginalEndLoc, Alpha);
						if (RepPhase->ProjectileGravity > 0.f)
							NextPos.Z += FMath::Sin(Alpha * PI) * RepPhase->ProjectileGravity;
						TrajectorySpline->AddSplinePoint(NextPos, ESplineCoordinateSpace::World, false);
					}
					FinalEndLoc = OriginalEndLoc;
				}
				TrajectorySpline->UpdateSpline();
			}
			else
			{
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

			// 페이즈 배열 전체를 순회하며 칠해질 타일들의 합집합(Union)을 수집합니다
			TSet<FIntPoint> MasterAffectedPositions;
			if (RepSkillData->HitPhases.Num() > 0)
			{
				for (const FPESkillHitPhase& Phase : RepSkillData->HitPhases)
				{
					FIntPoint PhaseTargetPos = RepHoveredTile;

					if (Phase.AoEShape == EPEAoEShape::Line)
					{
						FVector2D CasterV(CenterPos.X, CenterPos.Y);
						FVector2D TargetV(PhaseTargetPos.X, PhaseTargetPos.Y);
						FVector2D Dir = (TargetV - CasterV).GetSafeNormal();
						if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

						FIntPoint LastValidPos = CenterPos;
						for (int32 i = 1; i <= RepSkillData->BaseRange; ++i)
						{
							FIntPoint TestPos = CenterPos + FIntPoint(FMath::RoundToInt(Dir.X * i), FMath::RoundToInt(Dir.Y * i));
							if (GridSystem->GetTileAtPosition(TestPos)) LastValidPos = TestPos;
							else break;
						}
						PhaseTargetPos = LastValidPos;
					}
					MasterAffectedPositions.Append(Phase.GetAffectedGridPositions(CenterPos, PhaseTargetPos, RepSkillData->BaseRange));
				}
				GridSystem->HighlightAoE(OwnerActor, MasterAffectedPositions);
			}
			else
			{
				MasterAffectedPositions.Add(ActualTargetPos);
				GridSystem->HighlightTarget(OwnerActor, ActualTargetPos);
			}

			// 넉백 모듈은 여러 페이즈 중 최초 1개만 찾아서 한 번만 시뮬레이션 및 시각화합니다.
			const UPE_SkillEffect_Push* PushModule = nullptr;
			FIntPoint PushTargetPos = ActualTargetPos;

			if (RepSkillData->HitPhases.Num() > 0)
			{
				for (const FPESkillHitPhase& Phase : RepSkillData->HitPhases)
				{
					for (const UPE_SkillEffectModule* Module : Phase.EffectModules)
					{
						if (const UPE_SkillEffect_Push* FoundPush = Cast<UPE_SkillEffect_Push>(Module))
						{
							PushModule = FoundPush;
							PushTargetPos = RepHoveredTile;

							// 시각화를 그릴 모듈이 Line 형태에 속한다면 Target 위치 보정 적용
							if (Phase.AoEShape == EPEAoEShape::Line)
							{
								FVector2D CasterV(CenterPos.X, CenterPos.Y);
								FVector2D TargetV(PushTargetPos.X, PushTargetPos.Y);
								FVector2D Dir = (TargetV - CasterV).GetSafeNormal();
								if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

								FIntPoint LastValidPos = CenterPos;
								for (int32 i = 1; i <= RepSkillData->BaseRange; ++i)
								{
									FIntPoint TestPos = CenterPos + FIntPoint(FMath::RoundToInt(Dir.X * i), FMath::RoundToInt(Dir.Y * i));
									if (GridSystem->GetTileAtPosition(TestPos)) LastValidPos = TestPos;
									else break;
								}
								PushTargetPos = LastValidPos;
							}
							break;
						}
					}
					if (PushModule) break; // 모듈을 찾았다면 더 이상 다른 페이즈를 탐색하지 않고 종료
				}
			}

			// 단 한 번만 실행되는 밀치기 화살표 시각화 로직
			if (PushModule)
			{
				TArray<FPushSimulationResult> PushResults = PushModule->SimulatePush(GridSystem, OwnerActor, PushTargetPos, MasterAffectedPositions);

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