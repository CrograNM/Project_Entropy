// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_Push.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

void UPE_SkillLogic_Push::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	// 1. 부모 로직을 호출하여 기본 데미지와 이펙트를 먼저 터트립니다.
	Super::ApplySkillEffect_Implementation(Instigator, Target, TargetLocation, InSkillData, CalculatedDamage);

	// 2. 밀어낼 대상이 생물체라면 넉백 연산을 시작합니다.
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

			// 방향 벡터 계산 (시전자 -> 타겟 방향) 및 Clamp를 통한 -1, 0, 1 정규화
			FIntPoint PushDir(
				FMath::Clamp(TargetPos.X - InstPos.X, -1, 1),
				FMath::Clamp(TargetPos.Y - InstPos.Y, -1, 1)
			);

			// 대각선 넉백 방지 (상하좌우 십자 방향으로만 밀어내기 위함)
			if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1)
			{
				PushDir.Y = 0; // 임의로 X축 방향을 우선시
			}

			FIntPoint NextPos = TargetPos + (PushDir * PushDistance);

			// 도착할 타일이 안전한지(장애물이 아닌지) 검사 후 강제 이동
			if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
			{
				AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);
				if (NextTile && !NextTile->IsObstacle())
				{
					// 강제 넉백 궤적 생성 및 전송
					TArray<AACTile*> KnockbackPath;
					KnockbackPath.Add(NextTile);
					TargetMove->NetMulticast_MoveAlongPath(KnockbackPath);
				}
			}
		}
	}
}