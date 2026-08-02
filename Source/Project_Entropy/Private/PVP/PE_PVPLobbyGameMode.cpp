// Copyright CrograNM

#include "PVP/PE_PVPLobbyGameMode.h"
#include "PVP/PE_PVPLobbyGameState.h"
#include "Core/PE_PlayerState.h"
#include "Core/PE_PlayerController.h"

APE_PVPLobbyGameMode::APE_PVPLobbyGameMode()
{
	GameStateClass = APE_PVPLobbyGameState::StaticClass();
	PlayerStateClass = APE_PlayerState::StaticClass();

	// 로비에서는 맵에 캐릭터가 스폰될 필요가 없으므로 관전자(Spectator) 상태로 둡니다.
	DefaultPawnClass = nullptr;

	// 로비에서 본 게임 맵으로 넘어갈 때 끊김 없이 다같이 넘어가기 위한 설정
	bUseSeamlessTravel = true;
}

void APE_PVPLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 접속한 유저에게 기본 팀(0팀) 배정
	if (APE_PlayerState* PS = NewPlayer->GetPlayerState<APE_PlayerState>())
	{
		PS->SetTeamID(0);
	}

	// 누군가 접속했으니 모든 클라이언트의 로비 UI를 새로고침하라고 지시
	if (APE_PVPLobbyGameState* GS = GetGameState<APE_PVPLobbyGameState>())
	{
		GS->NetMulticast_RefreshLobbyUI();
	}
}

void APE_PVPLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 누군가 나갔으니 모든 클라이언트의 로비 UI를 새로고침하라고 지시
	if (APE_PVPLobbyGameState* GS = GetGameState<APE_PVPLobbyGameState>())
	{
		GS->NetMulticast_RefreshLobbyUI();
	}
}

void APE_PVPLobbyGameMode::StartPvPMatch(FString MapName)
{
	// 지정된 맵으로 모두를 데리고 이동합니다. (경로는 프로젝트에 맞게 수정 필요)
	GetWorld()->ServerTravel(MapName + TEXT("?listen"), true);
}