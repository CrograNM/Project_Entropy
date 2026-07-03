// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PE_GameMode.generated.h"

UENUM(BlueprintType)
enum class EPEGameState : uint8
{
	Base,   // 기지, 상점 등 직접 이동 모드
	Battle  // 카드 배틀 모드
};

UCLASS()
class PROJECT_ENTROPY_API APE_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APE_GameMode();

protected:
	virtual void BeginPlay() override;

	// 현재 게임의 상태 (Base / Battle)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game State")
	EPEGameState CurrentState;

public:
	// Base / Battle 상태 등 가져오기
	FORCEINLINE EPEGameState GetCurrentState() const { return CurrentState; }
};
