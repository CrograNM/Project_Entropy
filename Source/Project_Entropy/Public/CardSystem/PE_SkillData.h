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

/*
	개별 타격(투사체, 장판, 근접 베기 등) 1회의 모든 설정을 담는 페이즈 구조체입니다.

	한 페이즈 안에서 대기 시간 3개가 아래 순서로 이어집니다.
	카테고리도 이 순서대로 배치되어 있으니 위에서 아래로 읽으면 실행 흐름이 됩니다.

	① TriggerTime      시전 시작 -> 페이즈 발생 (투사체가 있으면 목표까지 비행)
	② ExplosionDelay   도달 -> 폭발 연출
	③ HitDelay         폭발 -> 피격 연출 및 EffectModules 적용
*/
USTRUCT(BlueprintType)
struct FPESkillHitPhase
{
	GENERATED_BODY()

	// ----- ① 기본 -----

	/*
		스킬 시전이 시작된 뒤 이 페이즈가 발생하기까지의 대기 시간입니다.
		연타 스킬은 페이즈마다 값을 늘려 타격 간격을 만듭니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Basic", meta = (DisplayName = "[기본] ① 발동 대기", ClampMin = "0.0", Units = "s"))
	float TriggerTime = 0.f;

	/** 스킬의 기본 데미지에 곱해지는 배율입니다. 0.5면 50 % 만 들어갑니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Basic", meta = (DisplayName = "[기본] 데미지 배율", ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	// ----- 범위 -----

	/** 이 페이즈가 영향을 주는 칸의 모양입니다. 아래 범위 설정들은 선택한 모양에 해당하는 것만 표시됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (DisplayName = "[범위] 모양"))
	EPEAoEShape AoEShape = EPEAoEShape::None;

	/*
		중심에서 몇 칸까지 뻗는지를 나타내는 반지름입니다. 지름이 아닙니다.
		예: Square에 1을 넣으면 3x3, Cross에 2를 넣으면 길이 5의 십자가 됩니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (DisplayName = "[범위] 크기 (중심에서의 반지름)", EditCondition = "AoEShape != EPEAoEShape::None && AoEShape != EPEAoEShape::Custom && AoEShape != EPEAoEShape::Line", EditConditionHides, ClampMin = "0"))
	int32 AoESize = 1;

	/*
		직선 중심에서 좌우로 번지는 폭입니다. 0이면 1칸 폭의 선이 됩니다.
		직선의 길이는 이 값이 아니라 스킬 루트의 BaseRange가 결정합니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (DisplayName = "[범위] 직선 폭", EditCondition = "AoEShape == EPEAoEShape::Line", EditConditionHides, ClampMin = "0.0"))
	float LineWidth = 0.0f;

	/*
		조준한 타일을 원점(0,0)으로 삼는 상대 좌표 목록입니다.
		bRotateToTarget이 켜져 있으면 시전자가 바라보는 4방향에 맞춰 통째로 회전합니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (DisplayName = "[범위] 커스텀 오프셋", EditCondition = "AoEShape == EPEAoEShape::Custom", EditConditionHides))
	TArray<FIntPoint> CustomAoEOffsets;

	/*
		커스텀 모양을 조준 방향으로 회전시킬지 여부입니다.
		회전은 90도 단위로 스냅되며(정확한 대각선은 항상 수평) 밀치기 방향과 같은 규칙을 씁니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|AoE", meta = (DisplayName = "[범위] 조준 방향으로 회전", EditCondition = "AoEShape == EPEAoEShape::Custom", EditConditionHides))
	bool bRotateToTarget = true;

	// ----- 투사체 -----

	/*
		날아갈 투사체 액터입니다. 비워두면 투사체 없이 조준 지점에서 즉시 터집니다.
		아래 투사체 설정 일부는 이 값이 채워져 있을 때만 표시됩니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 액터 클래스 (비우면 즉발)"))
	TSubclassOf<APE_SkillActionActor> SkillActorClass;

	/*
		투사체의 비행 속도입니다. 0이면 비행 없이 즉발로 처리되며 조준선도 직선 한 구간으로 그려집니다.
		액터를 지정하지 않아도 이 값은 조준 궤적 계산에 그대로 쓰입니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 속도 (0이면 즉발)", ClampMin = "0.0"))
	float ProjectileSpeed = 800.f;

	/*
		이름과 달리 중력 가속도가 아니라, 궤적 중간 지점에서 추가로 올라가는 높이(cm)입니다.
		0이면 직선으로 날아가고, 값을 키우면 포물선이 높아집니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 포물선 정점 높이", EditCondition = "ProjectileSpeed > 0.0", EditConditionHides, ClampMin = "0.0"))
	float ProjectileGravity = 0.f;

	/*
		첫 대상에 부딪히면 투사체를 소멸시킬지 여부입니다.
		끄면 관통 투사체가 되어, 스치는 대상을 순서대로 한 명씩 타격하며 끝까지 날아갑니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 충돌 시 소멸 (끄면 관통)", EditCondition = "SkillActorClass != nullptr", EditConditionHides))
	bool bDestroyOnHit = true;

	/** 투사체가 날아가는 동안 붙어서 재생되는 나이아가라 이펙트입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 비행 VFX", EditCondition = "SkillActorClass != nullptr", EditConditionHides))
	TObjectPtr<UNiagaraSystem> ActionVFX;

	/** 투사체가 날아가는 동안 붙어서 재생되는 사운드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Action", meta = (DisplayName = "[투사체] 비행 SFX", EditCondition = "SkillActorClass != nullptr", EditConditionHides))
	TObjectPtr<USoundBase> ActionSFX;

	// ----- ② 폭발 -----

	/*
		투사체가 목표에 도달한(또는 즉발 페이즈가 발생한) 뒤 폭발 연출이 나오기까지의 대기 시간입니다.
		투사체가 없는 즉발 스킬에도 그대로 적용됩니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion", meta = (DisplayName = "[폭발] ② 폭발 대기", ClampMin = "0.0", Units = "s"))
	float ExplosionDelay = 0.f;

	/** 조준 지점에 스폰되는 폭발 나이아가라 이펙트입니다. AoE 크기에 맞춰 스케일과 회전이 적용됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion", meta = (DisplayName = "[폭발] VFX"))
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	/** 조준 지점에서 재생되는 폭발 사운드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Explosion", meta = (DisplayName = "[폭발] SFX"))
	TObjectPtr<USoundBase> ExplosionSFX;

	// ----- ③ 피격 -----

	/*
		폭발 연출이 시작된 뒤 실제 피격 판정과 EffectModules 적용까지의 대기 시간입니다.
		파도가 덮치는 타이밍처럼 연출과 판정을 맞춰야 할 때 씁니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Hit", meta = (DisplayName = "[피격] ③ 피격 대기", ClampMin = "0.0", Units = "s"))
	float HitDelay = 0.f;

	/** 타격된 대상 위치마다 스폰되는 피격 나이아가라 이펙트입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Hit", meta = (DisplayName = "[피격] VFX"))
	TObjectPtr<UNiagaraSystem> HitVFX;

	/** 타격된 대상 위치에서 재생되는 피격 사운드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase|Hit", meta = (DisplayName = "[피격] SFX"))
	TObjectPtr<USoundBase> HitSFX;

	// ----- 효과 -----

	/** 페이즈마다 독립적인 이펙트 모듈 (밀치기 방향, 상태이상 등 독립 세팅 가능) */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Phase|Effects", meta = (DisplayName = "[효과] 이펙트 모듈"))
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
	/** 로그와 액션 큐 UI에 표시되는 식별자입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	FName SkillID;

	/** 무엇을 지정해 시전하는 스킬인지 결정합니다. 타일 지정 / 대상 스냅 / 자신 / 전체. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	EPESkillTargetType TargetType;

	/** 스킬의 원소 속성입니다. 카드 외형 테마와 이펙트 매핑에 쓰입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Info")
	FGameplayTag ElementTag;

	/** 페이즈의 DamageMultiplier가 곱해지기 전의 기준 데미지입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats", meta = (ClampMin = "0.0"))
	float BaseDamage = 0.f;

	/** 기준 회복량입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats", meta = (ClampMin = "0.0"))
	float BaseHeal = 0.f;

	/** 시전에 소모되는 행동력입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats", meta = (ClampMin = "0"))
	int32 BaseAPCost = 1;

	/*
		시전 가능한 최대 거리(칸)입니다.
		주의: Line 모양 AoE에서는 이 값이 직선의 길이로도 함께 쓰입니다.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Stats", meta = (ClampMin = "0"))
	int32 BaseRange = 1;

	/** 이 스킬을 구성하는 타격 페이즈 목록입니다. 배열 순서가 아니라 각 페이즈의 발동 대기 시간이 실행 순서를 결정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Phases")
	TArray<FPESkillHitPhase> HitPhases;

	/** 시전자가 재생할 몽타주입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<UAnimMontage> CastAnimMontage;

	/** 몽타주 안에서 재생할 섹션 이름입니다. 비워두면 처음부터 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast", meta = (EditCondition = "CastAnimMontage != nullptr", EditConditionHides))
	FName CastAnimSectionName;

	/** 시전 순간 시전자 위치에 스폰되는 나이아가라 이펙트입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** 시전 순간 재생되는 사운드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cast")
	TObjectPtr<USoundBase> CastSFX;
};
