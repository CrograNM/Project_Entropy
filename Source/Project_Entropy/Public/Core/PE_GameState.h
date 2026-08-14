// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Containers/Queue.h"
#include "CardSystem/PE_DataTypes.h"
#include "Core/PE_GameMode.h"
#include "PE_GameState.generated.h"

class UPE_TurnManagerComponent;

// UI 갱신 방송용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionQueueUpdatedSignature, const TArray<FPEActionLogData>&, CurrentQueue);

UCLASS()
class PROJECT_ENTROPY_API APE_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	APE_GameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override; 

	FORCEINLINE UPE_TurnManagerComponent* GetTurnManager() const { return TurnManager; }
	FORCEINLINE EPEGameState GetCurrentState() const { return CurrentState; }

	// --- [Action Queue 시스템] ---
	void EnqueueSkillAction(const FPESkillActionPayload& Payload); // 스킬 발동을 대기열에 추가

	// 레퍼런스 카운팅 방식의 액션 시작/종료 보고
	void ReportActionStarted();
	void ReportActionEnded(int32 ActionLogID = -1);

	// 현재 큐에 남은 행동이 있거나, 누군가 스킬을 실행 중인지 확인
	bool IsActionQueueActive() const { return bIsProcessingAction || !ActionQueue.IsEmpty() || PendingActionCount > 0; }

	// --- [Action UI Queue 시스템] ---
	UPROPERTY(BlueprintAssignable, Category = "Action Queue")
	FOnActionQueueUpdatedSignature OnActionQueueUpdated;

	// 텍스트 로그 등록 및 ID 반환
	int32 AddActionLog(int32 TeamID, const FString& Text);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;

	// 현재 게임의 상태를 모든 클라이언트에 복제
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentState, Category = "State")
	EPEGameState CurrentState;

	UFUNCTION()
	void OnRep_CurrentState();

	// 복제되는 실시간 UI 큐
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_ActionLogQueue, Category = "Action Queue")
	TArray<FPEActionLogData> ActionLogQueue;

	UFUNCTION()
	void OnRep_ActionLogQueue();

private:
	// 스킬 발동 대기열
	TQueue<FPESkillActionPayload> ActionQueue;

	// 현재 누군가 스킬을 쏘고 진행 중인지 여부
	bool bIsProcessingAction = false;

	UPROPERTY(VisibleAnywhere, Category = "Action Queue")
	int32 PendingActionCount = 0;

	// 큐에서 다음 행동을 꺼내어 실행
	void ProcessNextAction();
	void RemoveActionLog(int32 ActionID);

	FTimerHandle ActionDelayTimerHandle;

	// 스킬과 스킬 사이의 대기 시간 (0.2초)
	UPROPERTY(EditDefaultsOnly, Category = "Action Queue")
	float ActionInterval = 0.2f;
};