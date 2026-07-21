// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillBase.h"
#include "PE_Skill_MeleeAttack.generated.h"

UCLASS()
class PROJECT_ENTROPY_API UPE_Skill_MeleeAttack : public UPE_SkillBase
{
	GENERATED_BODY()

public:
	UPE_Skill_MeleeAttack();

	// 실제 데미지를 주는 로직 오버라이드
	virtual void Execute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Damage")
	float DamageAmount;
};