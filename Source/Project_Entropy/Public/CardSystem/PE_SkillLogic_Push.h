// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillLogic_SpawnActor.h"
#include "PE_SkillLogic_Push.generated.h"

UCLASS()
class PROJECT_ENTROPY_API UPE_SkillLogic_Push : public UPE_SkillLogic_SpawnActor
{
	GENERATED_BODY()

public:
	virtual void ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Push")
	int32 PushDistance = 1; // 뒤로 몇 칸 밀어낼 것인가?
};