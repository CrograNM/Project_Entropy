// Copyright CrograNM

#include "Core/PE_GameState.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACSkillComponent.h"
#include "Net/UnrealNetwork.h"
#include "Core/PE_PlayerController.h"

APE_GameState::APE_GameState()
{
	// 턴 매니저 컴포넌트 생성 및 부착 (기존 GameMode에서 이관)
	TurnManager = CreateDefaultSubobject<UPE_TurnManagerComponent>(TEXT("TurnManager"));
}

void APE_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APE_GameState, CurrentState);
	DOREPLIFETIME(APE_GameState, ActionLogQueue);
}

void APE_GameState::BeginPlay()
{
	Super::BeginPlay();

	// 서버인 경우 GameMode(새 맵의 룰)에서 현재 상태를 가져와 동기화 변수에 세팅합니다.
	if (HasAuthority())
	{
		if (APE_GameMode* GM = Cast<APE_GameMode>(GetWorld()->GetAuthGameMode()))
		{
			CurrentState = GM->GetCurrentState();
			OnRep_CurrentState(); // 서버 자신도 로컬 갱신을 위해 수동 호출
		}
	}
}

void APE_GameState::OnRep_CurrentState()
{
	UE_LOG(LogTemp, Warning, TEXT("[APE_GameState] CurrentState가 복제됨: %s"), *UEnum::GetValueAsString(CurrentState));

	// 존재하는 모든 플레이어 컨트롤러를 순회하며 확실하게 찔러줍니다. 
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
		{
			PC->SwitchInputMode(CurrentState);
		}
	}
}

void APE_GameState::EnqueueSkillAction(const FPESkillActionPayload& Payload)
{
	// 큐잉 시스템은 오직 서버에서만 동작합니다.
	if (!HasAuthority()) return;

	ActionQueue.Enqueue(Payload);

	// 현재 쏘고 있는 스킬이 아무것도 없다면 즉시 대기열 처리 시작
	if (!bIsProcessingAction)
	{
		ProcessNextAction();
	}
}

void APE_GameState::ReportActionStarted()
{
	if (HasAuthority())
	{
		PendingActionCount++;
	}
}

void APE_GameState::ReportActionEnded(int32 ActionLogID)
{
	if (!HasAuthority()) return;

	PendingActionCount--;

	// UI 큐 항목 삭제 처리
	RemoveActionLog(ActionLogID);

	// 꼬리를 무는 넉백을 포함해 파생된 모든 연산이 0이 될 때만 다음 큐로 넘어감
	if (PendingActionCount <= 0)
	{
		PendingActionCount = 0;
		GetWorld()->GetTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_GameState::ProcessNextAction, ActionInterval, false);
	}
}

void APE_GameState::ProcessNextAction()
{
	if (ActionQueue.IsEmpty())
	{
		bIsProcessingAction = false;

		if (TurnManager && TurnManager->IsPendingTurnEnd())
		{
			// 0.2초 딜레이를 타이머로 작동시킴으로써 턴 종료 카드 간의 흐름을 자연스럽게 유도
			GetWorld()->GetTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_GameState::AdvanceTurnEndPhase, ActionInterval, false);
		}
		return;
	}

	bIsProcessingAction = true;

	// 1차 스킬 본체의 액션 카운트 부여
	ReportActionStarted();

	ActionQueue.Dequeue(CurrentProcessingPayload);

	// 스킬 실제 실행 지시
	if (CurrentProcessingPayload.Instigator && CurrentProcessingPayload.SkillData)
	{
		if (UACSkillComponent* SkillComp = CurrentProcessingPayload.Instigator->FindComponentByClass<UACSkillComponent>())
		{
			SkillComp->PrepareQueuedSkill(CurrentProcessingPayload);
		}
		else ReportActionEnded();
	}
	else ReportActionEnded();
}

void APE_GameState::CommitCurrentAction()
{
	if (CurrentProcessingPayload.Instigator)
	{
		if (UACSkillComponent* SkillComp = CurrentProcessingPayload.Instigator->FindComponentByClass<UACSkillComponent>())
		{
			SkillComp->CommitQueuedSkill(CurrentProcessingPayload);
		}
	}
}

void APE_GameState::AdvanceTurnEndPhase()
{
	if (!HasAuthority()) return;

	// 1. 처음 진입했을 때 플레이어들을 배열(줄)에 세웁니다.
	if (!bTurnEndCardPhaseActive)
	{
		bTurnEndCardPhaseActive = true;
		TurnEndPlayersQueue.Empty();

		// 현재 턴을 종료하려는 팀 번호 가져오기
		int32 CurrentTeam = TurnManager ? TurnManager->GetCurrentTeamTurn() : 0;

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
			{
				if (APE_PlayerCharacter* Char = PC->GetCachedPlayerCharacter())
				{
					// PVP 대응: 턴을 마치는 팀 소속 플레이어들만 지목합니다!
					if (Char->GetTeamID() == CurrentTeam)
					{
						TurnEndPlayersQueue.Add(PC);
					}
				}
			}
		}
	}

	// 2. 대기열에 남은 사람이 있으면 1명씩 호명합니다.
	if (TurnEndPlayersQueue.Num() > 0)
	{
		APE_PlayerController* NextPC = TurnEndPlayersQueue[0];
		TurnEndPlayersQueue.RemoveAt(0);

		if (NextPC)
		{
			NextPC->Client_TriggerTurnEndCards(); // "네 카드들 쏴라"
		}
		else
		{
			AdvanceTurnEndPhase(); // 오류로 빈자리일 경우 다음 사람으로
		}
	}
	else
	{
		// 3. 모든 사람의 카드 발동이 끝났다면 진짜 턴을 끝냅니다!
		bTurnEndCardPhaseActive = false;
		if (TurnManager) TurnManager->ExecuteTurnEnd();
	}
}

void APE_GameState::ReportTurnEndCardsFinished(APE_PlayerController* PC)
{
	if (TurnEndPlayersQueue.Contains(PC))
	{
		TurnEndPlayersQueue.Remove(PC);
	}
	// 방금 끝낸 플레이어를 큐에서 제거하고 다음 플레이어 검사
	AdvanceTurnEndPhase();
}

// --- [UI 액션 큐 제어부] ---
int32 APE_GameState::AddActionLog(int32 TeamID, const FString& Text)
{
	if (!HasAuthority()) return -1;

	static int32 GlobalLogID = 0;
	FPEActionLogData NewLog;
	NewLog.ActionID = ++GlobalLogID;
	NewLog.TeamID = TeamID;
	NewLog.ActionText = Text;

	ActionLogQueue.Add(NewLog);
	OnRep_ActionLogQueue(); // 로컬 갱신 강제 호출
	return NewLog.ActionID;
}
void APE_GameState::RemoveActionLog(int32 ActionID)
{
	if (!HasAuthority() || ActionID == -1) return;

	ActionLogQueue.RemoveAll([ActionID](const FPEActionLogData& Data) { return Data.ActionID == ActionID; });
	OnRep_ActionLogQueue();
}
void APE_GameState::OnRep_ActionLogQueue()
{
	OnActionQueueUpdated.Broadcast(ActionLogQueue); // 클라이언트 BP 송출
}