// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PE_PVPLobbyPlayerController.generated.h"

UCLASS()
class PROJECT_ENTROPY_API APE_PVPLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APE_PVPLobbyPlayerController();

	// --- [UI 연동: 로비 조작] ---

	// 왼쪽/오른쪽 패널을 눌러 팀을 바꿀 때 호출
	UFUNCTION(BlueprintCallable, Category = "Lobby|Action")
	void RequestTeamChange(int32 NewTeamID);

	// 서버로 팀 변경을 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestTeamChange(int32 NewTeamID);

	// 'Play' 버튼을 눌러 게임 시작 (방장만 가능)
	UFUNCTION(BlueprintCallable, Category = "Lobby|Action")
	void RequestStartGame(FString MapName);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestStartGame(const FString& MapName);

	UFUNCTION(Client, Reliable)
	void Client_PrepareForTravel();

protected:
	virtual void BeginPlay() override;
};