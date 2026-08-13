// Copyright CrograNM

#include "Core/PE_GameState.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Characters/PE_CharacterBase.h"
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

void APE_GameState::ReportActionEnded()
{
	if (!HasAuthority()) return;

	PendingActionCount--;

	// [핵심] 꼬리를 무는 넉백을 포함해 파생된 모든 연산이 0이 될 때만 다음 큐로 넘어감
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

		// 큐가 완전히 비워진 순간, 턴 매니저가 만장일치로 대기 중이었다면 턴을 끝냅니다.
		if (TurnManager && TurnManager->IsPendingTurnEnd())
		{
			TurnManager->ExecuteTurnEnd();
		}
		return;
	}

	bIsProcessingAction = true;

	// 1차 스킬 본체의 액션 카운트 부여
	ReportActionStarted();

	FPESkillActionPayload Payload;
	ActionQueue.Dequeue(Payload);

	// 꺼낸 명세서를 바탕으로 스킬 실제 실행 지시
	if (Payload.Instigator && Payload.SkillData)
	{
		UACSkillComponent* SkillComp = Payload.Instigator->FindComponentByClass<UACSkillComponent>();
		if (SkillComp)
		{
			SkillComp->ExecuteQueuedSkill(Payload.SkillData, Payload.TargetTile, Payload.TargetCharacter, Payload.CalculatedDamage);
		}
		else ReportActionEnded();
	}
	else ReportActionEnded();
}