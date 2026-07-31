// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Math/RandomStream.h"
#include "PE_RunManagerSubsystem.generated.h"

UCLASS()
class PROJECT_ENTROPY_API UPE_RunManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** [게임시작] 무작위 시드로 완전히 새로운 런을 시작합니다. (일반런) */
	UFUNCTION(BlueprintCallable, Category = "Run|System")
	void StartRandomRun();

	/** [게임시작] 유저가 입력한 특정 시드를 기반으로 런을 시작합니다. (시드런) */
	UFUNCTION(BlueprintCallable, Category = "Run|System")
	void StartSeededRun(const FString& SeedString);

	/** 시드 기반 랜덤 정수 반환 (Min <= 반환값 <= Max) */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	int32 GetRandomIntInRange(int32 Min, int32 Max) const;

	/** 시드 기반 랜덤 정수 반환 (플레이어 고유 ID를 기반으로 한 고정 난수) */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	int32 GetPlayerRandomIntInRange(const FString& PlayerUniqueId, int32 Min, int32 Max) const;

	/** 시드 기반 랜덤 실수 반환 */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	float GetRandomFloatInRange(float Min, float Max) const;

	/** 시드 기반 랜덤 참/거짓 반환 (50% 확률) */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	bool GetRandomBool() const;

	/** 현재 런의 시드 번호 확인 (UI 표시용 등) */
	UFUNCTION(BlueprintPure, Category = "Run|State")
	int32 GetCurrentSeed() const { return CurrentSeed; }

	/** 현재 런의 시드를 문자열 포맷(예: 8자리 헥사코드)으로 반환 (UI 표시용) */
	UFUNCTION(BlueprintPure, Category = "Run|State")
	FString GetCurrentSeedAsString() const;

private:
	UPROPERTY()
	int32 CurrentSeed;

	/** 언리얼 엔진의 시드 기반 난수 생성 스트림 */
	FRandomStream RunRandomStream;
};