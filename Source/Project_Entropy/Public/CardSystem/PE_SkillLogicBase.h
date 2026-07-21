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
	 * 스킬 실행 함수 (C++에서 호출하고 BP에서 세부 연출 및 데미지 처리를 구현)
	 * @param Instigator 스킬을 사용한 주체 (플레이어 또는 몬스터)
	 * @param Target 액터 대상 (Snap 방식일 경우)
	 * @param TargetLocation 타일 좌표 (Tile 방식일 경우)
	 * @param InSkillData 이 스킬을 실행하게 만든 원본 스킬 데이터 (데미지, 이펙트 참조용)
	 * @param CalculatedDamage 마석 등이 연산 완료된 최종 데미지
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill|Execution")
	void ExecuteSkill(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage);
};