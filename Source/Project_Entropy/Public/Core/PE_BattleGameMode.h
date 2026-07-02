#pragma once

#include "CoreMinimal.h"
#include "Core/PE_GameMode.h"
#include "PE_BattleGameMode.generated.h"

class UPE_TurnManagerComponent;

UCLASS()
class PROJECT_ENTROPY_API APE_BattleGameMode : public APE_GameMode
{
	GENERATED_BODY()

public:
	APE_BattleGameMode();

	// 전역에서 쉽게 턴 매니저에 접근할 수 있도록 Getter 제공
	FORCEINLINE UPE_TurnManagerComponent* GetTurnManager() const { return TurnManager; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "System")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;
};