// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PE_CharacterBase.generated.h"

class UACGridMovementComponent;
class UACStatComponent;

UCLASS(Abstract) // 인스턴스화 방지 (반드시 상속해서 사용)
class PROJECT_ENTROPY_API APE_CharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	APE_CharacterBase();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE UACGridMovementComponent* GetGridMovementComponent() const { return GridMovement; }
	FORCEINLINE UACStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SnapCharacterToNearestTile();
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	/** 스탯 컴포넌트의 OnDeath 델리게이트에 바인딩될 공통 사망 처리 함수 */
	UFUNCTION()
	virtual void HandleDeath();

	/** 전장(Grid) 이동 제어 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACGridMovementComponent> GridMovement;

	/** 체력 및 행동력(AP) 제어 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACStatComponent> StatComponent;
};