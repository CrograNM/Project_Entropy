// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PE_PVPLobbyGameMode.generated.h"

UCLASS()
class PROJECT_ENTROPY_API APE_PVPLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APE_PVPLobbyGameMode();

protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

public:
	// 방장이 게임 시작(Play) 버튼을 눌렀을 때 호출
	void StartPvPMatch(FString MapName);
};