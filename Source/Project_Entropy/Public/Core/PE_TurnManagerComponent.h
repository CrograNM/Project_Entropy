// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PE_TurnManagerComponent.generated.h"

UENUM(BlueprintType)
enum class EPEBattlePhase : uint8
{
	None,
	BattleStart,     // 전투 초기화 (덱 셔플, 초기 카드 드로우 등)
	TeamTurn,
	EnvironmentTurn, // 환경 페이즈 (원소 타일 지속시간 감소, 화상 데미지 등)
	BattleEnd        // 전투 종료 (승/패)
};

class APE_EnemyBase;

// 턴 변경 이벤트 방송용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, EPEBattlePhase, NewPhase);

// 팀 턴이 시작될 때 어떤 팀의 턴인지 UI에 알려주는 전용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamTurnStartedSignature, int32, TeamID);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_ENTROPY_API UPE_TurnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPE_TurnManagerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 페이즈가 바뀔 때마다 발동하는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnPhaseChangedSignature OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnTeamTurnStartedSignature OnTeamTurnStarted;

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void StartBattle();

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void EndCurrentPhase();

	// --- [턴 종료 만장일치 시스템] ---
	void RequestTurnEnd(class APE_PlayerController* PC, bool bReady);
	bool IsPendingTurnEnd() const { return bPendingTurnEnd; }
	void ExecuteTurnEnd(); // 큐와 레디가 모두 달성되었을 때 내부 호출됨

	/** 현재 페이즈 반환 */
	FORCEINLINE EPEBattlePhase GetCurrentPhase() const { return CurrentPhase; }
	FORCEINLINE int32 GetCurrentTurnCount() const { return CurrentTurnCount; }
	FORCEINLINE int32 GetCurrentTeamTurn() const { return CurrentTeamTurn; }

protected:
	virtual void BeginPlay() override;

private:
	/** 실제 페이즈 변경 및 델리게이트 방송을 처리하는 내부 함수 */
	void ChangePhase(EPEBattlePhase NewPhase);
	void PrepareTeamTurn(); // 팀 턴 시작 시 초기화 및 AI 가동
	
	UFUNCTION()
	void ProcessNextEnemy();
	
	UFUNCTION()
	void TriggerNextEnemyWithDelay();

	UPROPERTY()
	TArray<APE_EnemyBase*> EnemyQueue;
	
	// --- 멀티플레이어 동기화 변수 및 함수 ---
	UFUNCTION()
	void OnRep_CurrentPhase(EPEBattlePhase OldPhase);

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentPhase, Category = "Turn System")
	EPEBattlePhase CurrentPhase;

	UFUNCTION() 
	void OnRep_CurrentTeamTurn();

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentTeamTurn, Category = "Turn System")
	int32 CurrentTeamTurn = 0; // 현재 턴을 진행 중인 팀 ID

	UPROPERTY(EditDefaultsOnly, Category = "Turn System")
	int32 MaxTeams = 2; // 교전에 참여하는 최대 팀 수 (0팀, 1팀)

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Turn System")
	int32 CurrentTurnCount;

	// --- [추가됨: 멀티플레이어 동기화 변수] ---
	UPROPERTY()
	TArray<class APE_PlayerController*> ReadyPlayers;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Turn System|Ready")
	int32 ReadyPlayerCount = 0;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Turn System|Ready")
	int32 TotalPlayerCount = 0;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Turn System|Ready")
	bool bPendingTurnEnd = false; // 모두가 레디를 눌렀지만 큐가 덜 끝나서 대기 중인 상태

	void EvaluateTurnEnd();
};