// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_Push.h"
#include "CardSystem/PE_SkillData.h" // [추가됨] 데이터 에셋 접근
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

void UPE_SkillLogic_Push::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	Super::ApplySkillEffect_Implementation(Instigator, Target, TargetLocation, InSkillData, CalculatedDamage);

	// 기획자가 스킬 데이터에서 밀치기를 껐거나 거리가 0이면 실행 취소
	if (!InSkillData || !InSkillData->bDoesPush || InSkillData->PushDistance <= 0) return;

	APE_CharacterBase* TargetChar = Cast<APE_CharacterBase>(Target);
	APE_CharacterBase* InstigatorChar = Cast<APE_CharacterBase>(Instigator);

	if (TargetChar && InstigatorChar)
	{
		UACGridMovementComponent* TargetMove = TargetChar->GetGridMovementComponent();
		UACGridMovementComponent* InstMove = InstigatorChar->GetGridMovementComponent();

		if (TargetMove && InstMove)
		{
			FIntPoint InstPos = InstMove->GetGridPosition();
			FIntPoint TargetPos = TargetMove->GetGridPosition();

			FIntPoint PushDir(
				FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
				FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
			);

			if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1)
			{
				PushDir.Y = 0;
			}

			// [수정됨] 하드코딩 변수 대신 기획자가 데이터 에셋에 세팅한 거리(PushDistance)를 사용
			FIntPoint NextPos = TargetPos + (PushDir * InSkillData->PushDistance);

			if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
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