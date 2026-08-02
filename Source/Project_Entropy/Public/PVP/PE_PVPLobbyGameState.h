// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "PE_PVPLobbyGameState.generated.h"

// 로비 인원이나 팀이 변경되었을 때 UI 위젯을 갱신하기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyUpdatedSignature);

UCLASS()
class PROJECT_ENTROPY_API APE_PVPLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	APE_PVPLobbyGameState();

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnLobbyUpdatedSignature OnLobbyUpdated;

	// 서버에서 인원 변동/팀 변경 시 호출하면 모든 클라이언트의 UI가 갱신됨
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_RefreshLobbyUI();
};