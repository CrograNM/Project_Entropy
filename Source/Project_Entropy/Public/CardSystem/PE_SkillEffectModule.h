// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PE_SkillEffectModule.generated.h"

class UPE_SkillData;
class APE_CharacterBase;
class AACGridSystem;
class AACTile;

/**
 * 스킬 조립을 위한 기본 효과 모듈 뼈대 (GAS의 GameplayEffect 역할)
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable)
class PROJECT_ENTROPY_API UPE_SkillEffectModule : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 다중 타겟(Targets) 그룹 전체에 효과를 적용합니다.
	 * TargetLocation(월드)과 TargetGridPos(논리 좌표)는 같은 지점을 가리킵니다.
	 * 밀치기처럼 칸 단위로 계산하는 모듈은 월드 좌표를 역산하지 말고 TargetGridPos를 쓰십시오.
	 */
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage) PURE_VIRTUAL(UPE_SkillEffectModule::ApplyEffects, );
};

// --- [모듈 1] 기본 데미지 적용 모듈 ---
UCLASS(DisplayName = "Effect: Damage")
class PROJECT_ENTROPY_API UPE_SkillEffect_Damage : public UPE_SkillEffectModule
{
	GENERATED_BODY()

public:
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
};

// --- [밀치기 타입 정의] ---
UENUM(BlueprintType)
enum class EPEPushType : uint8
{
	Radial		UMETA(DisplayName = "방사형 (Radial - 폭발)"),
	Directional UMETA(DisplayName = "지향성 (Directional - 파도/바람)")
};

/**
 * 밀치기 1회분의 시뮬레이션 결과.
 *
 * 시각화는 앞쪽 4개(어디서 어디로 어느 방향으로 누가)만 보면 되고,
 * 실제 적용(ApplyEffects)은 뒤쪽 필드로 이동 경로 / 충돌 데미지 / 연쇄 지연까지 그대로 실행합니다.
 * 두 경로가 같은 구조체를 소비하므로 "보이는 밀림"과 "실제 밀림"이 갈라질 수 없습니다.
 */
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

	// --- [실제 적용에만 쓰이는 정보] ---

	// 실제로 지나갈 타일 목록 (이동 연출용). 즉시 막혔다면 비어 있습니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<AACTile*> Path;

	// 이 밀치기가 시작될 때 남아있던 밀림 거리. 충돌 데미지 비율 산출에 씁니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 RemainingDist = 0;

	// 연쇄 밀치기가 앞 캐릭터의 이동을 기다리는 시간
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Delay = 0.f;

	// 다른 캐릭터에 부딪혀 멈췄다면 그 대상 (벽/장애물에 막혔으면 null)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APE_CharacterBase* HitCharacter = nullptr;

	// 목표 거리를 다 못 가고 무언가에 막혀 멈췄는지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bBlocked = false;
};

// --- [모듈 2] 넉백(밀치기) 적용 모듈 ---
UCLASS(DisplayName = "Effect: Push (Knockback)")
class PROJECT_ENTROPY_API UPE_SkillEffect_Push : public UPE_SkillEffectModule
{
	GENERATED_BODY()

public:
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage) override;
	int32 GetPushDistance() const { return PushDistance; }

	/**
	 * 밀치기의 단일 진실 공급원.
	 *
	 * 방향 산출 -> Back-to-Front 정렬 -> 연쇄 밀치기 전개를 전부 여기서 끝내고,
	 * ApplyEffects는 그 결과를 '실행만' 합니다. 시각화는 같은 결과를 '그리기만' 합니다.
	 * 월드/게임 상태를 바꾸지 않으므로 클라이언트에서도 안전하게 호출할 수 있습니다.
	 */
	TArray<FPushSimulationResult> SimulatePush(const AACGridSystem* GridSystem, AActor* Instigator, FIntPoint TargetGridPos, const TSet<APE_CharacterBase*>& Targets) const;

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