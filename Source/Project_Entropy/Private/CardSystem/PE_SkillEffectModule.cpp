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

// --- [모듈 1: 데미지 구현부] ---
void UPE_SkillEffect_Damage::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	for (APE_CharacterBase* Target : Targets)
	{
		if (Target && Instigator)
		{
			UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
		}
	}
}

// --- [모듈 2: 넉백 구현부 (실제 적용 로직)] ---
void UPE_SkillEffect_Push::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (Targets.IsEmpty() || !Instigator || PushDistance <= 0) return;

	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(Instigator, AACGridSystem::StaticClass()));
	UACGridMovementComponent* InstMove = Cast<APE_CharacterBase>(Instigator)->GetGridMovementComponent();
	if (!GridSystem || !InstMove) return;

	FIntPoint InstPos = InstMove->GetGridPosition();

	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GridSystem->GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;
			if (UACGridMovementComponent* CharMove = Char->GetGridMovementComponent())
			{
				FIntPoint Pos = (CharMove->GetTargetGridPosition() != FIntPoint(-999, -999))
					? CharMove->GetTargetGridPosition() : CharMove->GetGridPosition();
				CurrentPosMap.Add(Char, Pos);
			}
		}
	}

	struct FSimulatedPush {
		APE_CharacterBase* Actor;
		int32 RemainingDist;
		FIntPoint PushDir;
		float Delay;
	};
	TArray<FSimulatedPush> PendingPushes;

	for (APE_CharacterBase* Target : Targets)
	{
		if (!Target->IsPushable()) continue;
		if (!CurrentPosMap.Contains(Target)) continue;

		FIntPoint TargetPos = CurrentPosMap[Target];

		FIntPoint PushDir(
			FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
			FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
		);
		if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

		if (PushDir.X != 0 || PushDir.Y != 0)
		{
			PendingPushes.Add({ Target, PushDistance, PushDir, 0.f });
		}
	}

	while (PendingPushes.Num() > 0)
	{
		PendingPushes.Sort([&CurrentPosMap](const FSimulatedPush& A, const FSimulatedPush& B) {
			FIntPoint PosA = CurrentPosMap[A.Actor];
			FIntPoint PosB = CurrentPosMap[B.Actor];
			int32 DotA = PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y;
			int32 DotB = PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y;
			return DotA > DotB;
			});

		FSimulatedPush Task = PendingPushes[0];
		PendingPushes.RemoveAt(0);

		if (!CurrentPosMap.Contains(Task.Actor)) continue;

		FIntPoint CurrentPos = CurrentPosMap[Task.Actor];
		TArray<AACTile*> KnockbackPath;

		bool bHitSomething = false;
		APE_CharacterBase* HitCharacter = nullptr;
		float TimePerTile = 100.f / Task.Actor->GetGridMovementComponent()->GetGridMoveSpeed();

		for (int32 step = 1; step <= Task.RemainingDist; ++step)
		{
			FIntPoint NextPos = CurrentPos + Task.PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			if (!NextTile || NextTile->IsObstacle())
			{
				bHitSomething = true;
				break;
			}

			APE_CharacterBase* CollidedChar = nullptr;
			for (const auto& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == NextPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				bHitSomething = true;
				HitCharacter = CollidedChar;

				if (CollidedChar->IsPushable())
				{
					float HitDelay = Task.Delay + (step * TimePerTile);
					PendingPushes.Add({ CollidedChar, Task.RemainingDist - step, Task.PushDir, HitDelay });
				}
				break;
			}

			CurrentPos = NextPos;
			KnockbackPath.Add(NextTile);
		}

		// 반복문 내에서 즉시 이동 명령 송신 (이동 컴포넌트가 알아서 큐잉 처리)
		if (UACGridMovementComponent* MoveComp = Task.Actor->GetGridMovementComponent())
		{
			if (KnockbackPath.Num() > 0)
			{
				MoveComp->NetMulticast_MoveAlongPath(KnockbackPath, false, Task.Delay);
				CurrentPosMap[Task.Actor] = CurrentPos;
			}
		}

		if (bHitSomething)
		{
			float ScaledRatio = (float)Task.RemainingDist / (float)PushDistance;
			float FinalDamageRatio = CollisionDamageRatio * ScaledRatio;
			float TargetDamage = 0.f;
			float OtherDamage = 0.f;

			if (HitCharacter)
			{
				if (UACStatComponent* HitStat = HitCharacter->GetStatComponent())
				{
					TargetDamage = HitStat->GetMaxHP() * FinalDamageRatio;
					OtherDamage = TargetDamage;
				}
			}
			else
			{
				if (UACStatComponent* MyStat = Task.Actor->GetStatComponent())
				{
					TargetDamage = MyStat->GetMaxHP() * FinalDamageRatio;
				}
			}

			float DamageTime = Task.Delay + (KnockbackPath.Num() * TimePerTile);

			TWeakObjectPtr<APE_CharacterBase> WeakTarget = Task.Actor;
			TWeakObjectPtr<APE_CharacterBase> WeakHitChar = HitCharacter;
			TWeakObjectPtr<AActor> WeakInstigator = Instigator;

			FTimerDelegate DamageDel = FTimerDelegate::CreateLambda([WeakTarget, WeakHitChar, WeakInstigator, TargetDamage, OtherDamage]()
				{
					if (WeakInstigator.IsValid())
					{
						if (WeakTarget.IsValid() && TargetDamage > 0.f)
						{
							UGameplayStatics::ApplyDamage(WeakTarget.Get(), TargetDamage, WeakInstigator->GetInstigatorController(), WeakInstigator.Get(), UDamageType::StaticClass());
						}
						if (WeakHitChar.IsValid() && OtherDamage > 0.f)
						{
							UGameplayStatics::ApplyDamage(WeakHitChar.Get(), OtherDamage, WeakInstigator->GetInstigatorController(), WeakInstigator.Get(), UDamageType::StaticClass());
						}
					}
				});

			FTimerHandle TempHandle;
			Task.Actor->GetWorldTimerManager().SetTimer(TempHandle, DamageDel, FMath::Max(0.01f, DamageTime), false);
		}
	}
}

