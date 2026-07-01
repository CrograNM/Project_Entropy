// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACStatComponent.generated.h"

// 상태 변경 시 UI 갱신 등을 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHPChangedSignature, float, CurrentHP, float, MaxHP, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAPChangedSignature, int32, CurrentAP, int32, MaxAP, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACStatComponent();

protected:
	virtual void BeginPlay() override;

public:
	/** --- 이벤트 델리게이트 --- */
	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnHPChangedSignature OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnAPChangedSignature OnAPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnDeathSignature OnDeath;

	/** --- 체력 제어 --- */
	UFUNCTION(BlueprintCallable, Category = "Stats|HP")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats|HP")
	void Heal(float Amount);

	/** --- 행동력(AP) 제어 --- */
	// AP가 충분하여 소모에 성공하면 true, 부족하면 false를 반환합니다.
	UFUNCTION(BlueprintCallable, Category = "Stats|AP")
	bool ConsumeAP(int32 Amount);

	// 턴 시작 시 AP를 최대치로 회복합니다.
	UFUNCTION(BlueprintCallable, Category = "Stats|AP")
	void ResetAP();

	/** --- Getter --- */
	FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
	FORCEINLINE int32 GetCurrentAP() const { return CurrentAP; }
	FORCEINLINE bool IsDead() const { return bIsDead; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|HP")
	float MaxHP;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|HP")
	float CurrentHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|AP")
	int32 MaxAP;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|AP")
	int32 CurrentAP;

	bool bIsDead;
};
