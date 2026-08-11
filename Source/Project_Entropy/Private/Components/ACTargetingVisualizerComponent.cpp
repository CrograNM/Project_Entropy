// Copyright CrograNM

#include "Components/ACTargetingVisualizerComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/SplineComponent.h" 
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
	PrimaryComponentTick.bCanEverTick = true;
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

	// 1. 사거리(Range) 표시
	if (RepTargetingMode != ETargetingMode::None && RepRange > 0)
	{
		bool bIsMovement = (RepTargetingMode == ETargetingMode::Movement);
		CurrentValidTiles = GridSystem->HighlightArea(OwnerActor, CenterPos, RepRange, bIsMovement);
	}

	// 2. 마우스 호버링 세부 표시
	if (RepHoveredTile != FIntPoint(-999, -999) && RepTargetingMode != ETargetingMode::None)
	{
		if (RepTargetingMode == ETargetingMode::Movement)
		{
			GridSystem->HighlightPath(OwnerActor, CenterPos, RepHoveredTile, CurrentValidTiles);
		}
		else if (RepTargetingMode == ETargetingMode::Skill && RepSkillData)
		{
			// --- [물리 시뮬레이션: 직사/곡사에 따른 실제 타격 위치 도출] ---
			FIntPoint ActualTargetPos = RepHoveredTile; // 기본값: 마우스가 위치한 타일

			// 가슴 높이 출발점 계산
			FVector StartLoc = OwnerActor->GetActorLocation();
			if (UCapsuleComponent* Cap = OwnerActor->FindComponentByClass<UCapsuleComponent>())
				StartLoc.Z = Cap->GetScaledCapsuleHalfHeight() * 0.7f;
			StartLoc += OwnerActor->GetActorForwardVector() * 70.f;

			// 예상 도착점 계산
			FVector EndLoc = FVector::ZeroVector;
			if (APE_CharacterBase* TargetChar = GridSystem->GetCharacterAtPosition(RepHoveredTile))
			{
				EndLoc = TargetChar->GetActorLocation();
				if (UCapsuleComponent* TargetCap = TargetChar->FindComponentByClass<UCapsuleComponent>())
					EndLoc.Z = TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
			}
			else if (AACTile* HoveredTileActor = GridSystem->GetTileAtPosition(RepHoveredTile))
			{
				EndLoc = HoveredTileActor->GetActorLocation(); // 바닥(용암 등 높이가 없는 경우)
			}

			// [직사 판정] 장애물이 스킬을 막는지 레이캐스트 시뮬레이션
			if (RepSkillData->ProjectileSpeed > 0.f && RepSkillData->ProjectileGravity == 0.f)
			{
				FHitResult HitResult;
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(OwnerActor);

				if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, Params))
				{
					EndLoc = HitResult.Location; // 막힌 지점의 3D 좌표로 갱신!

					// 막힌 대상이 캐릭터/장애물이면 타겟 타일 좌표를 그 녀석이 서 있는 곳으로 보정
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

			// --- [궤적 스플라인 포인트 생성] ---
			if (RepSkillData->ProjectileGravity > 0.f) // 곡사 (포물선)
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
			else // 직사 (직선)
			{
				TrajectorySpline->AddSplinePoint(StartLoc, ESplineCoordinateSpace::World, false);
				TrajectorySpline->AddSplinePoint(EndLoc, ESplineCoordinateSpace::World, true);
			}

			// --- [실제 타격 타일 하이라이트] ---
			// (마우스 위치가 아닌, 레이캐스트로 보정된 ActualTargetPos를 칠합니다!)
			if (RepSkillData->AoEShape != EPEAoEShape::None)
			{
				TSet<FIntPoint> AoE = RepSkillData->GetAffectedGridPositions(ActualTargetPos);
				GridSystem->HighlightAoE(OwnerActor, AoE);
			}
			else
			{
				GridSystem->HighlightTarget(OwnerActor, ActualTargetPos);
			}

			// --- [밀치기(Push) 시뮬레이션 및 2차 화살표 생성] ---
			
			// 스킬에 장착된 모듈 배열을 뒤져서 '밀치기 모듈'이 있는지 확인
			const UPE_SkillEffect_Push* PushModule = nullptr;
			for (const UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
			{
				if (const UPE_SkillEffect_Push* FoundPush = Cast<UPE_SkillEffect_Push>(Module))
				{
					PushModule = FoundPush;
					break; // 찾았으면 루프 종료
				}
			}

			// 밀치기 모듈이 존재하고, 그 거리가 0보다 크다면 화살표를 그립니다.
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

						// 어디까지 밀려날지 예상 루프
						FIntPoint ExpectedEndPos = ActualTargetPos;

						// 모듈에서 빼온 거리(PushDist)를 사용
						for (int32 step = 1; step <= PushDist; ++step)
						{
							FIntPoint CheckPos = ExpectedEndPos + PushDir;
							AACTile* CheckTile = GridSystem->GetTileAtPosition(CheckPos);

							if (!CheckTile || CheckTile->IsObstacle() || GridSystem->IsTileOccupied(CheckPos, HitChar))
								break;

							ExpectedEndPos = CheckPos;
						}

						// 타겟이 실제로 뒤로 밀린다면 주황색 넉백 화살표 포인트 생성
						if (ExpectedEndPos != ActualTargetPos)
						{

							FVector PushStart = HitChar->GetActorLocation();
							FVector PushEnd = GridSystem->GetTileAtPosition(ExpectedEndPos)->GetActorLocation();

							if (UCapsuleComponent* Cap = HitChar->FindComponentByClass<UCapsuleComponent>())
								PushStart.Z = Cap->GetScaledCapsuleHalfHeight() * 0.5f;
							PushEnd.Z = PushStart.Z; // 수평 화살표

							PushSpline->AddSplinePoint(PushStart, ESplineCoordinateSpace::World, false);
							PushSpline->AddSplinePoint(PushEnd, ESplineCoordinateSpace::World, true);
						}
					}
				}
			}
		}
	}
}

void UACTargetingVisualizerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (RepTargetingMode == ETargetingMode::Skill)
	{
		// 1. 스킬 발사 궤적 렌더링
		int32 TrajPoints = TrajectorySpline->GetNumberOfSplinePoints();
		if (TrajPoints > 1)
		{
			for (int32 i = 0; i < TrajPoints - 1; ++i)
			{
				FVector P1 = TrajectorySpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
				FVector P2 = TrajectorySpline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

				// 마지막 선분(구간)일 경우 화살표를 그리고 뒤로 살짝 당김
				if (i == TrajPoints - 2)
				{
					FVector Dir = (P2 - P1).GetSafeNormal();
					float Dist = FVector::Distance(P1, P2);
					// 타겟과 너무 가까우면 방향이 뒤집히지 않게 최소 거리를 보장하여 당깁니다.
					float PullbackDist = FMath::Min(TrajectoryArrowSize * 0.6f, Dist * 0.5f);
					FVector ShortenedP2 = P2 - (Dir * PullbackDist);

					DrawDebugDirectionalArrow(GetWorld(), P1, ShortenedP2, TrajectoryArrowSize, TrajectoryColor, false, -1.f, 0, TrajectoryThickness);
				}
				else
				{
					DrawDebugLine(GetWorld(), P1, P2, TrajectoryColor, false, -1.f, 0, TrajectoryThickness);
				}
			}
		}

		// 2. 밀치기(넉백) 예상 궤적 렌더링
		int32 PushPoints = PushSpline->GetNumberOfSplinePoints();
		if (PushPoints > 1)
		{
			FVector P1 = PushSpline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
			FVector P2 = PushSpline->GetLocationAtSplinePoint(PushPoints - 1, ESplineCoordinateSpace::World);

			FVector Dir = (P2 - P1).GetSafeNormal();

			FVector ExtendedP2 = P2 + (Dir * PushArrowExtension);

			DrawDebugDirectionalArrow(GetWorld(), P1, ExtendedP2, PushArrowSize, PushColor, false, -1.f, 0, PushThickness);
		}
	}
}