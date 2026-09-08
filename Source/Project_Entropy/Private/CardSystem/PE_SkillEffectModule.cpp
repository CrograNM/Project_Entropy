// Copyright CrograNM

#include "CardSystem/PE_SkillEffectModule.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Queue.h"
#include "Core/PE_GameState.h" 

namespace
{
	// 격자 델타를 상하좌우 4방향 중 하나로 스냅합니다. 정확한 대각선(|dx| == |dy|)은 항상 '수평'으로 해석합니다.
	FIntPoint SnapToCardinalDirection(FIntPoint Delta)
	{
		if (Delta == FIntPoint::ZeroValue) return FIntPoint::ZeroValue;

		return (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
			? FIntPoint(Delta.X > 0 ? 1 : -1, 0)
			: FIntPoint(0, Delta.Y > 0 ? 1 : -1);
	}
}

// --- [모듈 1: 데미지 구현부] ---
void UPE_SkillEffect_Damage::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	for (APE_CharacterBase* Target : Targets)
	{
		if (Target && Instigator)
		{
			UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
		}
	}
}

// --- [모듈 2: 넉백 시뮬레이션 (단일 진실 공급원)] ---
TArray<FPushSimulationResult> UPE_SkillEffect_Push::SimulatePush(const AACGridSystem* GridSystem, AActor* Instigator, FIntPoint TargetGridPos, const TSet<APE_CharacterBase*>& Targets) const
{
	TArray<FPushSimulationResult> Results;

	APE_CharacterBase* InstigatorChar = Cast<APE_CharacterBase>(Instigator);
	if (!GridSystem || !InstigatorChar || PushDistance <= 0 || Targets.IsEmpty()) return Results;

	UACGridMovementComponent* InstMove = InstigatorChar->GetGridMovementComponent();
	if (!InstMove) return Results;

	const FIntPoint InstigatorPos = InstMove->GetGridPosition();

	// 연쇄 충돌 판정을 위해 전장 전체의 배치를 점유 레지스트리에서 스냅샷으로 떠 옵니다.
	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	for (const TPair<FIntPoint, APE_CharacterBase*>& Entry : GridSystem->GetOccupancyMap())
	{
		APE_CharacterBase* Char = Entry.Value;
		if (!Char) continue;
		if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;

		CurrentPosMap.Add(Char, Entry.Key);
	}

	// 지향성(Directional) 밀치기는 시전자 -> 목표 칸 방향을 4방향으로 스냅해서 씁니다.
	FIntPoint DirectionalDir = SnapToCardinalDirection(TargetGridPos - InstigatorPos);
	if (DirectionalDir == FIntPoint::ZeroValue) DirectionalDir = FIntPoint(1, 0); // 제자리 조준 보호

	struct FPendingPush
	{
		APE_CharacterBase* Actor = nullptr;
		int32 RemainingDist = 0;
		FIntPoint PushDir = FIntPoint::ZeroValue;
		float Delay = 0.f;
	};
	TArray<FPendingPush> PendingPushes;

	// 1단계: 피격 대상별로 밀려날 방향을 결정합니다.
	for (APE_CharacterBase* Target : Targets)
	{
		if (!Target || !Target->IsPushable() || Target == InstigatorChar) continue;

		const FIntPoint* FoundPos = CurrentPosMap.Find(Target);
		if (!FoundPos) continue;

		const FIntPoint TargetPos = *FoundPos;
		FIntPoint FinalPushDir(0, 0);

		if (PushType == EPEPushType::Directional)
		{
			FinalPushDir = DirectionalDir;
		}
		else
		{
			/*
				폭발 중심에 정확히 선 대상은 시전자 반대편으로, 나머지는 중심에서 바깥으로 밀립니다.
			*/
			const FIntPoint Origin = (TargetPos == TargetGridPos) ? InstigatorPos : TargetGridPos;
			FinalPushDir = SnapToCardinalDirection(TargetPos - Origin);
		}

		if (FinalPushDir != FIntPoint::ZeroValue)
		{
			PendingPushes.Add({ Target, PushDistance, FinalPushDir, 0.f });
		}
	}

	// 2단계: 뒤쪽(진행 방향 기준 앞선) 캐릭터부터 처리하며 연쇄 밀치기를 전개합니다.
	while (PendingPushes.Num() > 0)
	{
		PendingPushes.Sort([&CurrentPosMap](const FPendingPush& A, const FPendingPush& B)
			{
				const FIntPoint PosA = CurrentPosMap[A.Actor];
				const FIntPoint PosB = CurrentPosMap[B.Actor];
				return (PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y) > (PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y);
			});

		const FPendingPush Task = PendingPushes[0];
		PendingPushes.RemoveAt(0);

		if (!CurrentPosMap.Contains(Task.Actor)) continue;

		FPushSimulationResult Result;
		Result.TargetActor = Task.Actor;
		Result.StartPos = CurrentPosMap[Task.Actor];
		Result.PushDir = Task.PushDir;
		Result.RemainingDist = Task.RemainingDist;
		Result.Delay = Task.Delay;

		// 이 캐릭터가 한 칸 지나가는 데 걸리는 시간 (연쇄 대상의 출발 지연 계산용)
		const UACGridMovementComponent* TaskMove = Task.Actor->GetGridMovementComponent();
		const float TimePerTile = (TaskMove && TaskMove->GetGridMoveSpeed() > 0.f) ? (100.f / TaskMove->GetGridMoveSpeed()) : 0.f;

		FIntPoint CurrentPos = Result.StartPos;

		for (int32 Step = 1; Step <= Task.RemainingDist; ++Step)
		{
			const FIntPoint NextPos = CurrentPos + Task.PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			// 맵 밖이거나 고정 장애물 타일이면 그 자리에서 멈춥니다.
			if (!NextTile || NextTile->IsObstacle())
			{
				Result.bBlocked = true;
				break;
			}

			APE_CharacterBase* CollidedChar = nullptr;
			for (const TPair<APE_CharacterBase*, FIntPoint>& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == NextPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				Result.bBlocked = true;
				Result.HitCharacter = CollidedChar;

				// 부딪힌 상대가 밀릴 수 있다면 남은 거리를 넘겨 연쇄시킵니다.
				if (CollidedChar->IsPushable())
				{
					PendingPushes.Add({ CollidedChar, Task.RemainingDist - Step, Task.PushDir, Task.Delay + (Step * TimePerTile) });
				}
				break;
			}

			CurrentPos = NextPos;
			Result.Path.Add(NextTile);
		}

		Result.EndPos = CurrentPos;
		CurrentPosMap[Task.Actor] = CurrentPos;

		Results.Add(Result);
	}

	return Results;
}

