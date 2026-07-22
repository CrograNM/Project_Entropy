// Copyright CrograNM

#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_SkillData.h"

UPE_CardInstance::UPE_CardInstance()
{
}

void UPE_CardInstance::Initialize(UPE_CardData* InBaseData)
{
	if (InBaseData)
	{
		BaseCardData = InBaseData;

		// 초기화 시 모디파이어(증감치) 0으로 세팅
		CostModifier = 0;
		DamageModifier = 0.f;
		HealModifier = 0.f;

		// 추후 디스크(세이브 데이터)에서 이 카드의 마석 정보를 불러와 
		// 여기서 Modifier 수치들을 덮어씌워주는 로직 추가
	}
}

int32 UPE_CardInstance::GetCalculatedAPCost() const
{
	if (!BaseCardData || !BaseCardData->SkillDataToCast) return 0;

	// 원본 데이터 + 변동 수치
	int32 FinalCost = BaseCardData->SkillDataToCast->BaseAPCost + CostModifier;

	// 코스트는 0 밑으로 떨어지지 않도록 방어 기제 적용
	return FMath::Max(0, FinalCost);
}

float UPE_CardInstance::GetCalculatedDamage() const
{
	if (!BaseCardData || !BaseCardData->SkillDataToCast) return 0.f;

	float FinalDamage = BaseCardData->SkillDataToCast->BaseDamage + DamageModifier;
	return FMath::Max(0.f, FinalDamage);
}

float UPE_CardInstance::GetCalculatedHeal() const
{
	if (!BaseCardData || !BaseCardData->SkillDataToCast) return 0.f;

	float FinalHeal = BaseCardData->SkillDataToCast->BaseHeal + HealModifier;
	return FMath::Max(0.f, FinalHeal);
}