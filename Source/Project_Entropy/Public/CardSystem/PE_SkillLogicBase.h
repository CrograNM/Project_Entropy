// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PE_SkillLogicBase.generated.h"

class UPE_SkillData;

/**
 * 모든 스킬 로직(투사체, 광역, 버프 등)의 부모 클래스.
 * Blueprintable로 선언하여 BP_Skill_Projectile과 같이 BP로 상속받아 시각적 로직을 구현 가능.
 */
UCLASS(Blueprintable, Abstract, BlueprintType)
class PROJECT_ENTROPY_API UPE_SkillLogicBase : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

	/**
	 * 1단계 [발동]: 애니메이션 재생, 투사체 스폰 등을 처리합니다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteSkill(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage);

	/**
	 * 2단계 [적중]: 실제 데미지, 힐, 버프 등을 적용합니다. (모션 종료나 투사체 도착 시 호출됨)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|Execution")
	void ApplySkillEffect(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage);
};