// --- [모듈 2: 넉백 실행부] ---
void UPE_SkillEffect_Push::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	APE_CharacterBase* InstigatorChar = Cast<APE_CharacterBase>(Instigator);
	if (!InstigatorChar) return;

	UACGridMovementComponent* InstMove = InstigatorChar->GetGridMovementComponent();
	AACGridSystem* GridSystem = InstMove ? InstMove->GetCachedGridSystem() : nullptr;
	if (!GridSystem) return;

	// 계산은 전부 시뮬레이션에 맡기고, 여기서는 그 결과를 실행하기만 합니다.
	const TArray<FPushSimulationResult> PushResults = SimulatePush(GridSystem, Instigator, TargetGridPos, Targets);

	APE_GameState* GS = Instigator->GetWorld() ? Instigator->GetWorld()->GetGameState<APE_GameState>() : nullptr;

	for (const FPushSimulationResult& Result : PushResults)
	{
		if (!Result.TargetActor) continue;

		FGridKnockbackPayload Payload;
		Payload.bIsActive = true;
		Payload.Instigator = Instigator;
		Payload.SkillData = InSkillData;

		// 충돌 피해는 '남아있던 밀림 거리' 비율만큼만 들어갑니다 (덜 밀렸으면 덜 아픔).
		if (Result.bBlocked)
		{
			const float FinalDamageRatio = CollisionDamageRatio * ((float)Result.RemainingDist / (float)PushDistance);

			if (Result.HitCharacter)
			{
				if (UACStatComponent* HitStat = Result.HitCharacter->GetStatComponent())
				{
					Payload.TargetDamage = HitStat->GetMaxHP() * FinalDamageRatio;
					Payload.OtherDamage = Payload.TargetDamage;
					Payload.HitCharacter = Result.HitCharacter;
				}
			}
			else if (UACStatComponent* MyStat = Result.TargetActor->GetStatComponent())
			{
				// 벽에 부딪힌 경우는 본인만 피해를 입습니다.
				Payload.TargetDamage = MyStat->GetMaxHP() * FinalDamageRatio;
			}
		}

		// 개별 밀치기가 실행될 때마다 UI에 등록하고 전용 토큰을 발급받습니다.
		if (GS)
		{
			FString LogText = FString::Printf(TEXT("%s - %d칸 밀림"), *Result.TargetActor->GetName(), Result.RemainingDist);
			Payload.ActionLogID = GS->AddActionLog(Result.TargetActor->GetTeamID(), LogText);
			Payload.ActionTokenID = GS->BeginAction(
				FString::Printf(TEXT("Push:%s(%d칸)"), *Result.TargetActor->GetName(), Result.RemainingDist), Payload.ActionLogID);
		}

		if (UACGridMovementComponent* MoveComp = Result.TargetActor->GetGridMovementComponent())
		{
			MoveComp->NetMulticast_MoveAlongPath(Result.Path, false, Result.Delay, Payload);
		}
		else if (GS)
		{
			// 이동 컴포넌트가 없어 페이로드를 넘기지 못했다면 발급한 토큰을 즉시 되돌려 누수를 막습니다.
			GS->EndAction(Payload.ActionTokenID, Payload.ActionLogID);
		}
	}
}
