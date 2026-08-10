// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Characters/PE_CharacterBase.h"
#include "PE_ObstacleBase.generated.h"

/**
 * 파괴 가능한 지형지물, 바리케이드, 폭약통 등의 부모 클래스입니다.
 */
UCLASS()
class PROJECT_ENTROPY_API APE_ObstacleBase : public APE_CharacterBase
{
	GENERATED_BODY()

public:
	APE_ObstacleBase();

protected:
	virtual void HandleDeath() override;
};