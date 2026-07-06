// Copyright CrograNM

#include "Core/PE_TurnManagerComponent.h"

#include "Characters/PE_EnemyBase.h"
#include "Core/PE_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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
	
	// 일단 환경 턴은 스킵 (임시)
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

	// 다음 틱 부터 ProcessNextEnemy를 호출하여 적들의 행동을 순차적으로 처리
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
}

void UPE_TurnManagerComponent::ProcessNextEnemy()
{
	if (EnemyQueue.IsEmpty())
	{
		// 큐가 비었다면 모든 적이 행동을 마친 것이므로 환경(Environment) 턴으로 넘김
		UE_LOG(LogTemp, Warning, TEXT("[TurnManager] 모든 적의 행동이 종료되었습니다."));
		CurrentPhase = EPEBattlePhase::EnemyTurn; // 상태 무결성 보장
		EndCurrentPhase(); 
		return;
	}

	// 큐에서 맨 앞의 적을 하나 뽑아냄
	APE_EnemyBase* NextEnemy = EnemyQueue[0];
	EnemyQueue.RemoveAt(0);

	if (IsValid(NextEnemy))
	{
		// 이전 적이 등록했을 수 있는 델리게이트를 깔끔히 정리하여 중복 트리거를 방지합니다.
		NextEnemy->OnTurnFinished.RemoveDynamic(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
		
		// 이번 적이 끝나면 다음 틱에 안전하게 ProcessNextEnemy가 예약되도록 람다 함수나 단일 타겟 바인딩을 활용합니다.
		// 동기식 재귀 호출을 원천 차단하기 위해 델리게이트 수신 시 '한 프레임 미뤄서' 실행하도록 유도합니다.
		NextEnemy->OnTurnFinished.AddUniqueDynamic(this, &UPE_TurnManagerComponent::TriggerNextEnemyWithDelay);
		
		// 적 턴 가동
		NextEnemy->StartTurn();
	}
	else
	{
		// 죽었거나 소멸한 적이라면 스택을 쌓지 않고 다음 틱에 다음 적 처리
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
	}
}

void UPE_TurnManagerComponent::TriggerNextEnemyWithDelay()
{
	// 이벤트가 발동한 직후, 즉시 ProcessNextEnemy를 실행하지 않고 
	// 현재 함수 실행 스택이 완전히 해제된 '다음 틱'에 실행되도록 예약합
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UPE_TurnManagerComponent::ProcessNextEnemy);
}
