#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Core/PE_GameState.h"
#include "Kismet/GameplayStatics.h"

APE_BattleGameMode::APE_BattleGameMode()
{
}

void APE_BattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EPEGameState::Battle; // 전투 모드로 상태 변경

	if (APE_PlayerController* PC = Cast<APE_PlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->SwitchInputMode(EPEGameState::Battle);
	}

	// GameState를 거쳐 TurnManager 가동
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		if (UPE_TurnManagerComponent* TurnManager = GS->GetTurnManager())
		{
			TurnManager->StartBattle();
			UE_LOG(LogTemp, Warning, TEXT("[APE_BattleGameMode::BeginPlay] 턴 시스템 가동, 전투를 시작합니다."));
		}
	}
}