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

	// [당구 큐 시스템] <밀려날 대상, 남은 에너지(거리)>
	struct FPushTask {
		APE_CharacterBase* ActorToPush;
		int32 RemainingDist;
	};

	TQueue<FPushTask> PushQueue;
	PushQueue.Enqueue({ Target, PushDistance });

	while (!PushQueue.IsEmpty())
	{
		FPushTask Task;
		PushQueue.Dequeue(Task);

		APE_CharacterBase* CurrentTarget = Task.ActorToPush;
		int32 RemainingDist = Task.RemainingDist;

		if (!CurrentTarget || RemainingDist <= 0 || !CurrentTarget->IsPushable()) continue;

		UACGridMovementComponent* MoveComp = CurrentTarget->GetGridMovementComponent();
		if (!MoveComp) continue;

		// 연쇄 작용을 위해 대상의 현재(또는 예약된) 출발점 추적
		FIntPoint VirtualPos = (MoveComp->GetTargetGridPosition() != FIntPoint(-999, -999))
			? MoveComp->GetTargetGridPosition() : MoveComp->GetGridPosition();

		TArray<AACTile*> KnockbackPath;

		for (int32 step = 1; step <= RemainingDist; ++step)
		{
			FIntPoint NextPos = VirtualPos + PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			// 1. 비파괴 정적 장애물 (벽, 용암, 절벽 등)
			if (!NextTile || NextTile->IsObstacle())
			{
				if (UACStatComponent* Stat = CurrentTarget->GetStatComponent())
				{
					// 내 체력 비례 데미지
					float Damage = Stat->GetMaxHP() * CollisionDamageRatio;
					UGameplayStatics::ApplyDamage(CurrentTarget, Damage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
				}
				break; // 여기서 이동 강제 중단 (Path에 추가되지 않으므로 자연스럽게 이전 칸에 정지/롤백됨)
			}

			// 2. 동적 장애물 및 캐릭터 충돌
			if (APE_CharacterBase* HitChar = GridSystem->GetCharacterAtPosition(NextPos, CurrentTarget))
			{
				if (UACStatComponent* HitStat = HitChar->GetStatComponent())
				{
					// 부딪힌 상대방의 체력 비례 데미지
					float Damage = HitStat->GetMaxHP() * CollisionDamageRatio;

					// 양쪽 모두에게 데미지 적용
					UGameplayStatics::ApplyDamage(CurrentTarget, Damage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
					UGameplayStatics::ApplyDamage(HitChar, Damage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
				}

				// 상대방이 밀려나는 객체라면 남은 에너지를 전달 (당구 로직)
				if (HitChar->IsPushable())
				{
					PushQueue.Enqueue({ HitChar, RemainingDist - step });
				}

				break; // 현재 타겟은 타격점에서 정지
			}

			// 안전한 타일이면 경로에 추가
			VirtualPos = NextPos;
			KnockbackPath.Add(NextTile);
		}

		// 최종 계산된 경로가 존재하면 회전 없이(false) 강제 이동 명령 하달
		if (KnockbackPath.Num() > 0)
		{
			MoveComp->NetMulticast_MoveAlongPath(KnockbackPath, false);
		}
	}
}