#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Core/PE_GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PE_RunManagerSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Controller.h"
#include "Engine/GameInstance.h"

APE_BattleGameMode::APE_BattleGameMode()
{
	CurrentState = EPEGameState::Battle;
}

void APE_BattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 전투 시작 전 스폰 위치를 미리 섞어둡니다.
	InitializeShuffledSpawns();

	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		if (UPE_TurnManagerComponent* TurnManager = GS->GetTurnManager())
		{
			TurnManager->StartBattle();
			UE_LOG(LogTemp, Warning, TEXT("[APE_BattleGameMode::BeginPlay] 턴 시스템 가동, 전투를 시작합니다."));
		}
	}
}

void APE_BattleGameMode::InitializeShuffledSpawns()
{
	if (ShuffledPlayerStarts.Num() > 0) return;

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPE_RunManagerSubsystem* RunManager = GI->GetSubsystem<UPE_RunManagerSubsystem>())
		{
			// 런 매니저의 시드 스트림을 사용해 PlayerStarts 배열을 섞습니다.
			// 시드가 같다면 항상 동일한 순서로 배열이 섞입니다.
			for (int32 i = PlayerStarts.Num() - 1; i > 0; --i)
			{
				int32 SwapIndex = RunManager->GetRandomIntInRange(0, i);
				PlayerStarts.Swap(i, SwapIndex);
			}
		}
	}

	ShuffledPlayerStarts = PlayerStarts;
}

AActor* APE_BattleGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 예외 처리: 아직 섞이지 않았다면 섞어줍니다.
	InitializeShuffledSpawns();

	if (Player == nullptr || ShuffledPlayerStarts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	FString PlayerId = Player->GetName();

	// 1. 이미 배정된 기록이 있다면 그 자리를 그대로 반환합니다 (재접속 등 보장)
	if (AActor** FoundStart = PlayerSpawnMap.Find(PlayerId))
	{
		return *FoundStart;
	}

	// 2. 완벽한 접속 순서 독립성을 보장하기 위한 슬롯 배정
	// TODO: 이 부분을 "플레이어의 고유 인덱스 (0, 1, 2, 3...)"를 가져오도록 수정해야 합니다.
	// 임시로 PlayerSpawnMap의 크기(접속 순서)를 사용하지만, 실제로는 게임의 로비 데이터 등을 참조해야 합니다.
	int32 PlayerFixedSlotIndex = PlayerSpawnMap.Num();

	if (ShuffledPlayerStarts.IsValidIndex(PlayerFixedSlotIndex))
	{
		AActor* ChosenStart = ShuffledPlayerStarts[PlayerFixedSlotIndex];
		PlayerSpawnMap.Add(PlayerId, ChosenStart);
		return ChosenStart;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}