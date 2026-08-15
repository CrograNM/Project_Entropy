// Copyright CrograNM

#include "Components/ACStatComponent.h"
#include "Net/UnrealNetwork.h"

UACStatComponent::UACStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 컴포넌트 복제 활성화

	MaxHP = 100.f;
	CurrentHP = MaxHP;
	MaxAP = 3;
	CurrentAP = MaxAP;
	bIsDead = false;
}

void UACStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UACStatComponent, bIsDead);
	DOREPLIFETIME(UACStatComponent, MaxHP);
	DOREPLIFETIME(UACStatComponent, CurrentHP);
	DOREPLIFETIME(UACStatComponent, MaxAP);
	DOREPLIFETIME(UACStatComponent, CurrentAP);
}

void UACStatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		CurrentHP = MaxHP;
		CurrentAP = MaxAP;
	}
}

void UACStatComponent::SetMaxHP(float InMaxHP)
{
	if (!GetOwner()->HasAuthority()) return;
	MaxHP = InMaxHP;
	OnHPChanged.Broadcast(CurrentHP, MaxHP, 0.f);
}

void UACStatComponent::SetHP(float InHP)
{
	if (!GetOwner()->HasAuthority()) return;
	CurrentHP = FMath::Clamp(InHP, 0.f, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, MaxHP, 0.f);
}

void UACStatComponent::SetMaxAP(int32 InMaxAP)
{
	if (!GetOwner()->HasAuthority()) return;
	MaxAP = InMaxAP;
	OnAPChanged.Broadcast(CurrentAP, MaxAP, 0);
}

void UACStatComponent::SetAP(int32 InAP)
{
	if (!GetOwner()->HasAuthority()) return;
	CurrentAP = InAP;
	OnAPChanged.Broadcast(CurrentAP, MaxAP, 0);
}

void UACStatComponent::TakeDamage(float Amount)
{
	if (!GetOwner()->HasAuthority() || bIsDead || Amount <= 0.f) return;

	float OldHP = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP - Amount, 0.f, MaxHP);

	// 서버 로컬 UI 갱신용 호출
	OnRep_CurrentHP(OldHP);

	if (CurrentHP <= 0.f)
	{
		bIsDead = true;
		OnRep_IsDead();
	}
	UE_LOG(LogTemp, Warning, TEXT("[UACStatComponent::TakeDamage] 체력 감소: %f, 현재 HP: %f"), Amount, CurrentHP);
}

void UACStatComponent::Heal(float Amount)
{
	if (!GetOwner()->HasAuthority() || bIsDead || Amount <= 0.f) return;

	float OldHP = CurrentHP;
	CurrentHP = FMath::Clamp(CurrentHP + Amount, 0.f, MaxHP);
	OnRep_CurrentHP(OldHP);
}

bool UACStatComponent::ConsumeAP(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || bIsDead) return false;

	Amount = FMath::Max(Amount, 0); // 음수 방지
	if (CurrentAP >= Amount)
	{
		int32 OldAP = CurrentAP;
		CurrentAP -= Amount;
		OnRep_CurrentAP(OldAP);
		return true;
	}

	return false;
}

void UACStatComponent::ResetAP()
{
	if (!GetOwner()->HasAuthority() || bIsDead) return;

	int32 OldAP = CurrentAP;
	CurrentAP = MaxAP;
	OnRep_CurrentAP(OldAP);
}

/* --- OnRep 함수들 (클라이언트 UI 갱신을 위해 델리게이트 방송) --- */
void UACStatComponent::OnRep_CurrentHP(float OldHP)
{
	OnHPChanged.Broadcast(CurrentHP, MaxHP, CurrentHP - OldHP);
}

void UACStatComponent::OnRep_CurrentAP(int32 OldAP)
{
	OnAPChanged.Broadcast(CurrentAP, MaxAP, CurrentAP - OldAP);
}

void UACStatComponent::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}