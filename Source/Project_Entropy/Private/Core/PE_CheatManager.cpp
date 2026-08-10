// Copyright CrograNM

#include "Core/PE_CheatManager.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_PlayerController.h"
#include "Core/PE_CheatComponent.h"
#include "GameFramework/PlayerController.h"

void UPE_CheatManager::SetHP(float NewHP)
{
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
	{
		if (UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>())
		{
			CheatNet->Server_CheatSetHP(NewHP);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 서버에 HP 변경을 요청했습니다: %f"), NewHP);
		}
	}
}

void UPE_CheatManager::SetMaxHP(float NewMaxHP)
{
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
	{
		if (UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>())
		{
			CheatNet->Server_CheatSetMaxHP(NewMaxHP);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 서버에 최대 HP 변경을 요청했습니다: %f"), NewMaxHP);
		}
	}
}

void UPE_CheatManager::SetAP(int32 NewAP)
{
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
	{
		if (UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>())
		{
			CheatNet->Server_CheatSetAP(NewAP);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 서버에 AP 변경을 요청했습니다: %d"), NewAP);
		}
	}
}

void UPE_CheatManager::SetMaxAP(int32 NewMaxAP)
{
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
	{
		if (UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>())
		{
			CheatNet->Server_CheatSetMaxAP(NewMaxAP);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 서버에 최대 AP 변경을 요청했습니다: %d"), NewMaxAP);
		}
	}
}

void UPE_CheatManager::SetMoveRange(int32 NewRange)
{
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOuterAPlayerController()))
	{
		if (UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>())
		{
			CheatNet->Server_CheatSetMoveRange(NewRange);
			UE_LOG(LogTemp, Warning, TEXT("[Cheat] 서버에 이동 사거리 변경을 요청했습니다: %d"), NewRange);
		}
	}
}

void UPE_CheatManager::SetMapToolActive(bool bActive)
{
	bIsMapToolActive = bActive;

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