// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "PE_EnemyBase.generated.h"

// 턴 매니저에게 내 행동이 완전히 끝났음을 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnemyTurnFinishedSignature);

class UACSkillComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_EnemyBase : public APE_CharacterBase
{
	GENERATED_BODY()

public:
	APE_EnemyBase();

	/** 턴 매니저가 이 적의 차례일 때 호출하는 함수 */
	void StartTurn();

	/** 적의 행동이 모두 끝났을 때 방송되는 델리게이트 */
	UPROPERTY(BlueprintAssignable)
	FOnEnemyTurnFinishedSignature OnTurnFinished;

protected:
	virtual void BeginPlay() override;

	/** 이동 딜레이를 주기 위한 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Behavior")
	float ActionDelay = 0.5f;
	
private:
	/** AI의 행동 판단 루프 (AP가 소진될 때까지 스스로 계속 호출됨) */
	void EvaluateAndTakeAction();

	/** 이동 컴포넌트가 목표 지점에 도착했을 때 발동할 콜백 함수 */
	UFUNCTION()
	void OnMovementCompleted();

	/** 안전하게 턴 매니저에게 다음 순서를 넘김 */
	void FinishTurn();

	/** AI 행동 딜레이(시각적 효과 주기)용 타이머 핸들 */
	FTimerHandle ActionDelayTimerHandle;

	/** 대기 후 이동할 경로 임시 저장 */
	UPROPERTY()
	TArray<class AACTile*> PendingMovePath;

	/** 딜레이 이후 실제 이동 실행 */
	void ExecutePendingMovement();

	/** --- 스킬 대기 --- */
	int32 PendingSkillIndex = -1;

	UPROPERTY()
	TObjectPtr<class AACTile> PendingSkillTargetTile;

	UPROPERTY()
	TObjectPtr<class APE_CharacterBase> PendingSkillTargetCharacter;

	void ExecutePendingSkill();

	/** --- 멀티플레이어 시각화 RPC --- */
	// 공격 의도 표시
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_ShowSkillIntent(AACTile* TargetTile);

	// 이동 의도 표시
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_ShowMoveIntent(FIntPoint StartPos, int32 MoveRange, AACTile* DestinationTile);

	// 표시된 하이라이트 지우기
	UFUNCTION(NetMulticast, Unreliable)
	void NetMulticast_ClearIntent();
};