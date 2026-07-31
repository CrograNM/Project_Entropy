// Copyright CrograNM

#include "Core/PE_RunManagerSubsystem.h"

void UPE_RunManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPE_RunManagerSubsystem::StartRandomRun()
{
	// 현재 기기의 시간(Ticks)과 기존 랜덤 함수를 섞어 최대한 예측 불가능한 시드 생성
	int32 GeneratedSeed = FMath::Rand() ^ FDateTime::Now().GetTicks();

	CurrentSeed = GeneratedSeed;
	RunRandomStream.Initialize(CurrentSeed);

	UE_LOG(LogTemp, Warning, TEXT("[RunManager] 무작위 런 시작. 생성된 시드: %d"), CurrentSeed);
}

void UPE_RunManagerSubsystem::StartSeededRun(const FString& SeedString)
{
	// 입력된 문자열의 공백을 제거하고 대문자로 통일
	FString CleanString = SeedString.TrimStartAndEnd().ToUpper();

	// 문자열이 비어있다면 무작위 런으로 대체
	if (CleanString.IsEmpty())
	{
		StartRandomRun();
		return;
	}

	// 문자열을 고유한 정수 해시(CRC32)로 변환하여 시드로 사용
	int32 HashedSeed = FCrc::StrCrc32(*CleanString);

	CurrentSeed = HashedSeed;
	RunRandomStream.Initialize(CurrentSeed);

	UE_LOG(LogTemp, Warning, TEXT("[RunManager] 시드 기반 런 시작. 원본 문자열: %s, 변환된 시드: %d"), *CleanString, CurrentSeed);
}

int32 UPE_RunManagerSubsystem::GetRandomIntInRange(int32 Min, int32 Max) const
{
	// FRandomStream이 알아서 내부 시퀀스를 전진시키고 난수를 반환합니다.
	return RunRandomStream.RandRange(Min, Max);
}

int32 UPE_RunManagerSubsystem::GetPlayerRandomIntInRange(const FString& PlayerUniqueId, int32 Min, int32 Max) const
{
	// 메인 런 시드와 플레이어 고유 문자열의 해시를 조합하여 플레이어만의 고유 시드 생성
	int32 PlayerSpecificSeed = CurrentSeed ^ FCrc::StrCrc32(*PlayerUniqueId);

	// 이 플레이어만을 위한 일회성 난수 스트림 생성 및 초기화
	FRandomStream PlayerStream;
	PlayerStream.Initialize(PlayerSpecificSeed);

	// 접속 순서와 무관하게 이 플레이어에게만 고정된 난수 반환
	return PlayerStream.RandRange(Min, Max);
}

float UPE_RunManagerSubsystem::GetRandomFloatInRange(float Min, float Max) const
{
	return RunRandomStream.FRandRange(Min, Max);
}

bool UPE_RunManagerSubsystem::GetRandomBool() const
{
	return (RunRandomStream.RandRange(0, 1) == 1);
}

FString UPE_RunManagerSubsystem::GetCurrentSeedAsString() const
{
	return FString::Printf(TEXT("%08X"), CurrentSeed);
}
