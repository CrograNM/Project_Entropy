// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PE_CharacterBase.generated.h"

class UACGridMovementComponent;
class UACStatComponent;
class UACSkillComponent;

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

	// 범용 피아식별용 TeamID 반환 함수
	UFUNCTION(BlueprintCallable, Category = "Team")
	virtual int32 GetTeamID() const;

	// 이 엔티티가 밀치기(넉백) 스킬에 밀려나는지 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Status")
	bool IsPushable() const { return bIsPushable; }

protected:
	/** 스탯 컴포넌트의 OnDeath 델리게이트에 바인딩될 공통 사망 처리 함수 */
	UFUNCTION()
	virtual void HandleDeath();

	// 몬스터 등 PlayerState가 없는 AI들을 위한 기본 팀 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 1;

	// 에디터에서 개별적으로 밀치기 면역을 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bIsPushable = true;

	/** 전장(Grid) 이동 제어 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACGridMovementComponent> GridMovement;

	/** 체력 및 행동력(AP) 제어 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACStatComponent> StatComponent;

	// 스킬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACSkillComponent> SkillComponent;
};