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

	// 시전자 -> 타겟 방향 도출 (-1, 0, 1) (당구처럼 이 방향 에너지가 끝까지 유지됨)
	FIntPoint PushDir(
		FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
		FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
	);
	if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

	// [당구 큐 시스템 구조체] <밀릴 대상, 남은 에너지, 언제 출발할지(Delay)>
	struct FPushTask {
		APE_CharacterBase* ActorToPush;
		int32 RemainingDist;
		float StartTimeDelay;
	};

	TQueue<FPushTask> PushQueue;
	PushQueue.Enqueue({ Target, PushDistance, 0.f }); // 첫 타겟은 딜레이 0초로 즉시 출발

	// 1칸 이동하는 데 걸리는 시간 추정 (거리 100 / 속도)
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

		// 충돌 발생 여부 체크용 변수
		bool bHitSomething = false;
		APE_CharacterBase* HitCharacter = nullptr;

		for (int32 step = 1; step <= RemainingDist; ++step)
		{
			FIntPoint NextPos = VirtualPos + PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			// 1. 비파괴 정적 장애물 (벽, 용암 등) 또는 맵 끝
			if (!NextTile || NextTile->IsObstacle())
			{
				bHitSomething = true;
				break; // 이동 중단
			}

			// 2. 동적 장애물 및 캐릭터 충돌
			if (APE_CharacterBase* HitChar = GridSystem->GetCharacterAtPosition(NextPos, CurrentTarget))
			{
				bHitSomething = true;
				HitCharacter = HitChar;

				// 부딪힌 상대가 밀릴 수 있다면 당구공 큐에 예약 (내가 부딪히는 시간 = 상대방이 출발할 시간)
				if (HitChar->IsPushable())
				{
					float TimeUntilHit = step * TimePerTile;
					PushQueue.Enqueue({ HitChar, RemainingDist - step, StartDelay + TimeUntilHit });
				}
				break; // 이동 중단
			}

			VirtualPos = NextPos;
			KnockbackPath.Add(NextTile);
		}

		// --- 이동 명령 전송 ---
		if (KnockbackPath.Num() > 0)
		{
			// 회전 금지(false), StartDelay 적용
			MoveComp->NetMulticast_MoveAlongPath(KnockbackPath, false, StartDelay);
		}

		// --- 지연 데미지 예약 (충돌했을 때만) ---
		if (bHitSomething)
		{
			// 남은 에너지 비례 계수 계산 (예: 3칸 중 3칸 남기고 박으면 1.0, 1칸 남기고 박으면 0.33)
			float ScaledRatio = (float)RemainingDist / (float)PushDistance;
			float FinalDamageRatio = CollisionDamageRatio * ScaledRatio;

			float TargetDamage = 0.f;
			float HitDamage = 0.f;

			if (HitCharacter)
			{
				// 유저 요청: 캐릭터->장애물(동적) 시 부딪힌 장애물의 최대 체력에 따라 데미지 결정
				if (UACStatComponent* HitStat = HitCharacter->GetStatComponent())
				{
					float BaseDamage = HitStat->GetMaxHP() * FinalDamageRatio;
					TargetDamage = BaseDamage;
					HitDamage = BaseDamage;
				}
			}
			else
			{
				// 유저 요청: 비파괴장애물(벽)에 부딪힌 경우 밀려진 객체 본인의 최대 체력 비례
				if (UACStatComponent* MyStat = CurrentTarget->GetStatComponent())
				{
					TargetDamage = MyStat->GetMaxHP() * FinalDamageRatio;
				}
			}

			// 부딪히는 순간의 시간 계산
			float DamageTime = StartDelay + (KnockbackPath.Num() * TimePerTile);

			// 안전한 지연 데미지 적용을 위해 WeakPtr 캡처
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
			// 0초에 부딪히더라도 타이머가 실행되도록 최소치 보장
			Target->GetWorldTimerManager().SetTimer(TempHandle, DamageDel, FMath::Max(0.01f, DamageTime), false);
		}
	}
}

// --- [시각화를 위한 당구 연쇄 로직 모듈화] ---
TArray<FPushSimulationResult> UPE_SkillEffect_Push::SimulatePush(AACGridSystem* GridSystem, FIntPoint InstigatorPos, const TSet<FIntPoint>& AffectedGridPositions) const
{
	TArray<FPushSimulationResult> Results;
	if (!GridSystem || PushDistance <= 0) return Results;

	TMap<APE_CharacterBase*, FIntPoint> InitialPosMap;
	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	TMap<APE_CharacterBase*, FIntPoint> PushDirMap; // 밀리는 방향 기록

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
				SimQueue.Enqueue({ HitChar, PushDistance, PushDir });
				PushDirMap.Add(HitChar, PushDir);
			}
		}
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
					// 연쇄 대상의 방향 기록 갱신 (이미 맵에 있어도 덮어쓰거나 추가)
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

	// 에너지를 받은 모든 대상(이동 유무 상관없이) 반환 구조체 구성
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