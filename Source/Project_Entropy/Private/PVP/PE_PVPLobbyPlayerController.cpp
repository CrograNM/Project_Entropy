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

void APE_PVPLobbyPlayerController::RequestTeamChange(int32 NewTeamID)
{
	if (APE_PlayerState* PS = GetPlayerState<APE_PlayerState>())
	{
		PS->SetTeamIDLocal(NewTeamID); // 즉시 0프레임 내에 UI가 갱신됨
	}

	Server_RequestTeamChange(NewTeamID);
}

bool APE_PVPLobbyPlayerController::Server_RequestTeamChange_Validate(int32 NewTeamID) { return true; }
void APE_PVPLobbyPlayerController::Server_RequestTeamChange_Implementation(int32 NewTeamID)
{
	if (APE_PlayerState* PS = GetPlayerState<APE_PlayerState>())
	{
		PS->SetTeamID(NewTeamID);
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
			// [추가됨] 맵을 넘어가기 직전, 접속해 있는 모든 클라이언트에게 UI를 부수라고 명령합니다.
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (APE_PVPLobbyPlayerController* PC = Cast<APE_PVPLobbyPlayerController>(It->Get()))
				{
					PC->Client_PrepareForTravel();
				}
			}

			// 안전하게 정리할 아주 짧은 시간을 준 뒤(0.1초) 서버 트래블을 실행합니다.
			FTimerHandle TravelTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TravelTimerHandle, [GM, MapName]()
				{
					GM->StartPvPMatch(MapName);
				}, 0.1f, false);
		}
	}
}

void APE_PVPLobbyPlayerController::Client_PrepareForTravel_Implementation()
{
	// 1. 델리게이트 바인딩을 모두 날려버려, 맵 파괴 중 위젯의 이벤트가 터지는 것을 막습니다.
	if (APE_PVPLobbyGameState* GS = GetWorld()->GetGameState<APE_PVPLobbyGameState>())
	{
		GS->OnLobbyUpdated.Clear();
	}

	// 2. 블루프린트로 만든 UI 위젯들이 있다면 화면에서 모두 제거(RemoveFromParent)하는 노드를 실행해야 합니다.
	// 블루프린트에서 이 함수를 이벤트를 잡아서 UI를 지우셔도 됩니다.
	// (예: 마우스 커서 숨기기 등 마무리 작업)
}