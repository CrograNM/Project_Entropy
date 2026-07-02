#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Kismet/GameplayStatics.h"

APE_BattleGameMode::APE_BattleGameMode()
{
	// 턴 매니저 컴포넌트 생성 및 부착
	TurnManager = CreateDefaultSubobject<UPE_TurnManagerComponent>(TEXT("TurnManager"));
}

void APE_BattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (APE_PlayerController* PC = Cast<APE_PlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->SwitchInputMode(EPEGameState::Battle);
		UE_LOG(LogTemp, Warning, TEXT("[APE_BattleGameMode::BeginPlay] 플레이어 조작을 배틀 모드로 전환했습니다."));
	}

	// 맵 세팅이 끝났으므로 턴 시스템을 가동합니다.
	if (TurnManager)
	{
		TurnManager->StartBattle();
		UE_LOG(LogTemp, Warning, TEXT("[APE_BattleGameMode::BeginPlay] 턴 시스템 가동, 전투를 시작합니다."));
	}
}