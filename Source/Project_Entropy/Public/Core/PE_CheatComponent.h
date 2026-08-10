// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PE_CheatComponent.generated.h"

/**
 * 치트 매니저의 명령을 서버로 전달하는 전용 네트워크 컴포넌트입니다.
 * PlayerController의 코드 비대화를 막기 위해 분리되었습니다.
 */
UCLASS(ClassGroup = (Cheat), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UPE_CheatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPE_CheatComponent();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetHP(float NewHP);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetMaxHP(float NewMaxHP);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetAP(int32 NewAP);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetMaxAP(int32 NewMaxAP);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetMoveRange(int32 NewRange);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatSetTileObstacle(class AACTile* TargetTile, bool bIsObstacle);
};