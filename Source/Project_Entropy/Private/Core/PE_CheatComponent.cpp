// Copyright CrograNM

#include "Core/PE_CheatComponent.h"
#include "Core/PE_PlayerController.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACStatComponent.h"

UPE_CheatComponent::UPE_CheatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 헬퍼 매크로 (중복 코드 방지)
#define GET_STAT_COMP() \
	UACStatComponent* Stat = nullptr; \
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOwner())) { \
		if (APE_PlayerCharacter* Char = Cast<APE_PlayerCharacter>(PC->GetPawn())) { \
			Stat = Char->GetStatComponent(); \
		} \
	}

bool UPE_CheatComponent::Server_CheatSetHP_Validate(float NewHP) { return true; }
void UPE_CheatComponent::Server_CheatSetHP_Implementation(float NewHP)
{
	GET_STAT_COMP();
	if (Stat) Stat->SetHP(NewHP);
}

bool UPE_CheatComponent::Server_CheatSetMaxHP_Validate(float NewMaxHP) { return true; }
void UPE_CheatComponent::Server_CheatSetMaxHP_Implementation(float NewMaxHP)
{
	GET_STAT_COMP();
	if (Stat) Stat->SetMaxHP(NewMaxHP);
}

bool UPE_CheatComponent::Server_CheatSetAP_Validate(int32 NewAP) { return true; }
void UPE_CheatComponent::Server_CheatSetAP_Implementation(int32 NewAP)
{
	GET_STAT_COMP();
	if (Stat) Stat->SetAP(NewAP);
}

bool UPE_CheatComponent::Server_CheatSetMaxAP_Validate(int32 NewMaxAP) { return true; }
void UPE_CheatComponent::Server_CheatSetMaxAP_Implementation(int32 NewMaxAP)
{
	GET_STAT_COMP();
	if (Stat) Stat->SetMaxAP(NewMaxAP);
}

bool UPE_CheatComponent::Server_CheatSetMoveRange_Validate(int32 NewRange) { return true; }
void UPE_CheatComponent::Server_CheatSetMoveRange_Implementation(int32 NewRange)
{
	GET_STAT_COMP();
	if (Stat) Stat->SetMoveRange(NewRange);
}