// --- [모듈 2: 시각화를 위한 넉백 시뮬레이션부] ---
// 실제 로직과 완벽하게 동일한 알고리즘을 사용하되 이동/데미지 명령 대신 결과 구조체만 도출합니다.
TArray<FPushSimulationResult> UPE_SkillEffect_Push::SimulatePush(AACGridSystem* GridSystem, FIntPoint InstigatorPos, const TSet<FIntPoint>& AffectedGridPositions) const
{
	TArray<FPushSimulationResult> Results;
	if (!GridSystem || PushDistance <= 0) return Results;

	TMap<APE_CharacterBase*, FIntPoint> InitialPosMap;
	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	TMap<APE_CharacterBase*, FIntPoint> PushDirMap;

	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GridSystem->GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;
			if (UACGridMovementComponent* CharMove = Char->GetGridMovementComponent())
			{
				FIntPoint Pos = (CharMove->GetTargetGridPosition() != FIntPoint(-999, -999))
					? CharMove->GetTargetGridPosition() : CharMove->GetGridPosition();

				InitialPosMap.Add(Char, Pos);
				CurrentPosMap.Add(Char, Pos);
			}
		}
	}

	struct FSimulatedPush {
		APE_CharacterBase* Actor;
		int32 RemainingDist;
		FIntPoint PushDir;
	};
	TArray<FSimulatedPush> PendingPushes;

	for (const FIntPoint& TargetPos : AffectedGridPositions)
	{
		APE_CharacterBase* HitChar = nullptr;
		for (const auto& Pair : CurrentPosMap)
		{
			if (Pair.Value == TargetPos)
			{
				HitChar = Pair.Key;
				break;
			}
		}

		if (HitChar && HitChar->IsPushable())
		{
			FIntPoint PushDir(
				FMath::Clamp(TargetPos.X - InstigatorPos.X, -1, 1),
				FMath::Clamp(TargetPos.Y - InstigatorPos.Y, -1, 1)
			);
			if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

			if (PushDir.X != 0 || PushDir.Y != 0)
			{
				PendingPushes.Add({ HitChar, PushDistance, PushDir });
				PushDirMap.Add(HitChar, PushDir);
			}
		}
	}

	while (PendingPushes.Num() > 0)
	{
		// 시각화에서도 실제 처리와 동일하게 Back-to-Front 정렬
		PendingPushes.Sort([&CurrentPosMap](const FSimulatedPush& A, const FSimulatedPush& B) {
			FIntPoint PosA = CurrentPosMap[A.Actor];
			FIntPoint PosB = CurrentPosMap[B.Actor];
			int32 DotA = PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y;
			int32 DotB = PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y;
			return DotA > DotB;
			});

		FSimulatedPush Task = PendingPushes[0];
		PendingPushes.RemoveAt(0);

		if (!CurrentPosMap.Contains(Task.Actor)) continue;

		FIntPoint CurrentPos = CurrentPosMap[Task.Actor];

		for (int32 step = 1; step <= Task.RemainingDist; ++step)
		{
			FIntPoint NextPos = CurrentPos + Task.PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			if (!NextTile || NextTile->IsObstacle()) break;

			APE_CharacterBase* CollidedChar = nullptr;
			for (const auto& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == NextPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				if (CollidedChar->IsPushable())
				{
					PendingPushes.Add({ CollidedChar, Task.RemainingDist - step, Task.PushDir });
					PushDirMap.Add(CollidedChar, Task.PushDir);
				}
				break;
			}

			CurrentPos = NextPos;
		}

		CurrentPosMap[Task.Actor] = CurrentPos;
	}

	for (const auto& Pair : PushDirMap)
	{
		APE_CharacterBase* Char = Pair.Key;
		FPushSimulationResult Result;
		Result.TargetActor = Char;
		Result.StartPos = InitialPosMap[Char];
		Result.EndPos = CurrentPosMap[Char];
		Result.PushDir = Pair.Value;
		Results.Add(Result);
	}

	return Results;
}