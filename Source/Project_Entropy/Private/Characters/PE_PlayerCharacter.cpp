// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_BattleGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

APE_PlayerCharacter::APE_PlayerCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	// 스프링암 컴포넌트 생성 및 설정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1000.f; 
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); 
	CameraBoom->bDoCollisionTest = false;			// 카메라가 장애물에 가려져도 당겨지지 않도록 설정 (전술 뷰 유지)
	CameraBoom->bUsePawnControlRotation = false;	// 컨트롤러 회전에 카메라가 돌아가지 않도록 고정

	// 카메라 컴포넌트 생성 및 설정
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
}

void APE_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APE_BattleGameMode* BattleGM = Cast<APE_BattleGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (UPE_TurnManagerComponent* TurnManager = BattleGM->GetTurnManager())
		{
			TurnManager->OnPhaseChanged.AddDynamic(this, &APE_PlayerCharacter::OnBattlePhaseChanged);
		}
	}
}

void APE_PlayerCharacter::OnBattlePhaseChanged(EPEBattlePhase NewPhase)
{
	// 턴 매니저가 플레이어 턴의 시작을 알리면 즉시 AP를 최대로 리셋!
	if (NewPhase == EPEBattlePhase::PlayerTurn)
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