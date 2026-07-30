// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PE_SkillActionActor.generated.h"

class UShapeComponent;
class UProjectileMovementComponent;
class UPE_SkillLogicBase;
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
	void InitializeActionActor(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage);

protected:
	virtual void Tick(float DeltaTime) override;

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

	UPROPERTY(Replicated)
	TObjectPtr<AActor> RepTargetActor;

	// 클라이언트도 동일한 도착지점을 향해 궤적을 그리도록 복제
	UPROPERTY(Replicated)
	FVector RepTargetLocation;

	UPROPERTY()
	TObjectPtr<UPE_SkillLogicBase> SkillLogicInstance;

	UPROPERTY()
	TObjectPtr<AActor> Caster;

	float DamageToApply;

	// --- [수학적 궤적(포물선) 연산용 변수] ---
	FVector StartLocation;
	float FlightDuration = 0.f;
	float CurrentFlightTime = 0.f;
	float ArcHeight = 0.f;
	bool bIsFlying = false;

	// 목표 도착 시 폭발 처리
	void Explode();
};