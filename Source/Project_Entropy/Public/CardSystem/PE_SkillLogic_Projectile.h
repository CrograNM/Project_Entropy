// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "PE_SkillLogic_Projectile.generated.h"

class APE_ProjectileBase;

/**
 * [행위 로직]: 투사체를 생성하여 날려보내고, 도착 시 데미지/이펙트를 적용합니다.
 */
UCLASS()
class PROJECT_ENTROPY_API UPE_SkillLogic_Projectile : public UPE_SkillLogicBase
{
	GENERATED_BODY()

public:
	// 1단계: 투사체 발사
	virtual void ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;

	// 2단계: 도착 후 피격
	virtual void ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;

protected:
	// 날려보낼 투사체 클래스 (블루프린트 자식 클래스에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	TSubclassOf<APE_ProjectileBase> ProjectileClass;
};