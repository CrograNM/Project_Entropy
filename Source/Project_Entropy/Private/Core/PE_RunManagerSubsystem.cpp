// Copyright CrograNM

#include "Core/PE_RunManagerSubsystem.h"

void UPE_RunManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPE_RunManagerSubsystem::StartNewRunWithSeed(int32 NewSeed)
{
	CurrentSeed = NewSeed;
	RunRandomStream.Initialize(CurrentSeed);

	UE_LOG(LogTemp, Warning, TEXT("[RunManager] 새로운 런 시작. 현재 시드: %d"), CurrentSeed);
}

int32 UPE_RunManagerSubsystem::GetRandomIntInRange(int32 Min, int32 Max) const
{
	// FRandomStream이 알아서 내부 시퀀스를 전진시키고 난수를 반환합니다.
	return RunRandomStream.RandRange(Min, Max);
}

float UPE_RunManagerSubsystem::GetRandomFloatInRange(float Min, float Max) const
{
	return RunRandomStream.FRandRange(Min, Max);
}

bool UPE_RunManagerSubsystem::GetRandomBool() const
{
	return (RunRandomStream.RandRange(0, 1) == 1);
}