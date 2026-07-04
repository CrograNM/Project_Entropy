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

void APE_PlayerCharacter::PanCamera(FVector2D PanInput)
{
	if (PanInput.IsNearlyZero()) return;
	
	// 현재 카메라가 바라보는 방향을 기준으로 상하좌우 이동 벡터 계산 (Z축 무시)
	FVector Forward = TopDownCamera->GetForwardVector();
	Forward.Z = 0.f; 
	Forward.Normalize();

	FVector Right = TopDownCamera->GetRightVector();
	Right.Z = 0.f; 
	Right.Normalize();

	FVector MoveDelta = (Forward * PanInput.Y + Right * PanInput.X) * CameraPanSpeed;
	
	// CameraBase의 위치를 이동시키면 카메라 전체가 캐릭터에서 멀어지며 팬(Pan) 됩니다.
	CameraBase->AddWorldOffset(MoveDelta);
}

void APE_PlayerCharacter::RotateCamera(FVector2D RotateInput)
{
	if (RotateInput.IsNearlyZero()) return;

	// 마우스 좌우 이동(X) -> CameraBase의 좌우 회전 (Yaw)
	CameraBase->AddWorldRotation(FRotator(0.f, RotateInput.X * CameraRotationSpeed, 0.f));

	// 마우스 상하 이동(Y) -> SpringArm의 상하 줌/각도 회전 (Pitch, 제한 범위 설정)
	FRotator BoomRot = CameraBoom->GetRelativeRotation();
	BoomRot.Pitch = FMath::Clamp(BoomRot.Pitch + (RotateInput.Y * CameraRotationSpeed), -85.f, -20.f);
	CameraBoom->SetRelativeRotation(BoomRot);
}

void APE_PlayerCharacter::ResetCameraPosition()
{
	if (bIsCameraFree)
	{
		UE_LOG(LogTemp, Log, TEXT("[Camera] 고정 모드 상태이므로 카메라 초기화를 무시합니다."));
		return;
	}
	
	CameraBase->SetRelativeLocation(FVector::ZeroVector);
	CameraBase->SetRelativeRotation(FRotator::ZeroRotator);
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	UE_LOG(LogTemp, Warning, TEXT("[Camera] 카메라가 초기 상태로 리셋되었습니다."));
}

void APE_PlayerCharacter::ToggleCameraFreeMode()
{
	SetCameraFreeMode(!bIsCameraFree);
}

void APE_PlayerCharacter::SetCameraFreeMode(bool bEnable)
{
	if (bIsCameraFree == bEnable) return;
	
	bIsCameraFree = bEnable;

	if (bIsCameraFree)
	{
		CameraBase->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		
		UE_LOG(LogTemp, Warning, TEXT("[Camera] 카메라 모드: 자유"));
	}
	else
	{
		CameraBase->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		
		ResetCameraPosition();
		
		UE_LOG(LogTemp, Warning, TEXT("[Camera] 카메라 모드: 플레이어 추적"));
	}
}
