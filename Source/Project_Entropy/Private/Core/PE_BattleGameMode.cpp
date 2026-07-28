#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Core/PE_GameState.h"
#include "Kismet/GameplayStatics.h"

APE_BattleGameMode::APE_BattleGameMode()
{
	// 생성자에서 상태를 Battle로 미리 덮어씌움 (이후 PostLogin에서 이 상태를 기반으로 RPC 발송)
	CurrentState = EPEGameState::Battle;
}

void APE_BattleGameMode::BeginPlay()
{
	Super::BeginPlay();

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