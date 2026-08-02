// Copyright CrograNM

#include "PVP/PE_PVPLobbyPlayerController.h"
#include "Core/PE_PlayerState.h"
#include "PVP/PE_PVPLobbyGameState.h"
#include "PVP/PE_PVPLobbyGameMode.h"
#include "Kismet/GameplayStatics.h"

APE_PVPLobbyPlayerController::APE_PVPLobbyPlayerController()
{
	// 로비 UI 클릭을 위해 마우스 활성화
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void APE_PVPLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeUIOnly()); // 캐릭터 조작이 없으므로 UI 전용 모드
}

void APE_PVPLobbyPlayerController::HostLobby(FString MapName)
{
	// "LobbyMap"을 호스트(서버) 모드로 엽니다. (현재 레벨이 LobbyMap이라면 재시작 개념)
	UGameplayStatics::OpenLevel(this, FName(MapName), true, TEXT("listen"));
}

void APE_PVPLobbyPlayerController::JoinLobby(const FString& IPAddress)
{
	// 에디터 테스트 시에는 "127.0.0.1" (로컬호스트)를 사용하면 됩니다.
	ClientTravel(IPAddress, TRAVEL_Absolute);
}

void APE_PVPLobbyPlayerController::RequestTeamChange(int32 NewTeamID)
{
	Server_RequestTeamChange(NewTeamID);
}

bool APE_PVPLobbyPlayerController::Server_RequestTeamChange_Validate(int32 NewTeamID) { return true; }
void APE_PVPLobbyPlayerController::Server_RequestTeamChange_Implementation(int32 NewTeamID)
{
	if (APE_PlayerState* PS = GetPlayerState<APE_PlayerState>())
	{
		PS->SetTeamID(NewTeamID);

		// 누군가 팀을 바꿨으니 UI 새로고침 방송
		if (APE_PVPLobbyGameState* GS = GetWorld()->GetGameState<APE_PVPLobbyGameState>())
		{
			GS->NetMulticast_RefreshLobbyUI();
		}
	}
}

void APE_PVPLobbyPlayerController::RequestStartGame(FString MapName)
{
	Server_RequestStartGame(MapName);
}

bool APE_PVPLobbyPlayerController::Server_RequestStartGame_Validate(const FString& MapName) { return true; }
void APE_PVPLobbyPlayerController::Server_RequestStartGame_Implementation(const FString& MapName)
{
	// 호스트(방장)인지 검사합니다.
	if (HasAuthority())
	{
		if (APE_PVPLobbyGameMode* GM = Cast<APE_PVPLobbyGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->StartPvPMatch(MapName);
		}
	}
}