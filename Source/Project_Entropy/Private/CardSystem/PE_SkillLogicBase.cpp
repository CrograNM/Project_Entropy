// Copyright CrograNM

#include "CardSystem/PE_SkillLogicBase.h"

UWorld* UPE_SkillLogicBase::GetWorld() const
{
	// 자신을 소유한 Outer(보통 생성한 캐릭터나 컨트롤러)를 통해 월드를 가져옵니다.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}

	return nullptr;
}

void UPE_SkillLogicBase::ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
}

void UPE_SkillLogicBase::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
}