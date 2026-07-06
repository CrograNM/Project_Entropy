// Copyright CrograNM

#include "Skills/PE_Skill_MeleeAttack.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACStatComponent.h"
#include "Kismet/GameplayStatics.h"

UPE_Skill_MeleeAttack::UPE_Skill_MeleeAttack()
{
	SkillName = TEXT("기본 타격 (Melee)");
	APCost = 1;
	Range = 1;
	TargetType = EPESkillTargetType::Enemy;
	DamageAmount = 10.f;
}

void UPE_Skill_MeleeAttack::Execute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	Super::Execute_Implementation(Caster, TargetTile, TargetCharacter);

	if (TargetCharacter && TargetCharacter->GetStatComponent())
	{
		// 타겟의 스탯 컴포넌트에 데미지 적용
		TargetCharacter->GetStatComponent()->ApplyDamage(DamageAmount);

		// TODO: 피격 이펙트 스폰, 근거리 타격 애니메이션 재생 등을 여기서 구현합니다.
		UE_LOG(LogTemp, Warning, TEXT("[Skill] %s 가 %s 에게 %f 의 피해를 입혔습니다!"), *Caster->GetName(), *TargetCharacter->GetName(), DamageAmount);
	}
}