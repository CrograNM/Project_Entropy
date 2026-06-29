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
	
	// 시작 시 현재 상태에 맞춰 플레이어 컨트롤러의 입력 모드를 세팅 유도
	APE_PlayerController* PC = Cast<APE_PlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (PC)
	{
		PC->SwitchInputMode(CurrentState);
	}
}
