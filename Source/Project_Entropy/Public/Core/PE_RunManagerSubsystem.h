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

	/** 게임 시작 시 새로운 런의 시드를 주입하고 난수 생성기를 초기화합니다. */
	UFUNCTION(BlueprintCallable, Category = "Run|Random")
	void StartNewRunWithSeed(int32 NewSeed);

	/** 시드 기반 랜덤 정수 반환 (Min <= 반환값 <= Max) */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	int32 GetRandomIntInRange(int32 Min, int32 Max) const;

	/** 시드 기반 랜덤 실수 반환 */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	float GetRandomFloatInRange(float Min, float Max) const;

	/** 시드 기반 랜덤 참/거짓 반환 (50% 확률) */
	UFUNCTION(BlueprintPure, Category = "Run|Random")
	bool GetRandomBool() const;

	/** 현재 런의 시드 번호 확인 (UI 표시용 등) */
	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentSeed() const { return CurrentSeed; }

private:
	UPROPERTY()
	int32 CurrentSeed;

	/** 언리얼 엔진의 시드 기반 난수 생성 스트림 */
	FRandomStream RunRandomStream;
};