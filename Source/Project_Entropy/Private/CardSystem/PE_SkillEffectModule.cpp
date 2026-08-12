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
void UPE_SkillEffect_Damage::ApplyEffect(AActor* Instigator, APE_CharacterBase* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (Target && Instigator)
	{
		UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
	}
}

// --- [모듈 2: 넉백 구현부] ---
void UPE_SkillEffect_Push::ApplyEffect(AActor* Instigator, APE_CharacterBase* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!Target || !Instigator || PushDistance <= 0 || !Target->IsPushable()) return;

	UACGridMovementComponent* TargetMove = Target->GetGridMovementComponent();
	UACGridMovementComponent* InstMove = Cast<APE_CharacterBase>(Instigator)->GetGridMovementComponent();
	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(Target, AACGridSystem::StaticClass()));

	if (!TargetMove || !InstMove || !GridSystem) return;

	FIntPoint InstPos = InstMove->GetGridPosition();
	FIntPoint TargetPos = TargetMove->GetGridPosition();

	FIntPoint PushDir(
		FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
		FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
	);
	if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

	struct FPushTask {
		APE_CharacterBase* ActorToPush;
		int32 RemainingDist;
		float StartTimeDelay;
	};

	TQueue<FPushTask> PushQueue;
	PushQueue.Enqueue({ Target, PushDistance, 0.f });

	float TimePerTile = 100.f / TargetMove->GetGridMoveSpeed();

	while (!PushQueue.IsEmpty())
	{
		FPushTask Task;
		PushQueue.Dequeue(Task);

		APE_CharacterBase* CurrentTarget = Task.ActorToPush;
		int32 RemainingDist = Task.RemainingDist;
		float StartDelay = Task.StartTimeDelay;

		if (!CurrentTarget || RemainingDist <= 0 || !CurrentTarget->IsPushable()) continue;

		UACGridMovementComponent* MoveComp = CurrentTarget->GetGridMovementComponent();
		if (!MoveComp) continue;

		FIntPoint VirtualPos = (MoveComp->GetTargetGridPosition() != FIntPoint(-999, -999))
			? MoveComp->GetTargetGridPosition() : MoveComp->GetGridPosition();

		TArray<AACTile*> KnockbackPath;
		bool bHitSomething = false;
		APE_CharacterBase* HitCharacter = nullptr;

		for (int32 step = 1; step <= RemainingDist; ++step)
		{
			FIntPoint NextPos = VirtualPos + PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			if (!NextTile || NextTile->IsObstacle())
			{
				bHitSomething = true;
				break;
			}

			if (APE_CharacterBase* HitChar = GridSystem->GetCharacterAtPosition(NextPos, CurrentTarget))
			{
				bHitSomething = true;
				HitCharacter = HitChar;

				if (HitChar->IsPushable())
				{
					float TimeUntilHit = step * TimePerTile;
					PushQueue.Enqueue({ HitChar, RemainingDist - step, StartDelay + TimeUntilHit });
				}
				break;
			}

			VirtualPos = NextPos;
			KnockbackPath.Add(NextTile);
		}

		if (KnockbackPath.Num() > 0)
		{
			MoveComp->NetMulticast_MoveAlongPath(KnockbackPath, false, StartDelay);
		}

		if (bHitSomething)
		{
			float ScaledRatio = (float)RemainingDist / (float)PushDistance;
			float FinalDamageRatio = CollisionDamageRatio * ScaledRatio;
			float TargetDamage = 0.f;
			float HitDamage = 0.f;

			if (HitCharacter)
			{
				if (UACStatComponent* HitStat = HitCharacter->GetStatComponent())
				{
					float BaseDamage = HitStat->GetMaxHP() * FinalDamageRatio;
					TargetDamage = BaseDamage;
					HitDamage = BaseDamage;
				}
			}
			else
			{
				if (UACStatComponent* MyStat = CurrentTarget->GetStatComponent())
				{
					TargetDamage = MyStat->GetMaxHP() * FinalDamageRatio;
				}
			}

			float DamageTime = StartDelay + (KnockbackPath.Num() * TimePerTile);

			TWeakObjectPtr<APE_CharacterBase> WeakTarget = CurrentTarget;
			TWeakObjectPtr<APE_CharacterBase> WeakHitChar = HitCharacter;
			TWeakObjectPtr<AActor> WeakInstigator = Instigator;

			FTimerDelegate DamageDel = FTimerDelegate::CreateLambda([WeakTarget, WeakHitChar, WeakInstigator, TargetDamage, HitDamage]()
				{
					if (WeakInstigator.IsValid())
					{
						if (WeakTarget.IsValid() && TargetDamage > 0.f)
						{
							UGameplayStatics::ApplyDamage(WeakTarget.Get(), TargetDamage, WeakInstigator->GetInstigatorController(), WeakInstigator.Get(), UDamageType::StaticClass());
						}
						if (WeakHitChar.IsValid() && HitDamage > 0.f)
						{
							UGameplayStatics::ApplyDamage(WeakHitChar.Get(), HitDamage, WeakInstigator->GetInstigatorController(), WeakInstigator.Get(), UDamageType::StaticClass());
						}
					}
				});

			FTimerHandle TempHandle;
			WeakTarget->GetWorldTimerManager().SetTimer(TempHandle, DamageDel, FMath::Max(0.01f, DamageTime), false);
		}
	}
}

// --- [모듈 2: 넉백 시뮬레이션부] ---
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

	TQueue<FSimulatedPush> SimQueue;
	TArray<FSimulatedPush> InitialPushes;

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
				InitialPushes.Add({ HitChar, PushDistance, PushDir });
				PushDirMap.Add(HitChar, PushDir);
			}
		}
	}

	// 핵심 수정: 밀리는 방향(PushDir)을 기준으로 가장 앞에 있는 객체부터 내림차순 정렬
	InitialPushes.Sort([&CurrentPosMap](const FSimulatedPush& A, const FSimulatedPush& B) {
		FIntPoint PosA = CurrentPosMap[A.Actor];
		FIntPoint PosB = CurrentPosMap[B.Actor];
		int32 DotA = PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y;
		int32 DotB = PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y;
		return DotA > DotB;
		});

	for (const FSimulatedPush& Task : InitialPushes)
	{
		SimQueue.Enqueue(Task);
	}

	while (!SimQueue.IsEmpty())
	{
		FSimulatedPush Task;
		SimQueue.Dequeue(Task);

		if (!Task.Actor || Task.RemainingDist <= 0 || !CurrentPosMap.Contains(Task.Actor)) continue;

		FIntPoint CurrentPos = CurrentPosMap[Task.Actor];
		FIntPoint ExpectedEndPos = CurrentPos;

		for (int32 step = 1; step <= Task.RemainingDist; ++step)
		{
			FIntPoint CheckPos = ExpectedEndPos + Task.PushDir;
			AACTile* CheckTile = GridSystem->GetTileAtPosition(CheckPos);

			if (!CheckTile || CheckTile->IsObstacle()) break;

			APE_CharacterBase* CollidedChar = nullptr;
			for (const auto& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == CheckPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				if (CollidedChar->IsPushable())
				{
					SimQueue.Enqueue({ CollidedChar, Task.RemainingDist - step, Task.PushDir });
					PushDirMap.Add(CollidedChar, Task.PushDir);
				}
				break;
			}

			ExpectedEndPos = CheckPos;
		}

		if (ExpectedEndPos != CurrentPos)
		{
			CurrentPosMap[Task.Actor] = ExpectedEndPos;
		}
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