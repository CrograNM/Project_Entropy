// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillLogic_SpawnActor.h"
#include "PE_SkillLogic_AoE.generated.h"

UCLASS()
class PROJECT_ENTROPY_API UPE_SkillLogic_AoE : public UPE_SkillLogic_SpawnActor
{
	GENERATED_BODY()

public:
	virtual void ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;

protected:
	// 타일 간격(TileSpacing)이 100.f일 때, 주변 8칸(대각선 포함)을 포함할 수 있는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Skill|AoE")
	float SplashRadius = 150.f;
};