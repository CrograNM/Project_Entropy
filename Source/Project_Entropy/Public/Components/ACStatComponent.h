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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/* --- Setter --- */
	UFUNCTION(BlueprintCallable, Category = "Stats | Setters")
	void SetMaxHP(float InMaxHP);

	UFUNCTION(BlueprintCallable, Category = "Stats | Setters")
	void SetHP(float InHP);

	UFUNCTION(BlueprintCallable, Category = "Stats | Setters")
	void SetMaxAP(int32 InMaxAP);

	UFUNCTION(BlueprintCallable, Category = "Stats | Setters")
	void SetAP(int32 InAP);
	
	UFUNCTION(BlueprintCallable, Category = "Stats | Setters")
	void SetMoveRange(int32 InRange) { MoveRange = InRange; }

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
	void TakeDamage(float Amount);

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
	FORCEINLINE bool IsDead() const { return bIsDead; }
	FORCEINLINE float GetMaxHP() const { return MaxHP; }
	FORCEINLINE float GetCurrentHP() const { return CurrentHP; }
	FORCEINLINE int32 GetCurrentAP() const { return CurrentAP; }
	FORCEINLINE int32 GetMoveRange() const { return MoveRange; }

protected:
	// 변수들이 변경될 때 클라이언트에서 호출될 OnRep 함수들
	UFUNCTION()
	void OnRep_CurrentHP(float OldHP);

	UFUNCTION()
	void OnRep_CurrentAP(int32 OldAP);

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_IsDead, Category = "Stats|State")
	bool bIsDead;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Stats|HP")
	float MaxHP;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentHP, BlueprintReadOnly, Category = "Stats|HP")
	float CurrentHP;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Stats|AP")
	int32 MaxAP;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentAP, BlueprintReadOnly, Category = "Stats|AP")
	int32 CurrentAP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Movement")
	int32 MoveRange = 4;
};
