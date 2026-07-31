#pragma once

#include "CoreMinimal.h"
#include "Core/PE_GameMode.h"
#include "PE_BattleGameMode.generated.h"

class UPE_TurnManagerComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_BattleGameMode : public APE_GameMode
{
	GENERATED_BODY()

public:
	APE_BattleGameMode();

protected:
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	// 시드 기반으로 순서가 섞인 PlayerStart 목록
	UPROPERTY()
	TArray<AActor*> ShuffledPlayerStarts;

	// 이미 스폰 위치를 배정받은 플레이어 추적용 맵 (재접속 대비)
	UPROPERTY()
	TMap<FString, AActor*> PlayerSpawnMap;

	// 스폰 위치들을 시드 기반으로 섞는 함수
	void InitializeShuffledSpawns();
};