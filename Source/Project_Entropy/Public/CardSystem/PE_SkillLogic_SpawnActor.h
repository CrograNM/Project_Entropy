// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "PE_SkillLogic_SpawnActor.generated.h"

class APE_SkillActionActor;

/**
 * [행위 로직]: 투사체를 생성하여 날려보내고, 도착 시 데미지/이펙트를 적용합니다.
 */
UCLASS()
class PROJECT_ENTROPY_API UPE_SkillLogic_SpawnActor : public UPE_SkillLogicBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
	virtual void ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
};