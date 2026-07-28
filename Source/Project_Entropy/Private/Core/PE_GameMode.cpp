// Copyright CrograNM


#include "Core/PE_GameMode.h"
#include "Core/PE_PlayerController.h"
#include "Kismet/GameplayStatics.h"

APE_GameMode::APE_GameMode()
{
	// 기본 상태: 기지(Base) 모드로 설정
	CurrentState = EPEGameState::Base;
}

void APE_GameMode::BeginPlay()
{
	Super::BeginPlay();
}

void APE_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 접속한 플레이어(호스트 및 클라이언트 모두 포함)에게 현재 게임 모드에 맞는 인풋 상태를 RPC로 전송
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(NewPlayer))
	{
		PC->Client_SetupInputMode(CurrentState);
		UE_LOG(LogTemp, Warning, TEXT("[APE_GameMode] 플레이어 %s 접속, 인풋 모드 갱신 요청."), *PC->GetName());
	}
}