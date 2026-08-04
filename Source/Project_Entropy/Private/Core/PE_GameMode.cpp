// Copyright CrograNM


#include "Core/PE_GameMode.h"
#include "Core/PE_PlayerController.h"
#include "Kismet/GameplayStatics.h"

APE_GameMode::APE_GameMode()
{
	// 기본 상태: 기지(Base) 모드로 설정
	CurrentState = EPEGameState::Base;

	bUseSeamlessTravel = true;
}

void APE_GameMode::BeginPlay()
{
	Super::BeginPlay();
}

void APE_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}