// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_AoE.h"
#include "CardSystem/PE_SkillData.h"
#include "Components/ACSkillComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

void UPE_SkillLogic_AoE::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!Instigator || !InSkillData) return;

	FIntPoint CenterPos(-999, -999);

	// 1. 목표 타일의 그리드 좌표(X, Y)를 정확히 찾아냅니다.
	if (Target)
	{
		if (UACGridMovementComponent* MoveComp = Target->FindComponentByClass<UACGridMovementComponent>())
		{
			CenterPos = MoveComp->GetGridPosition();
		}
	}
	else
	{
		// 맨 땅(타일)에 장판을 시전했을 경우 위치 기반으로 타일을 탐색합니다.
		TArray<AActor*> Tiles;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AACTile::StaticClass(), Tiles);
		for (AActor* Actor : Tiles)
		{
			if (Actor->GetActorLocation().Equals(TargetLocation, 50.f)) // 오차 허용
			{
				CenterPos = Cast<AACTile>(Actor)->GetGridPosition();
				break;
			}
		}
	}

	if (CenterPos == FIntPoint(-999, -999)) return; // 타겟 지점을 못 찾으면 폭발 취소

	// 2. 기획자가 설정한 모양(Shape) 데이터에 맞춰 타격 범위(좌표 세트)를 수학적으로 그려냅니다.
	TSet<FIntPoint> AffectedPositions;
	AffectedPositions.Add(CenterPos); // 기본 중심점 포함

	if (InSkillData->AoEShape == EPEAoEShape::Cross)
	{
		for (int32 i = 1; i <= InSkillData->AoESize; ++i)
		{
			AffectedPositions.Add(CenterPos + FIntPoint(i, 0));
			AffectedPositions.Add(CenterPos + FIntPoint(-i, 0));
			AffectedPositions.Add(CenterPos + FIntPoint(0, i));
			AffectedPositions.Add(CenterPos + FIntPoint(0, -i));
		}
	}
	else if (InSkillData->AoEShape == EPEAoEShape::Square)
	{
		for (int32 x = -InSkillData->AoESize; x <= InSkillData->AoESize; ++x)
		{
			for (int32 y = -InSkillData->AoESize; y <= InSkillData->AoESize; ++y)
			{
				AffectedPositions.Add(CenterPos + FIntPoint(x, y));
			}
		}
	}
	else if (InSkillData->AoEShape == EPEAoEShape::Ring)
	{
		for (int32 x = -InSkillData->AoESize; x <= InSkillData->AoESize; ++x)
		{
			for (int32 y = -InSkillData->AoESize; y <= InSkillData->AoESize; ++y)
			{
				if (FMath::Abs(x) == InSkillData->AoESize || FMath::Abs(y) == InSkillData->AoESize)
				{
					AffectedPositions.Add(CenterPos + FIntPoint(x, y));
				}
			}
		}
	}
	else if (InSkillData->AoEShape == EPEAoEShape::Custom)
	{
		// 개발자가 에디터에서 +버튼을 눌러 직접 좌표를 찍은 경우 (예: X:1,Y:1 대각선 공격 등)
		for (const FIntPoint& Offset : InSkillData->CustomAoEOffsets)
		{
			AffectedPositions.Add(CenterPos + Offset);
		}
	}

	// 3. 맵 위의 모든 캐릭터 중, 위에서 계산된 '타격 범위 좌표' 안에 서 있는 애들만 데미지를 줍니다.
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (UACGridMovementComponent* MoveComp = Char->GetGridMovementComponent())
			{
				if (AffectedPositions.Contains(MoveComp->GetGridPosition()))
				{
					UGameplayStatics::ApplyDamage(Char, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
				}
			}
		}
	}

	// 4. 폭발 중앙 시각 효과
	if (UACSkillComponent* SkillComp = Instigator->FindComponentByClass<UACSkillComponent>())
	{
		SkillComp->NetMulticast_PlayHitVisuals(InSkillData, TargetLocation);
	}
}