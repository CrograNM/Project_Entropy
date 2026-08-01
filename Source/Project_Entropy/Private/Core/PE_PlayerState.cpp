// Copyright CrograNM

#include "Core/PE_PlayerState.h"
#include "Net/UnrealNetwork.h"

APE_PlayerState::APE_PlayerState()
{
	TeamID = 0; // 기본적으로 0팀(A팀/협동) 배정
}

void APE_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APE_PlayerState, TeamID);
}

void APE_PlayerState::SetTeamID(int32 InTeamID)
{
	if (HasAuthority())
	{
		TeamID = InTeamID;
		OnRep_TeamID();
	}
}

void APE_PlayerState::OnRep_TeamID()
{
	// 향후 UI(팀 색상 테두리 등) 갱신이 필요할 때 델리게이트를 호출할 수 있습니다.
	UE_LOG(LogTemp, Log, TEXT("[PlayerState] %s 의 팀이 %d 로 변경되었습니다."), *GetName(), TeamID);
}