// Copyright CrograNM

#include "Core/PE_TurnManagerComponent.h"
#include "Characters/PE_EnemyBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Core/PE_PlayerController.h"
#include "Core/PE_GameState.h"
#include "Components/ACStatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UPE_TurnManagerComponent::UPE_TurnManagerComponent()
{
	// 신호만 주고받으므로 틱 연산이 전혀 필요 없습니다. (최적화)
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 컴포넌트 동기화 활성화

	CurrentPhase = EPEBattlePhase::None;
	CurrentTurnCount = 0;
	CurrentTeamTurn = 0;
	MaxTeams = 2; // 기본 2파전 (0팀 vs 1팀)
}

void UPE_TurnManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 클라이언트들이 턴 수와 현재 페이즈를 알 수 있도록 복제
	DOREPLIFETIME(UPE_TurnManagerComponent, CurrentPhase);
	DOREPLIFETIME(UPE_TurnManagerComponent, CurrentTeamTurn);
	DOREPLIFETIME(UPE_TurnManagerComponent, CurrentTurnCount);
	DOREPLIFETIME(UPE_TurnManagerComponent, ReadyPlayerCount);
	DOREPLIFETIME(UPE_TurnManagerComponent, TotalPlayerCount);
	DOREPLIFETIME(UPE_TurnManagerComponent, bPendingTurnEnd);
}

void UPE_TurnManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPE_TurnManagerComponent::StartBattle()
{
	CurrentTurnCount = 1;
	UE_LOG(LogTemp, Warning, TEXT("[UPE_TurnManagerComponent::StartBattle] 전투가 시작되었습니다! 턴: %d"), CurrentTurnCount);

	CurrentTeamTurn = 0; // 항상 0팀부터 시작

	// 배틀 시작 세팅 페이즈 호출 후, 플레이어 턴으로 넘김
	ChangePhase(EPEBattlePhase::BattleStart);
	
	// ---- 연출용 딜레이 필요하면 타이머 걸기 ----
	ChangePhase(EPEBattlePhase::TeamTurn);
}

void UPE_TurnManagerComponent::EndCurrentPhase()
{
	// 서버에서만 실행되어야 함
	if (!GetOwner()->HasAuthority()) return;
	switch (CurrentPhase)
	{
	case EPEBattlePhase::TeamTurn:
	{
		// 1. 턴을 마치는 팀 소속 플레이어들의 조작 강제 취소
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
			{
				if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn()))
				{
					if (Char->GetTeamID() == CurrentTeamTurn)
					{
						PC->Client_CancelCurrentAction();
					}
				}
			}
		}

		// 2. 다음 팀으로 바톤 터치
		CurrentTeamTurn++;
		if (CurrentTeamTurn >= MaxTeams)
		{
			// 모든 팀의 턴이 끝났다면 환경 턴으로
			ChangePhase(EPEBattlePhase::EnvironmentTurn);
		}
		else
		{
			// 팀만 바뀌고 페이즈(TeamTurn)는 유지되므로, 델리게이트와 준비 함수를 수동 호출
			OnRep_CurrentTeamTurn();
			PrepareTeamTurn();
		}
		break;
	}

	case EPEBattlePhase::EnvironmentTurn:
		CurrentTurnCount++;
		CurrentTeamTurn = 0; // 다시 0팀 턴으로 복구
		UE_LOG(LogTemp, Warning, TEXT("[UPE_TurnManagerComponent::EndCurrentPhase] --- 새로운 턴 시작: %d ---"), CurrentTurnCount);
		ChangePhase(EPEBattlePhase::TeamTurn);
		break;

	default:
		break;
	}
}

void UPE_TurnManagerComponent::ChangePhase(EPEBattlePhase NewPhase)
{
	if (!GetOwner()->HasAuthority()) return; // 페이즈 변경은 서버만 가능
	if (CurrentPhase == NewPhase) return;

	EPEBattlePhase OldPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	
	// 서버 로컬 델리게이트 발동 (클라이언트는 OnRep_CurrentPhase에서 발동)
	OnRep_CurrentPhase(OldPhase);

	if (CurrentPhase == EPEBattlePhase::TeamTurn)
	{
		PrepareTeamTurn();
		OnRep_CurrentTeamTurn();
	}
	else if (CurrentPhase == EPEBattlePhase::EnvironmentTurn)
	{
		EndCurrentPhase();
	}
}

