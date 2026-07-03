// Copyright CrograNM

#include "Core/PE_TurnManagerComponent.h"

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
		ChangePhase(EPEBattlePhase::EnemyTurn);
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
}