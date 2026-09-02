// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_SkillData.generated.h"

class UPE_SkillEffectModule;
class APE_SkillActionActor;
class UNiagaraSystem;
class USoundBase;
class UAnimMontage;

// [추가 핵심] 개별 타격(투사체, 장판, 근접 베기 등)의 모든 설정을 담는 페이즈 구조체
USTRUCT(BlueprintType)
struct FPESkillHitPhase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Timing")
	float TriggerTime = 0.f; // 스킬 시전 시작 후 해당 타격/투사체가 발생할 때까지의 대기 시간

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Stats")
	float DamageMultiplier = 1.0f; // 기본 데미지 배율 (예: 0.5 = 50% 데미지)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE")
	EPEAoEShape AoEShape = EPEAoEShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (EditCondition = "AoEShape != EPEAoEShape::None && AoEShape != EPEAoEShape::Custom && AoEShape != EPEAoEShape::Line"))
	int32 AoESize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (EditCondition = "AoEShape == EPEAoEShape::Custom"))
	TArray<FIntPoint> CustomAoEOffsets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (EditCondition = "AoEShape == EPEAoEShape::Line"))
	float LineWidth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (EditCondition = "AoEShape == EPEAoEShape::Custom"))
	bool bRotateToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	TSubclassOf<APE_SkillActionActor> SkillActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	bool bDestroyOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	float ProjectileSpeed = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	float ProjectileGravity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	TObjectPtr<UNiagaraSystem> ActionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	TObjectPtr<USoundBase> ActionSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action")
	float ExplosionDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion")
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion")
	TObjectPtr<USoundBase> ExplosionSFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion")
	float HitDelay = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Hit")
	TObjectPtr<UNiagaraSystem> HitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Hit")
	TObjectPtr<USoundBase> HitSFX;

	// 페이즈마다 독립적인 이펙트 모듈 (밀치기 방향, 상태이상 등 독립 세팅 가능)
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Phase|Effects")
	TArray<TObjectPtr<UPE_SkillEffectModule>> EffectModules;

	// 수학적 연산 함수 (BaseRange는 스킬 루트에서 받아옴)
	TSet<FIntPoint> GetAffectedGridPositions(FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange) const;
	void GetAoEBoundsAndRotation(FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, FVector2D& OutSize, float& OutRadius, FRotator& OutRotation) const;
};

UCLASS(BlueprintType)
class PROJECT_ENTROPY_API UPE_SkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Logic")
	EPESkillTargetType TargetType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Element")
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	float BaseHeal = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	int32 BaseAPCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats")
	int32 BaseRange = 1;

	// ---- 스킬 타격 페이즈 배열
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phases")
	TArray<FPESkillHitPhase> HitPhases;

	// ---- 스킬 시전 애니메이션 (루트 설정) 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<UAnimMontage> CastAnimMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	FName CastAnimSectionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<UNiagaraSystem> CastVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<USoundBase> CastSFX;
};