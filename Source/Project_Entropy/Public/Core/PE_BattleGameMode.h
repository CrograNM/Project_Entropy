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

protected:
	virtual void BeginPlay() override;
};