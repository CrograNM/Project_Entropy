// Copyright CrograNM

#include "CardSystem/PE_SkillBase.h"
#include "Characters/PE_CharacterBase.h"
#include "Grid/ACTile.h"

UPE_SkillBase::UPE_SkillBase()
{
	SkillName = TEXT("Default Skill");
	APCost = 1;
	Range = 1;
	TargetType = EPESkillTargetType::Enemy;
}

UWorld* UPE_SkillBase::GetWorld() const
{
	// 스킬 객체를 소유한 Outer(보통 SkillComponent)의 월드를 반환하여, 스킬 내부에서 파티클 스폰 등을 가능하게 합니다.
	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}
	return nullptr;
}

bool UPE_SkillBase::CanExecute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	// 기본 로직: 시전자와 타겟이 존재해야 발동 가능
	if (!Caster || !TargetTile) return false;
	
	// 타겟 타입 검증 (예: 적을 타겟하는 스킬인데 빈 타일이면 거절)
	if (TargetType == EPESkillTargetType::Enemy && !TargetCharacter) return false;

	return true;
}

void UPE_SkillBase::Execute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	// 자식 클래스(C++ 또는 블루프린트)에서 이 함수를 오버라이드하여 데미지나 파티클 로직을 작성합니다.
	UE_LOG(LogTemp, Log, TEXT("[Skill] %s 가 %s 스킬을 발동했습니다!"), *Caster->GetName(), *SkillName.ToString());
}