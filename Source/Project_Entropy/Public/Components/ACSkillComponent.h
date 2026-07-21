// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACSkillComponent.generated.h"

class UPE_SkillData;
class UPE_SkillLogicBase;
class APE_CharacterBase;
class AACTile;
class UACStatComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_ENTROPY_API UACSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UACSkillComponent();

	/** 특정 스킬을 발동 시도합니다. (AP 체크 및 결제 포함) */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	bool TryExecuteSkill(int32 SkillIndex, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);

	/** [카드 시스템용] 카드 인스턴스가 연산된 최종 데미지와 함께 스킬 데이터를 직접 주입하여 발동 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	bool TryExecuteSkillByData(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage);

	/** 현재 장착된 스킬 목록 반환 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	TArray<UPE_SkillData*> GetActiveSkills() const { return ActiveSkills; }

protected:
	virtual void BeginPlay() override;

	/** 게임 시작 시 기본으로 장착할 스킬 데이터 (주로 몬스터의 패턴용 세팅) */
	UPROPERTY(EditAnywhere, Category = "Skill System")
	TArray<TObjectPtr<UPE_SkillData>> DefaultSkills;

private:
	/** 런타임에 소유자가 사용할 수 있는 스킬 데이터들 (데이터 에셋의 참조만 가짐) */
	UPROPERTY()
	TArray<TObjectPtr<UPE_SkillData>> ActiveSkills;

	/** 시전자(소유자)의 스탯 컴포넌트 캐싱 (AP 통제용) */
	UPROPERTY()
	TObjectPtr<UACStatComponent> OwnerStatComponent;
};