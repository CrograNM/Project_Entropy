// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACSkillComponent.generated.h"

class UPE_SkillBase;
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

	/** 현재 장착된 스킬 목록 반환 */
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	TArray<UPE_SkillBase*> GetActiveSkills() const { return ActiveSkills; }

protected:
	virtual void BeginPlay() override;

	/** 게임 시작 시 기본으로 장착할 스킬 클래스 목록 (에디터에서 세팅) */
	UPROPERTY(EditAnywhere, Category = "Skill System")
	TArray<TSubclassOf<UPE_SkillBase>> DefaultSkillClasses;

private:
	/** 인스턴스화되어 메모리에 올라간 실제 스킬 객체들 */
	UPROPERTY()
	TArray<TObjectPtr<UPE_SkillBase>> ActiveSkills;

	/** 시전자(소유자)의 스탯 컴포넌트 캐싱 (AP 통제용) */
	UPROPERTY()
	TObjectPtr<UACStatComponent> OwnerStatComponent;
};