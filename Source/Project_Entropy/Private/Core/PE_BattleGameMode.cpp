#include "Core/PE_BattleGameMode.h"

#include "Core/PE_PlayerController.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Core/PE_GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PE_RunManagerSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Controller.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerState.h"

APE_BattleGameMode::APE_BattleGameMode()
{
	CurrentState = EPEGameState::Battle;
}

void APE_BattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 맵 로드 시 스폰 위치를 시드 기반으로 미리 섞어둡니다.
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
			// RunManager의 시드를 사용하여 위치 배열을 섞습니다.
			// 시드가 같다면 항상 똑같은 순서로 섞입니다.
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
	InitializeShuffledSpawns();

	if (Player == nullptr || ShuffledPlayerStarts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// PlayerState가 초기화되지 않은 시점을 대비하여 기본값을 컨트롤러 이름으로 설정합니다.
	FString PlayerId = Player->GetName();

	// PlayerState가 유효할 경우에만 UniqueId를 가져옵니다. 
	if (Player->PlayerState != nullptr)
	{
		PlayerId = Player->PlayerState->GetUniqueId().ToString();
	}

	// 1. 이미 배정된 기록이 있다면 그 자리를 반환 (재접속 보장)
	if (AActor** FoundStart = PlayerSpawnMap.Find(PlayerId))
	{
		return *FoundStart;
	}

	// 2. 플레이어 고유 ID와 시드를 조합하여 '선호하는 지정석(인덱스)'을 계산합니다.
	int32 PreferredIndex = 0;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPE_RunManagerSubsystem* RunManager = GI->GetSubsystem<UPE_RunManagerSubsystem>())
		{
			PreferredIndex = RunManager->GetPlayerRandomIntInRange(PlayerId, 0, ShuffledPlayerStarts.Num() - 1);
		}
	}

	// 3. 선호하는 자리가 비어있는지 확인하고, 만약 우연히 겹쳤다면 남은 빈자리를 순차 탐색합니다.
	int32 FinalIndex = -1;

	for (int32 i = 0; i < ShuffledPlayerStarts.Num(); ++i)
	{
		int32 CheckIndex = (PreferredIndex + i) % ShuffledPlayerStarts.Num();
		AActor* CheckStart = ShuffledPlayerStarts[CheckIndex];

		// 이 스폰 위치가 다른 플레이어에게 이미 배정되었는지 확인
		bool bIsOccupied = false;
		for (const auto& Elem : PlayerSpawnMap)
		{
			if (Elem.Value == CheckStart)
			{
				bIsOccupied = true;
				break;
			}
		}

		// 비어있다면 이 자리를 최종 자리로 결정
		if (!bIsOccupied)
		{
			FinalIndex = CheckIndex;
			break;
		}
	}

	// 4. 최종 결정된 자리를 배정 맵에 등록 후 반환
	if (FinalIndex != -1 && ShuffledPlayerStarts.IsValidIndex(FinalIndex))
	{
		AActor* ChosenStart = ShuffledPlayerStarts[FinalIndex];
		PlayerSpawnMap.Add(PlayerId, ChosenStart);
		return ChosenStart;
	}

	// 실패 시 기본 로직
	return Super::ChoosePlayerStart_Implementation(Player);
}