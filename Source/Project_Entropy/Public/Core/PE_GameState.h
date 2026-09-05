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

/**
 * 진행 중인 액션 1건의 추적 정보.
 * 완료 보고가 유실되어 큐가 멈추는 것을 감지하기 위해 발급 시각과 발생 지점을 함께 보관합니다.
 */
struct FPEPendingAction
{
	int32 ActionLogID = -1;
	double StartTime = 0.0;
	FString Context;
};

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

	/**
	 * 액션 시작을 보고하고 추적 토큰을 발급받습니다.
	 * @param Context     완료 보고가 유실됐을 때 원인을 추적하기 위한 발생 지점 문자열
	 * @param ActionLogID 이 액션이 소유한 UI 로그 ID. 워치독이 강제 해제할 때 함께 정리됩니다.
	 * @return 발급된 토큰 ID. 반드시 EndAction으로 반납해야 큐가 다음으로 진행합니다.
	 */
	int32 BeginAction(const FString& Context = TEXT("Unknown"), int32 ActionLogID = -1);

	/** 발급받은 토큰을 반납합니다. ActionLogID가 유효하면 UI 로그도 함께 제거합니다. */
	void EndAction(int32 TokenID, int32 ActionLogID = -1);

	// 클라이언트에서 애니메이션이 완전히 끝나 발사 확정을 지을 때 호출
	void CommitCurrentAction();

	void AdvanceTurnEndPhase();
	void ReportTurnEndCardsFinished(class APE_PlayerController* PC);

	bool IsActionQueueActive() const { return bIsProcessingAction || !ActionQueue.IsEmpty() || PendingActions.Num() > 0; }

	// --- [Action UI Queue 시스템] ---
	UPROPERTY(BlueprintAssignable, Category = "Action Queue")
	FOnActionQueueUpdatedSignature OnActionQueueUpdated;

	int32 AddActionLog(int32 TeamID, const FString& Text);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentState, Category = "State")
	EPEGameState CurrentState;

	UFUNCTION()
	void OnRep_CurrentState();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_ActionLogQueue, Category = "Action Queue")
	TArray<FPEActionLogData> ActionLogQueue;

	UFUNCTION()
	void OnRep_ActionLogQueue();

private:
	// 스킬 발동 대기열
	TQueue<FPESkillActionPayload> ActionQueue;

	// 애니메이션 재생 등을 대기 중인 현재 큐 아이템
	UPROPERTY()
	FPESkillActionPayload CurrentProcessingPayload;

	// --- [턴 종료 대기열 관리 변수] ---
	UPROPERTY()
	TArray<class APE_PlayerController*> TurnEndPlayersQueue;
	bool bTurnEndCardPhaseActive = false;

	// 현재 누군가 스킬을 쏘고 진행 중인지 여부
	bool bIsProcessingAction = false;

	// 발급되었으나 아직 반납되지 않은 액션들 (Key: 토큰 ID)
	TMap<int32, FPEPendingAction> PendingActions;
	int32 NextActionTokenID = 0;

	// 에디터/인게임 관찰용 미반납 액션 수 (PendingActions.Num()의 거울값)
	UPROPERTY(VisibleAnywhere, Category = "Action Queue")
	int32 PendingActionCount = 0;

	// 큐에서 다음 행동을 꺼내어 실행
	void ProcessNextAction();
	void RemoveActionLog(int32 ActionID);

	/** 완료 보고가 유실된 액션을 찾아 강제 해제하여 큐가 영구 정지하는 것을 막습니다. */
	void TickActionWatchdog();

	FTimerHandle ActionDelayTimerHandle;
	FTimerHandle ActionWatchdogTimerHandle;

	// 스킬과 스킬 사이의 대기 시간 (0.2초)
	UPROPERTY(EditDefaultsOnly, Category = "Action Queue")
	float ActionInterval = 0.2f;

	// 이 시간(초)을 넘겨 완료 보고가 없으면 유실로 간주하고 강제 해제합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Action Queue")
	float ActionTimeout = 5.f;

	// 워치독 검사 주기(초)
	UPROPERTY(EditDefaultsOnly, Category = "Action Queue")
	float WatchdogInterval = 1.f;
};