void UPE_TurnManagerComponent::PrepareTeamTurn()
{
	bPendingTurnEnd = false;
	ReadyPlayers.Empty();
	ReadyPlayerCount = 0;

	// 현재 턴인 팀 소속 플레이어들의 레디 상태만 초기화
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
		{
			if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn()))
			{
				if (Char->GetTeamID() == CurrentTeamTurn)
				{
					PC->Client_ResetReadyState();
				}
			}
		}
	}

	// AI 팀 행동 준비: 이번 턴 팀(CurrentTeamTurn)과 ID가 일치하는 몬스터들만 긁어모음
	EnemyQueue.Empty();
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_EnemyBase::StaticClass(), FoundEnemies);
	for (AActor* Actor : FoundEnemies)
	{
		if (APE_EnemyBase* Enemy = Cast<APE_EnemyBase>(Actor))
		{
			if (Enemy->GetTeamID() == CurrentTeamTurn && Enemy->GetStatComponent() && !Enemy->GetStatComponent()->IsDead())
			{
				EnemyQueue.Add(Enemy);
			}
		}
	}

	// 몬스터가 한 마리라도 있으면 큐 가동
	if (EnemyQueue.Num() > 0)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
	}
	else
	{
		// 몬스터가 없다면? 이 팀에 플레이어도 있는지 검사해본다. (완전 빈 팀인지 순수 유저 팀인지)
		int32 TeamPlayerCount = 0;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
			{
				if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn()))
				{
					if (Char->GetTeamID() == CurrentTeamTurn) TeamPlayerCount++;
				}
			}
		}

		if (TeamPlayerCount == 0)
		{
			// 유저도 없고 몬스터도 없는 빈 팀(Empty Team)이라면 허공에 턴을 낭비하지 않고 즉시 스킵
			UE_LOG(LogTemp, Warning, TEXT("[TurnManager] %d 팀에는 아무도 없어 턴을 스킵합니다."), CurrentTeamTurn);
			EndCurrentPhase();
		}
	}
}

void UPE_TurnManagerComponent::OnRep_CurrentPhase(EPEBattlePhase OldPhase)
{
	// 클라이언트 및 서버 모두 이 델리게이트를 통해 UI 갱신 등 반응
	OnPhaseChanged.Broadcast(CurrentPhase);
}

void UPE_TurnManagerComponent::OnRep_CurrentTeamTurn()
{
	UE_LOG(LogTemp, Warning, TEXT("[TurnManager] --- %d 팀의 턴이 시작되었습니다! ---"), CurrentTeamTurn);
	OnTeamTurnStarted.Broadcast(CurrentTeamTurn);
}

void UPE_TurnManagerComponent::RequestTurnEnd(APE_PlayerController* PC, bool bReady)
{
	if (!GetOwner()->HasAuthority()) return;

	if (bReady) ReadyPlayers.AddUnique(PC);
	else ReadyPlayers.Remove(PC);

	ReadyPlayerCount = ReadyPlayers.Num();
	EvaluateTurnEnd();
}

void UPE_TurnManagerComponent::EvaluateTurnEnd()
{
	TotalPlayerCount = 0;
	int32 ReadyTeamPlayerCount = 0;

	// [수정됨] 맵에 접속한 '모든' 유저가 아니라, '현재 턴을 진행 중인 팀'에 속한 유저만 필터링해서 만장일치를 검사합니다.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
		{
			if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn()))
			{
				if (Char->GetTeamID() == CurrentTeamTurn)
				{
					TotalPlayerCount++;
					if (ReadyPlayers.Contains(PC)) ReadyTeamPlayerCount++;
				}
			}
		}
	}

	if (TotalPlayerCount > 0 && ReadyTeamPlayerCount >= TotalPlayerCount)
	{
		bPendingTurnEnd = true;
		APE_GameState* GS = Cast<APE_GameState>(GetOwner());
		if (GS && !GS->IsActionQueueActive())
		{
			// ExecuteTurnEnd();
		}
	}
	else
	{
		bPendingTurnEnd = false;
	}
}

void UPE_TurnManagerComponent::ExecuteTurnEnd()
{
	if (!GetOwner()->HasAuthority()) return;

	bPendingTurnEnd = false;
	ReadyPlayers.Empty();
	ReadyPlayerCount = 0;

	EndCurrentPhase(); // 진짜 턴 종료 페이즈 진입
}

void UPE_TurnManagerComponent::ProcessNextEnemy()
{
	if (EnemyQueue.IsEmpty())
	{
		// AI 큐가 끝남. 만약 이 팀이 AI 전용 팀(플레이어 0명)이라면 자동으로 턴 종료를 넘겨줌.
		int32 TeamPlayerCount = 0;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APE_PlayerController* PC = Cast<APE_PlayerController>(It->Get()))
			{
				if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn()))
				{
					if (Char->GetTeamID() == CurrentTeamTurn) TeamPlayerCount++;
				}
			}
		}

		if (TeamPlayerCount == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TurnManager] %d 팀 소속 AI의 모든 행동 종료. 다음 턴으로 넘깁니다."), CurrentTeamTurn);
			EndCurrentPhase();
		}
		return;
	}

	APE_EnemyBase* NextEnemy = EnemyQueue[0];
	EnemyQueue.RemoveAt(0);

	if (IsValid(NextEnemy))
	{
		NextEnemy->OnTurnFinished.RemoveDynamic(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
		NextEnemy->OnTurnFinished.AddUniqueDynamic(this, &UPE_TurnManagerComponent::TriggerNextEnemyWithDelay);
		NextEnemy->StartTurn();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
	}
}

void UPE_TurnManagerComponent::TriggerNextEnemyWithDelay()
{
	// 이벤트가 발동한 직후, 즉시 ProcessNextEnemy를 실행하지 않고 
	// 현재 함수 실행 스택이 완전히 해제된 '다음 틱'에 실행되도록 예약합
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
}
