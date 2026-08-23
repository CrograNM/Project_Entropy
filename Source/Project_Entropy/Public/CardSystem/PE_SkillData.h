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

// 스킬의 순수한 수치, 속성, 연출 데이터를 정의
UCLASS(BlueprintType)
class PROJECT_ENTROPY_API UPE_SkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Logic")
	EPESkillTargetType TargetType;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Skill|Effects")
	TArray<TObjectPtr<UPE_SkillEffectModule>> EffectModules;

	// ---- 스킬 액터 (투사체, 장판 등) 및 물리/동작 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Action Actor")
	TSubclassOf<APE_SkillActionActor> SkillActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Action Actor")
	bool bDestroyOnHit = true;		// true: 투사체(맞으면 파괴), false: 관통/장판(계속 유지)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Action Actor")
	float ProjectileSpeed = 800.f;	// 0이면 장판처럼 제자리에 고정됨

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Action Actor")
	float ProjectileGravity = 0.f;	// 포물선 곡사 여부 (0이면 직사)

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

	// --- [광역 공격(AoE) 커스텀 데이터] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effect (AoE)")
	EPEAoEShape AoEShape = EPEAoEShape::None; 

	// 십자 모양의 가지 길이, 정사각형의 반경 등
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effect (AoE)", meta = (EditCondition = "AoEShape != EPEAoEShape::None && AoEShape != EPEAoEShape::Custom"))
	int32 AoESize = 1; 

	// Custom 선택 시 에디터에서 직접 칠할 수 있는 타일 오프셋 배열 (0,0 은 타겟 중심점)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effect (AoE)", meta = (EditCondition = "AoEShape == EPEAoEShape::Custom"))
	TArray<FIntPoint> CustomAoEOffsets;

	// 관통 스킬 / 레이저 폭 (0.0이면 1칸짜리 선)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effect (AoE)", meta = (EditCondition = "AoEShape == EPEAoEShape::Line"))
	float LineWidth = 0.0f; 

	// 커스텀 오프셋 4방향 자동 회전 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Effect (AoE)", meta = (EditCondition = "AoEShape == EPEAoEShape::Custom"))
	bool bRotateToTarget = true; 

	// 시전자 위치를 알아야 방향(Direction)을 구할 수 있음
	TSet<FIntPoint> GetAffectedGridPositions(FIntPoint CasterPos, FIntPoint TargetPos) const;

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

	// 장판/즉발 스킬이 생성된 후 폭발(데미지/밀치기)할 때까지 대기하는 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 2 (Action)")
	float ExplosionDelay = 0.f;

	// ---- 3단계: 적중/적용 (Hit / Apply)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 3 (Hit)")
	TObjectPtr<UNiagaraSystem> HitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phase 3 (Hit)")
	TObjectPtr<USoundBase> HitSFX;
};