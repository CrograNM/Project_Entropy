// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Containers/Queue.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_GameState.generated.h"

class UPE_TurnManagerComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	APE_GameState();

	/** 전역에서 쉽게 턴 매니저에 접근할 수 있도록 Getter 제공 */
	FORCEINLINE UPE_TurnManagerComponent* GetTurnManager() const { return TurnManager; }

	// --- [Action Queue 시스템] ---
	void EnqueueSkillAction(const FPESkillActionPayload& Payload); // 스킬 발동을 대기열에 추가
	void CompleteCurrentAction();	// 현재 발동 중인 액션(투사체 적중 등)이 완전히 끝났음을 알림

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;

private:
	// 스킬 발동 대기열
	TQueue<FPESkillActionPayload> ActionQueue;

	// 현재 누군가 스킬을 쏘고 진행 중인지 여부
	bool bIsProcessingAction = false;

	// 큐에서 다음 행동을 꺼내어 실행
	void ProcessNextAction();

	FTimerHandle ActionDelayTimerHandle;

	// 스킬과 스킬 사이의 대기 시간 (0.2초)
	UPROPERTY(EditDefaultsOnly, Category = "Action Queue")
	float ActionInterval = 0.2f;
};