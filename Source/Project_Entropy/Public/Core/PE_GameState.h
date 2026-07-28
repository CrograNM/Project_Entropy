// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PE_GameState.generated.h"

class UPE_TurnManagerComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_GameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	APE_GameState();

	/** 전역에서 쉽게 턴 매니저에 접근할 수 있도록 Getter 제공 */
	FORCEINLINE UPE_TurnManagerComponent* GetTurnManager() const { return TurnManager; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;
};