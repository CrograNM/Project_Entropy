// Copyright CrograNM

#include "Core/PE_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "PVP/PE_PVPLobbyGameState.h"

APE_PlayerState::APE_PlayerState()
{
	TeamID = 0; // 기본적으로 0팀(A팀/협동) 배정
}

void APE_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APE_PlayerState, TeamID);
}

void APE_PlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (APE_PlayerState* NewPS = Cast<APE_PlayerState>(PlayerState))
	{
		// 로비에서 들고 있던 TeamID를 전투 맵의 새 PlayerState에 복사합니다.
		NewPS->TeamID = this->TeamID;
	}
}

void APE_PlayerState::SetTeamID(int32 InTeamID)
{
	if (HasAuthority())
	{
		TeamID = InTeamID;
		OnRep_TeamID();
	}
}

// [추가됨] 클라이언트가 통신 대기 없이 강제로 변수를 바꾸고 UI를 새로고침하게 만드는 함수
void APE_PlayerState::SetTeamIDLocal(int32 InTeamID)
{
	TeamID = InTeamID;
	OnRep_TeamID(); // 즉각적인 UI 갱신 방송 트리거
}

void APE_PlayerState::OnRep_TeamID()
{
	// 향후 UI(팀 색상 테두리 등) 갱신이 필요할 때 델리게이트를 호출할 수 있습니다.
	UE_LOG(LogTemp, Log, TEXT("[PlayerState] %s 의 팀이 %d 로 변경되었습니다."), *GetName(), TeamID);

	if (APE_PVPLobbyGameState* GS = GetWorld()->GetGameState<APE_PVPLobbyGameState>())
	{
		GS->OnLobbyUpdated.Broadcast();
	}
}