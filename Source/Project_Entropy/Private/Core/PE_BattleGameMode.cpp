#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Core/PE_GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PE_RunManagerSubsystem.h"
#include "GameFramework/PlayerStart.h"

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

AActor* APE_BattleGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 1. 레벨 내의 모든 PlayerStart 액터를 찾아서 배열에 담습니다.
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);

	// 배치된 PlayerStart가 없다면 부모 클래스의 기본 로직(0,0,0 월드 원점 등)을 따릅니다.
	if (FoundActors.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// 2. GameInstance를 통해 런 매니저 서브시스템을 가져옵니다.
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		UPE_RunManagerSubsystem* RunManager = GameInstance->GetSubsystem<UPE_RunManagerSubsystem>();
		if (RunManager)
		{
			// 3. 런 매니저의 시드 기반 스트림을 사용하여 무작위 인덱스를 결정합니다.
			int32 MaxIndex = FoundActors.Num() - 1;
			int32 RandomIndex = RunManager->GetRandomIntInRange(0, MaxIndex);

			UE_LOG(LogTemp, Log, TEXT("[GameMode] 시드 기반 플레이어 스타트 선택됨. 인덱스: %d / 총 개수: %d"), RandomIndex, FoundActors.Num());

			return FoundActors[RandomIndex];
		}
	}

	// 서브시스템을 찾지 못한 경우 안전장치로 부모의 기본 로직을 실행합니다.
	return Super::ChoosePlayerStart_Implementation(Player);
}
