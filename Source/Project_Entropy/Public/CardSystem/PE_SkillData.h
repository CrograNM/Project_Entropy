// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_SkillData.generated.h"

class UPE_SkillLogicBase;
class APE_SkillActionActor;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;

// 스킬의 순수한 수치, 속성, 연출 데이터를 정의
UCLASS(BlueprintType)
class PROJECT_ENTROPY_API UPE_SkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Logic")
	TSubclassOf<UPE_SkillLogicBase> LogicClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Logic")
	EPESkillTargetType TargetType;

	// (필요하면) 중간 과정에 스폰되어 판정 로직에 사용될 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Logic")
	TSubclassOf<APE_SkillActionActor> SkillActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Element")
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	float BaseHeal = 0.f;

	// AP 비용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	int32 BaseAPCost = 1;

	// 사거리 (타일 수)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	int32 BaseRange = 1;

	// ---- 1단계: 시전 (Cast) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 1 (Cast)")
	TObjectPtr<UAnimMontage> CastAnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 1 (Cast)")
	FName CastAnimSectionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 1 (Cast)")
	TObjectPtr<UNiagaraSystem> CastVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 1 (Cast)")
	TObjectPtr<USoundBase> CastSFX;

	// ---- 2단계: 중간 동작 (Action) - 투사체 비행, 장판기 지속 이펙트 등
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 2 (Action)")
	TObjectPtr<UNiagaraSystem> ActionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 2 (Action)")
	TObjectPtr<USoundBase> ActionSFX;

	// ---- 3단계: 적중/적용 (Hit / Apply)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 3 (Hit)")
	TObjectPtr<UNiagaraSystem> HitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 3 (Hit)")
	TObjectPtr<USoundBase> HitSFX;
};