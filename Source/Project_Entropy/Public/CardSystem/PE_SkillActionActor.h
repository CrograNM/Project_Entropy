// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PE_SkillActionActor.generated.h"

class UShapeComponent;
class UProjectileMovementComponent;
class UPE_SkillData;
class UNiagaraComponent;
class UAudioComponent;

/**
 * 충돌 판정과 생명 주기를 가지는 스킬의 물리적 실체 (투사체, 장판, 함정 등)
 */
UCLASS()
class PROJECT_ENTROPY_API APE_SkillActionActor : public AActor
{
	GENERATED_BODY()

public:
	APE_SkillActionActor(); 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; 
	void InitializeActionActor(AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, int32 InPhaseIndex,
		float InDamage, int32 InActionLogID, int32 InActionTokenID, const TSet<class APE_CharacterBase*>& InTargets, FIntPoint InCasterGridPos, FIntPoint InTargetGridPos);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 물리 엔진 의존성을 없애고 순수 렌더링용 루트로 변경
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> ActionVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> ActionSFXComponent;

private:
	// --- [데이터 복제] ---
	UFUNCTION()
	void OnRep_SkillData();

	UPROPERTY(ReplicatedUsing = OnRep_SkillData)
	TObjectPtr<const UPE_SkillData> RepSkillData;

	UPROPERTY(ReplicatedUsing = OnRep_SkillData)
	int32 RepPhaseIndex = 0;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> RepTargetActor;

	// 클라이언트도 동일한 도착지점을 향해 궤적을 그리도록 복제
	UPROPERTY(Replicated)
	FVector RepTargetLocation;

	UPROPERTY()
	TObjectPtr<AActor> Caster;

	float DamageToApply;
	int32 ActionLogID = -1;

	// 액션 큐에서 발급받은 토큰. 타격을 마치거나 도중에 파괴될 때 반드시 반납해야 합니다.
	int32 ActionTokenID = -1;
	bool bHasReportedEnd = false;

	/** 액션 큐 토큰을 1회만 반납하도록 보장합니다. */
	void ReleaseActionToken();

	// --- [수학적 궤적(포물선) 연산용 변수] ---
	FVector StartLocation;
	float FlightDuration = 0.f;
	float CurrentFlightTime = 0.f;
	float ArcHeight = 0.f;
	bool bIsFlying = false;

	// 타격 대기 중인 대상 목록
	UPROPERTY()
	TSet<class APE_CharacterBase*> PendingTargets;

	// 폭발 크기 계산을 위해 저장하는 논리적 그리드 좌표 (서버 전용)
	FIntPoint CasterGridPos;
	FIntPoint TargetGridPos;

	// 폭발 과정을 두 단계(시각적 폭발 -> 실제 물리적 타격)로 분리합니다.
	void TriggerExplosion();
	void ApplyHitAndEffects();
};