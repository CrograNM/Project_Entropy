// Copyright CrograNM

#include "CardSystem/PE_SkillEffectModule.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

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
	if (Target && Instigator && PushDistance > 0)
	{
		UACGridMovementComponent* TargetMove = Target->GetGridMovementComponent();
		UACGridMovementComponent* InstMove = Cast<APE_CharacterBase>(Instigator)->GetGridMovementComponent();

		if (TargetMove && InstMove)
		{
			FIntPoint InstPos = InstMove->GetGridPosition();
			FIntPoint TargetPos = TargetMove->GetGridPosition();

			// 시전자 -> 타겟 방향 도출 (-1, 0, 1)
			FIntPoint PushDir(
				FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
				FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
			);

			// 대각선 방지 (상하좌우 우선)
			if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;

			FIntPoint NextPos = TargetPos + (PushDir * PushDistance);

			// 장애물이 아닌 타일로 강제 이동 명령
			if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(Target, AACGridSystem::StaticClass())))
			{
				AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);
				if (NextTile && !NextTile->IsObstacle())
				{
					TArray<AACTile*> KnockbackPath;
					KnockbackPath.Add(NextTile);
					TargetMove->NetMulticast_MoveAlongPath(KnockbackPath);
				}
			}
		}
	}
}