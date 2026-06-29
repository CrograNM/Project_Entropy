// Copyright CrograNM

#include "Core/PE_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"

APE_PlayerController::APE_PlayerController()
{
	// 배틀 단계에서는 마우스 커서가 보여야 하므로 기본 활성화 설정 가능 (상황에 따라 토글 가능)
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void APE_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 최초 상태 설정 (여기서는 편의상 기지 모드로 시작한다고 가정)
	SwitchInputMode(EPEGameState::Base);
}

void APE_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input Component로 캐스팅하여 액션 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 1. 기지 이동 바인딩
		if (IA_Move)
		{
			EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APE_PlayerController::Move);
		}

		// 2. 전투 마우스 클릭 바인딩
		if (IA_MouseClick)
		{
			EnhancedInputComponent->BindAction(IA_MouseClick, ETriggerEvent::Started, this, &APE_PlayerController::OnMouseClick);
		}
	}
}

void APE_PlayerController::SwitchInputMode(EPEGameState NewState)
{
	// 향상된 입력 서브시스템 가져오기
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;

	// 안전하게 기존 컨텍스트를 모두 비우기
	Subsystem->ClearAllMappings();

	// 상태에 따라 필요한 IMC만 활성화하고, 마우스 입력 모드 제어
	switch (NewState)
	{
		case EPEGameState::Base:
			if (IMC_DirectMove)
			{
				Subsystem->AddMappingContext(IMC_DirectMove, 0);
			}
			// 기지 모드: 마우스로 화면 회전을 하거나 조작해야 한다면 GameAndUI 모드로 설정
			FInputModeGameOnly InputModeDataGame;
			SetInputMode(InputModeDataGame);
			break;

		case EPEGameState::Battle:
			if (IMC_Battle)
			{
				Subsystem->AddMappingContext(IMC_Battle, 0);
			}
			// 배틀 모드: 마우스가 UI(카드)와 전장을 자유롭게 넘나들어야 하므로 GameAndUI 모드로 설정
			FInputModeGameAndUI InputModeDataUI;
			InputModeDataUI.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			SetInputMode(InputModeDataUI);
			break;
	}
}

void APE_PlayerController::Move(const FInputActionValue& Value)
{
	// 기지 모드일 때만 IMC를 통해 이 함수가 호출됨
	FVector2D MovementVector = Value.Get<FVector2D>();

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		// 컨트롤러의 Yaw 회전값을 기준으로 전방/우측 방향 계산
		const FRotator Rotation = GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APE_PlayerController::OnMouseClick(const FInputActionValue& Value)
{
	// 배틀 모드일 때 마우스 클릭 시 호출됨
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		// 클릭한 타일이나 오브젝트 정보를 가져와 카드 타겟팅 로직 전개 가능
		AActor* HitActor = HitResult.GetActor();
		UE_LOG(LogTemp, Log, TEXT("배틀 클릭 타겟: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
	}
}