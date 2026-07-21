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
	// 이 함수는 BP_Skill_Projectile 등 자식 블루프린트에서 오버라이드하여 사용합니다.
	// 순수 가상함수로 두지 않고 기본 구현을 비워둡니다.
}