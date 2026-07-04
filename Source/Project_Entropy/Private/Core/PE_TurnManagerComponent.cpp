// Copyright CrograNM

#include "Core/PE_TurnManagerComponent.h"

#include "Characters/PE_EnemyBase.h"
#include "Core/PE_PlayerController.h"
#include "Kismet/GameplayStatics.h"

UPE_TurnManagerComponent::UPE_TurnManagerComponent()
{
	// 신호만 주고받으므로 틱 연산이 전혀 필요 없습니다. (최적화)
	PrimaryComponentTick.bCanEverTick = false;
	
	CurrentPhase = EPEBattlePhase::None;
	CurrentTurnCount = 0;
}

void UPE_TurnManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPE_TurnManagerComponent::StartBattle()
{
	CurrentTurnCount = 1;
	UE_LOG(LogTemp, Warning, TEXT("[UPE_TurnManagerComponent::StartBattle] 전투가 시작되었습니다! 턴: %d"), CurrentTurnCount);
	
	// 배틀 시작 세팅 페이즈 호출 후, 플레이어 턴으로 넘김
	ChangePhase(EPEBattlePhase::BattleStart);
	
	// ---- 연출용 딜레이 필요하면 타이머 걸기 ----
	ChangePhase(EPEBattlePhase::PlayerTurn);
}

void UPE_TurnManagerComponent::EndCurrentPhase()
{
	APE_PlayerController* PC = Cast<APE_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	// 현재 페이즈에 따라 다음 페이즈로 바통을 넘깁니다.
	switch (CurrentPhase)
	{
	case EPEBattlePhase::PlayerTurn:
		if (PC)
		{
			// 플레이어 턴 종료 시, 이동 모드가 켜져있다면 강제로 끄고 사거리 하이라이트를 초기화
			PC->CancelCurrentAction();
		}
		StartEnemyPhase();
		break;

	case EPEBattlePhase::EnemyTurn:
		ChangePhase(EPEBattlePhase::EnvironmentTurn);
		break;

	case EPEBattlePhase::EnvironmentTurn:
		// 한 바퀴 사이클이 끝나면 턴 수를 올리고 다시 플레이어 턴으로
		CurrentTurnCount++;
		UE_LOG(LogTemp, Warning, TEXT("[UPE_TurnManagerComponent::EndCurrentPhase] --- 새로운 턴 시작: %d ---"), CurrentTurnCount);
		ChangePhase(EPEBattlePhase::PlayerTurn);
		break;

	default:
		break;
	}
}

void UPE_TurnManagerComponent::ChangePhase(EPEBattlePhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;

	CurrentPhase = NewPhase;
	
	// Enum 이름을 String으로 변환하여 깔끔하게 로그 출력
	FString PhaseName = UEnum::GetValueAsString(CurrentPhase);
	UE_LOG(LogTemp, Warning, TEXT("[UPE_TurnManagerComponent::ChangePhase] 페이즈 변경: %s"), *PhaseName);

	// 이 델리게이트가 호출되면, 구독하고 있는 모든 객체가 일제히 반응합니다.
	OnPhaseChanged.Broadcast(CurrentPhase);
	
	// -------
	// 테스트를 위해 환경(Environment) 턴이 시작되면 즉시 종료하도록 설정 -> 아직 환경 턴 로직이 구현되지 않음
	// -------
	if (CurrentPhase == EPEBattlePhase::EnvironmentTurn)
	{
		EndCurrentPhase();
	}
}

void UPE_TurnManagerComponent::StartEnemyPhase()
{
	ChangePhase(EPEBattlePhase::EnemyTurn);

	EnemyQueue.Empty();
	
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_EnemyBase::StaticClass(), FoundEnemies);

	for (AActor* Actor : FoundEnemies)
	{
		if (APE_EnemyBase* Enemy = Cast<APE_EnemyBase>(Actor))
		{
			EnemyQueue.Add(Enemy);
		}
	}

	// 수집 완료 후 첫 번째 적부터 순차 실행 시작
	ProcessNextEnemy();
}

void UPE_TurnManagerComponent::ProcessNextEnemy()
{
	if (EnemyQueue.IsEmpty())
	{
		// 큐가 비었다면 모든 적이 행동을 마친 것이므로 환경(Environment) 턴으로 넘김
		UE_LOG(LogTemp, Warning, TEXT("[TurnManager] 모든 적의 행동이 종료되었습니다."));
		EndCurrentPhase(); 
		return;
	}

	// 큐에서 맨 앞의 적을 하나 뽑아냄
	APE_EnemyBase* NextEnemy = EnemyQueue[0];
	EnemyQueue.RemoveAt(0);

	if (NextEnemy)
	{
		// 이 적의 행동이 끝날 때 다시 나(TurnManager)의 ProcessNextEnemy를 호출하도록 1회용 콜백 연결
		NextEnemy->OnTurnFinished.AddDynamic(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
		
		// 적 턴 시작 (이 안에서 비동기로 움직이고 공격한 뒤 OnTurnFinished를 Broadcast 함)
		NextEnemy->StartTurn();
	}
	else
	{
		// 적이 이미 파괴되었거나 유효하지 않다면 다음 적으로 바로 스킵
		ProcessNextEnemy();
	}
}
