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

			FVector EndLoc = FVector::ZeroVector;
			if (APE_CharacterBase* TargetChar = GridSystem->GetCharacterAtPosition(RepHoveredTile))
			{
				EndLoc = TargetChar->GetActorLocation();
				if (UCapsuleComponent* TargetCap = TargetChar->FindComponentByClass<UCapsuleComponent>())
					EndLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
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

			// --- 실제 타격 범위(AoE) 타일들 저장 및 하이라이트 ---
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

			// --- [가상 보드 및 연쇄 밀치기 시뮬레이션] ---
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
				FIntPoint InstPos = MoveComp->GetGridPosition();

				// 1. 가상 보드(VirtualBoard) 구축: 서버의 이동 점유 시스템(StartPos, EndPos)을 완벽히 모방
				struct FSimCharState {
					FIntPoint StartPos;
					FIntPoint EndPos;
					bool bIsPushable;
				};
				TMap<APE_CharacterBase*, FSimCharState> VirtualBoard;

				TArray<AActor*> AllChars;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

				for (AActor* Actor : AllChars)
				{
					if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
					{
						if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;
						if (UACGridMovementComponent* CharMove = Char->GetGridMovementComponent())
						{
							FIntPoint Pos = CharMove->GetGridPosition();
							FIntPoint TargetPos = CharMove->GetTargetGridPosition();
							FIntPoint EPos = (TargetPos != FIntPoint(-999, -999)) ? TargetPos : Pos;
							VirtualBoard.Add(Char, { Pos, EPos, Char->IsPushable() });
						}
					}
				}

				struct FSimulatedPush {
					APE_CharacterBase* Actor;
					int32 RemainingDist;
					FIntPoint PushDir;
				};
				TQueue<FSimulatedPush> SimQueue;

				// 2. 1차 타격 대상을 가상 보드에서 찾아 큐에 삽입 (서버와 동일한 판정 기준)
				for (const FIntPoint& TargetPos : AffectedGridPositions)
				{
					APE_CharacterBase* HitChar = nullptr;
					for (const auto& Pair : VirtualBoard)
					{
						if (Pair.Value.StartPos == TargetPos || Pair.Value.EndPos == TargetPos)
						{
							HitChar = Pair.Key;
							break;
						}
					}

					if (HitChar && VirtualBoard[HitChar].bIsPushable)
					{
						FIntPoint PushDir(
							FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
							FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
						);
						if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

						SimQueue.Enqueue({ HitChar, PushDist, PushDir });
					}
				}

				// 3. 당구 연쇄 루프 실행
				while (!SimQueue.IsEmpty())
				{
					FSimulatedPush Task;
					SimQueue.Dequeue(Task);

					if (!Task.Actor || Task.RemainingDist <= 0 || !VirtualBoard.Contains(Task.Actor)) continue;

					// 연쇄 충돌 시 이 캐릭터가 위치한 '마지막 예약 지점(EndPos)'에서 출발
					FIntPoint CurrentPos = VirtualBoard[Task.Actor].EndPos;
					FIntPoint ExpectedEndPos = CurrentPos;

					for (int32 step = 1; step <= Task.RemainingDist; ++step)
					{
						FIntPoint CheckPos = ExpectedEndPos + Task.PushDir;
						AACTile* CheckTile = GridSystem->GetTileAtPosition(CheckPos);

						// 비파괴 장애물에 막힘
						if (!CheckTile || CheckTile->IsObstacle()) break;

						// 가상 보드 내 캐릭터/동적 장애물 충돌 검사
						APE_CharacterBase* CollidedChar = nullptr;
						for (const auto& Pair : VirtualBoard)
						{
							if (Pair.Key == Task.Actor) continue;
							if (Pair.Value.StartPos == CheckPos || Pair.Value.EndPos == CheckPos)
							{
								CollidedChar = Pair.Key;
								break;
							}
						}

						if (CollidedChar)
						{
							// 내가 치고 지나간 대상이 또 밀릴 수 있다면 당구 큐에 예약!
							if (VirtualBoard[CollidedChar].bIsPushable)
							{
								SimQueue.Enqueue({ CollidedChar, Task.RemainingDist - step, Task.PushDir });
							}
							break; // 나는 여기서 멈춤
						}

						ExpectedEndPos = CheckPos;
					}

					// 거리가 0이라 제자리(CurrentPos == ExpectedEndPos)라면 화살표를 절대 그리지 않음!
					if (ExpectedEndPos != CurrentPos)
					{
						// 가상 보드 상에서 내 최종 도착점 갱신 (다른 연쇄 충돌 방지용)
						VirtualBoard[Task.Actor].EndPos = ExpectedEndPos;

						FVector PushStart = GridSystem->GetTileAtPosition(CurrentPos)->GetActorLocation();
						FVector PushEnd = GridSystem->GetTileAtPosition(ExpectedEndPos)->GetActorLocation();

						if (UCapsuleComponent* Cap = Task.Actor->FindComponentByClass<UCapsuleComponent>())
							PushStart.Z += Cap->GetScaledCapsuleHalfHeight() * 0.5f;
						PushEnd.Z = PushStart.Z;

						FVector Dir = (PushEnd - PushStart).GetSafeNormal();
						PushEnd += (Dir * PushArrowExtension);

						PushSpline->ClearSplinePoints();
						PushSpline->AddSplinePoint(PushStart, ESplineCoordinateSpace::World, false);
						PushSpline->AddSplinePoint(PushEnd, ESplineCoordinateSpace::World, true);

						GenerateMeshesAlongSpline(PushSpline, PushMaterial, PushThickness, PushArrowSize);
					}
				}
				PushSpline->ClearSplinePoints();
			}

			// 본체의 스킬 발사 궤적(붉은 선) 메쉬 생성
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