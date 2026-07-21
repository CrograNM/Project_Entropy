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
	void InitializeActionActor(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage);

protected:
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 자식 BP에서 Sphere, Box 등으로 교체할 수 있도록 UShapeComponent로 선언
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UShapeComponent> CollisionComp;

	// 이동이 필요 없는 스킬일 경우, 자식 BP에서 컴포넌트 비활성화
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> ActionVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> ActionSFXComponent;

	// 적중 시 파괴될 것인가? (투사체 = true / 장판 = false 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Action")
	bool bDestroyOnHit = true;

private:
	UPROPERTY()
	TObjectPtr<UPE_SkillLogicBase> SkillLogicInstance;

	UPROPERTY()
	TObjectPtr<AActor> Caster;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	UPROPERTY()
	const UPE_SkillData* SkillData;

	float DamageToApply;
};