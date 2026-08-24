// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PE_SkillEffectModule.generated.h"

class UPE_SkillData;
class APE_CharacterBase;
class AACGridSystem;

/**
 * 스킬 조립을 위한 기본 효과 모듈 뼈대 (GAS의 GameplayEffect 역할)
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable)
class PROJECT_ENTROPY_API UPE_SkillEffectModule : public UObject
{
	GENERATED_BODY()

public:
	// 다중 타겟(Targets) 그룹 전체를 받도록 구조 변경
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) PURE_VIRTUAL(UPE_SkillEffectModule::ApplyEffects, );
};

// --- [모듈 1] 기본 데미지 적용 모듈 ---
UCLASS(DisplayName = "Effect: Damage")
class PROJECT_ENTROPY_API UPE_SkillEffect_Damage : public UPE_SkillEffectModule
{
	GENERATED_BODY()

public:
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
};

// --- [밀치기 타입 정의] ---
UENUM(BlueprintType)
enum class EPEPushType : uint8
{
	Radial		UMETA(DisplayName = "방사형 (Radial - 폭발)"),
	Directional UMETA(DisplayName = "지향성 (Directional - 파도/바람)")
};

// --- [밀치기 시뮬레이션 결과를 담을 구조체] ---
USTRUCT(BlueprintType)
struct FPushSimulationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APE_CharacterBase* TargetActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint StartPos = FIntPoint(-999, -999);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint EndPos = FIntPoint(-999, -999);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint PushDir = FIntPoint::ZeroValue;
};

// --- [모듈 2] 넉백(밀치기) 적용 모듈 ---
UCLASS(DisplayName = "Effect: Push (Knockback)")
class PROJECT_ENTROPY_API UPE_SkillEffect_Push : public UPE_SkillEffectModule
{
	GENERATED_BODY()

public:
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
	int32 GetPushDistance() const { return PushDistance; }

	// --- [시각화 컴포넌트 등에서 호출할 밀치기 예상 결과 반환 함수] ---
	TArray<FPushSimulationResult> SimulatePush(AACGridSystem* GridSystem, AActor* Instigator, FIntPoint TargetPos, const TSet<FIntPoint>& AffectedGridPositions) const;

protected:
	// 방사형 vs 지향성 선택
	UPROPERTY(EditAnywhere, Category = "Push")
	EPEPushType PushType = EPEPushType::Directional;

	UPROPERTY(EditAnywhere, Category = "Push")
	int32 PushDistance = 1; // 뒤로 몇 칸 밀 것인가?

	// 충돌 시 대상(또는 본인)의 최대 체력 대비 입을 피해량 (0.2 = 20%)
	UPROPERTY(EditAnywhere, Category = "Push|Collision")
	float CollisionDamageRatio = 0.2f;
};