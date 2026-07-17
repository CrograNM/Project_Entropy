// Copyright CrograNM

#include "Core/PE_CheatManager.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_PlayerController.h"
#include "GameFramework/PlayerController.h"

UACStatComponent* UPE_CheatManager::GetPlayerStatComponent() const
{
	if (APlayerController* PC = GetOuterAPlayerController())
	{
		if (APawn* PlayerPawn = PC->GetPawn())
		{
			return PlayerPawn->FindComponentByClass<UACStatComponent>();
		}
	}
	return nullptr;
}

void UPE_CheatManager::SetHP(float NewHP)
{
	if (UACStatComponent* Stat = GetPlayerStatComponent())
	{
		Stat->SetHP(NewHP);
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] 플레이어 HP를 %f(으)로 변경했습니다."), NewHP);
	}
}

void UPE_CheatManager::SetMaxHP(float NewMaxHP)
{
	if (UACStatComponent* Stat = GetPlayerStatComponent())
	{
		Stat->SetMaxHP(NewMaxHP);
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] 플레이어 최대 HP를 %f(으)로 변경했습니다."), NewMaxHP);
	}
}

void UPE_CheatManager::SetAP(int32 NewAP)
{
	if (UACStatComponent* Stat = GetPlayerStatComponent())
	{
		Stat->SetAP(NewAP);
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] 플레이어 AP를 %d(으)로 변경했습니다."), NewAP);
	}
}

void UPE_CheatManager::SetMaxAP(int32 NewMaxAP)
{
	if (UACStatComponent* Stat = GetPlayerStatComponent())
	{
		Stat->SetMaxAP(NewMaxAP);
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] 플레이어 최대 AP를 %d(으)로 변경했습니다."), NewMaxAP);
	}
}

void UPE_CheatManager::SetMoveRange(int32 NewRange)
{
	if (UACStatComponent* Stat = GetPlayerStatComponent())
	{
		Stat->SetMoveRange(NewRange);
		UE_LOG(LogTemp, Warning, TEXT("[Cheat] 플레이어 이동 사거리를 %d(으)로 변경했습니다."), NewRange);
	}
}

void UPE_CheatManager::SetMapToolActive(bool bActive)
{
	bIsMapToolActive = bActive;

	// 툴이 켜지는 순간, 플레이어의 모든 조작(이동 모드, 호버링 등)을 강제로 취소
	if (bIsMapToolActive)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
		{
			PC->CancelCurrentAction();
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 디버그 맵 툴 활성화: 플레이어 조작을 차단합니다."));
		}
	}
	
	OnMapToolStateChanged.Broadcast(bIsMapToolActive);
}
