// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Combat/PE_PushTypes.h"
#include "PE_SkillEffectModule.generated.h"

class UPE_SkillData;
class APE_CharacterBase;
class AACGridSystem;
class AACTile;
struct FPEPushField;

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

/**
 * --- [모듈 2] 넉백(밀치기) 어댑터 ---
 *
 * 밀치기 규칙은 FPEPushResolver가, 실행과 연쇄는 UPE_PushCoordinatorComponent가 소유합니다.
 * 이 모듈이 하는 일은 에디터에서 설정한 값을 밀치기 요청으로 바꿔 넘기는 것뿐입니다.
 * 덕분에 장판 / 함정 / 돌진처럼 스킬이 아닌 주체도 같은 요청만 만들면 똑같이 밀 수 있습니다.
 */
UCLASS(DisplayName = "Effect: Push (Knockback)")
class PROJECT_ENTROPY_API UPE_SkillEffect_Push : public UPE_SkillEffectModule
{
	GENERATED_BODY()

public:
	virtual void ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage) override;

	int32 GetPushDistance() const { return PushDistance; }

	/**
	 * 이 모듈이 만들어낼 밀치기 요청 목록.
	 * 서버 실행과 클라 시각화가 같은 함수를 통과하므로 입력이 갈라질 수 없습니다.
	 */
	TArray<FPEPushRequest> BuildPushRequests(const FPEPushField& Field, AActor* Instigator, FIntPoint TargetGridPos, const TSet<APE_CharacterBase*>& Targets) const;

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
