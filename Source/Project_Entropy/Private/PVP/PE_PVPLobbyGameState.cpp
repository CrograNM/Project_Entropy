// Copyright CrograNM

#include "PVP/PE_PVPLobbyGameState.h"

APE_PVPLobbyGameState::APE_PVPLobbyGameState()
{
}

void APE_PVPLobbyGameState::NetMulticast_RefreshLobbyUI_Implementation()
{
	// 모든 클라이언트(및 서버)에서 델리게이트를 발생시킵니다.
	// 블루프린트 UI 위젯은 이 델리게이트를 구독하여 PlayerArray를 순회하며 좌/우 패널을 다시 그리면 됩니다.
	OnLobbyUpdated.Broadcast();
}