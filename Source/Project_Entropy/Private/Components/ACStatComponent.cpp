// Copyright CrograNM

#include "Components/ACStatComponent.h"

UACStatComponent::UACStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxHP = 100.f;
	CurrentHP = MaxHP;
	MaxAP = 3;
	CurrentAP = MaxAP;
	bIsDead = false;
}

void UACStatComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
	CurrentAP = MaxAP;
}

void UACStatComponent::TakeDamage(float Amount)
{
	if (bIsDead || Amount <= 0.f) return;

	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP, -Amount);

	if (CurrentHP <= 0.f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
	UE_LOG(LogTemp, Warning, TEXT("[UACStatComponent::TakeDamage] 체력 감소: %f, 현재 HP: %f"), Amount, CurrentHP);
}

void UACStatComponent::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f) return;

	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP, Amount);
	UE_LOG(LogTemp, Warning, TEXT("[UACStatComponent::Heal] 체력 회복: %f, 현재 HP: %f"), Amount, CurrentHP);
}

bool UACStatComponent::ConsumeAP(int32 Amount)
{
	if (bIsDead || Amount <= 0) return false;

	if (CurrentAP >= Amount)
	{
		CurrentAP -= Amount;
		OnAPChanged.Broadcast(CurrentAP, MaxAP, -Amount);
		return true;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[UACStatComponent::ConsumeAP] 행동력(AP)이 부족합니다!"));
	return false;
}

void UACStatComponent::ResetAP()
{
	if (bIsDead) return;

	int32 RecoveredAmount = MaxAP - CurrentAP;
	CurrentAP = MaxAP;
	OnAPChanged.Broadcast(CurrentAP, MaxAP, RecoveredAmount);
	UE_LOG(LogTemp, Warning, TEXT("[UACStatComponent::ResetAP] 행동력(AP) 초기화: 현재 AP: %d"), CurrentAP);
}