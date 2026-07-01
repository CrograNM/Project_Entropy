// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
}

void APE_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APE_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}