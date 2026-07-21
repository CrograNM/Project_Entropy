// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "PE_SkillLogic_DirectDamage.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_ENTROPY_API UPE_SkillLogic_DirectDamage : public UPE_SkillLogicBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
};
