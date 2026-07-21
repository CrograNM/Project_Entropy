// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PE_ProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UPE_SkillLogicBase;
class UPE_SkillData;

UCLASS()
class PROJECT_ENTROPY_API APE_ProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	APE_ProjectileBase();

	/** 스킬 로직에서 투사체를 스폰하자마자 데이터를 배달해 주는 함수 */
	void InitializeProjectile(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage);

protected:
	/** 물체에 닿았을 때 발생하는 이벤트 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	// GC가 스킬 로직을 지우지 못하도록 UPROPERTY로 참조 유지
	UPROPERTY()
	TObjectPtr<UPE_SkillLogicBase> SkillLogicInstance;

	// 배달할 데이터들
	UPROPERTY()
	TObjectPtr<AActor> Caster;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	UPROPERTY()
	const UPE_SkillData* SkillData;

	float DamageToApply;
};