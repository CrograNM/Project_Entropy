// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardSystem/PE_DataTypes.h"
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

	UFUNCTION(BlueprintCallable, Category = "Skill System")
	bool TryExecuteSkill(int32 SkillIndex, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);

	UFUNCTION(BlueprintCallable, Category = "Skill System")
	bool TryExecuteSkillByData(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage, int32 ClientRequestID = -1, bool bIsFreeCast = false);

	// 게임 스테이트의 큐 연산 대기 준비 단계 (유효성 검사 후 애니메이션 지시)
	void PrepareQueuedSkill(const FPESkillActionPayload& Payload);

	// 클라이언트 측 애니메이션이 끝나고 서버에서 실제로 물리적 연산을 처리하는 확정 단계
	void CommitQueuedSkill(const FPESkillActionPayload& Payload);

	// 현재 장착된 스킬 목록 반환
	UFUNCTION(BlueprintCallable, Category = "Skill System")
	TArray<UPE_SkillData*> GetActiveSkills() const { return ActiveSkills; }

	// --- 스킬 시각화 멀티캐스트 RPC ---
	// 시전자(소유자)에서 1번 발생하는 시전 연출
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_PlayCastVisuals(const UPE_SkillData* SkillData);

	// 목표 지점에서 1번 발생하는 거대한 광역 폭발 연출
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_PlayExplosionVisuals(const UPE_SkillData* SkillData, FVector TargetLocation, FRotator TargetRotation, FVector2D ExplosionSize, float ExplosionRadius);

	// 개별 대상의 몸에서 발생하는 피격 연출
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_PlayHitVisuals(const UPE_SkillData* SkillData, FVector TargetLocation);

protected:
	virtual void BeginPlay() override;

	// 기본 장착 스킬 데이터 (주로 몬스터의 패턴용 세팅)
	UPROPERTY(EditAnywhere, Category = "Skill System")
	TArray<TObjectPtr<UPE_SkillData>> DefaultSkills;

private:
	// 런타임에 소유자가 사용할 수 있는 스킬 데이터들 (데이터 에셋의 참조만 가짐)
	UPROPERTY()
	TArray<TObjectPtr<UPE_SkillData>> ActiveSkills;

	// 시전자(소유자)의 스탯 컴포넌트 캐싱 (AP 통제용)
	UPROPERTY()
	TObjectPtr<UACStatComponent> OwnerStatComponent;
};