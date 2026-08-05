// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/ACCameraControlComponent.h"
#include "Components/ACStatComponent.h"
#include "Components/ACTargetingVisualizerComponent.h"
#include "Core/PE_BattleGameMode.h"
#include "Core/PE_GameState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

APE_PlayerCharacter::APE_PlayerCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	CameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("CameraBase"));
	CameraBase->SetupAttachment(RootComponent);
	CameraBase->SetUsingAbsoluteRotation(true);
	
	// 스프링암 컴포넌트 생성 및 설정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CameraBase);
	CameraBoom->TargetArmLength = 1000.f; 
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); 
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bUsePawnControlRotation = false;
	
	// 카메라 컴포넌트 생성 및 설정
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	
	// 카메라 제어 컴포넌트 생성
	CameraControlComponent = CreateDefaultSubobject<UACCameraControlComponent>(TEXT("CameraControlComponent"));

	TargetingVisualizer = CreateDefaultSubobject<UACTargetingVisualizerComponent>(TEXT("TargetingVisualizer"));
}

void APE_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 카메라 제어 컴포넌트 초기화
	if (CameraControlComponent)
	{
		CameraControlComponent->InitCameraComponents(CameraBase, CameraBoom, TopDownCamera);
	}
	
	// TurnManager의 페이즈 변경 신호를 구독하여 내 턴 시작 시 AP를 회복하도록 설정
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		if (UPE_TurnManagerComponent* TurnManager = GS->GetTurnManager())
		{
			TurnManager->OnTeamTurnStarted.AddDynamic(this, &APE_PlayerCharacter::OnTeamTurnStarted);
		}
	}
}

void APE_PlayerCharacter::OnTeamTurnStarted(int32 InTeamID)
{
	// 턴 매니저가 플레이어 턴의 시작을 알리면 즉시 AP를 최대로 리셋!
	if (InTeamID == GetTeamID())
	{
		if (StatComponent)
		{
			StatComponent->ResetAP();
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerCharacter] 내 턴 시작! AP가 %d로 모두 회복되었습니다."), StatComponent->GetCurrentAP());
		}
	}
}

void APE_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APE_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}