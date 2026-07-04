// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "PE_EnemyBase.generated.h"

// 턴 매니저에게 내 행동이 완전히 끝났음을 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyTurnFinishedSignature);

UCLASS()
class PROJECT_ENTROPY_API APE_EnemyBase : public APE_CharacterBase
{
	GENERATED_BODY()

public:
	APE_EnemyBase();

protected:
	virtual void BeginPlay() override;

public:
	/** 턴 매니저가 이 적의 차례일 때 호출하는 함수 */
	void StartTurn();

	/** 적의 행동이 모두 끝났을 때 방송되는 델리게이트 */
	UPROPERTY(BlueprintAssignable)
	FOnEnemyTurnFinishedSignature OnTurnFinished;

protected:
	/** 더미 AI의 공격 사거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	int32 AttackRange = 1;

private:
	/** AI의 핵심 로직 (이동 및 타겟 탐색) */
	void ProcessAI();

	/** 이동 컴포넌트가 목표 지점에 도착했을 때 발동할 콜백 함수 */
	UFUNCTION()
	void OnMovementCompleted();

	/** 이동 후 플레이어를 때릴 수 있는지 검사하고 행동을 종료하는 함수 */
	void EvaluateAttackAndEndTurn();

	/** 안전하게 턴 매니저에게 다음 순서를 넘김 */
	void FinishTurn();
};