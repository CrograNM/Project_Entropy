// Copyright CrograNM

#include "Core/PE_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

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
	
	// 현재 월드에 스폰되어 배치되어 있는 AACGridSystem 타입의 액터를 찾아 자동으로 연동
	AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass());
	if (FoundGridActor)
	{
		GridSystem = Cast<AACGridSystem>(FoundGridActor);
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::BeginPlay] 월드에서 GridSystem 연동 성공"));
		SwitchInputMode(EPEGameState::Battle);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::BeginPlay] 월드에 GridSystem 액터가 없음"));
		SwitchInputMode(EPEGameState::Base);
	}
	
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

void APE_PlayerController::ToggleMovementMode()
{
	bIsMovementMode = !bIsMovementMode;

	APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn());
	if (bIsMovementMode && PC && PC->GetGridMovementComponent() && PC->GetStatComponent() && GridSystem)
	{
		const int32 Range = PC->GetStatComponent()->GetMoveRange();

		ValidRangeTiles = GridSystem->ShowMovementRange(PC->GetGridMovementComponent()->GetGridPosition(), Range);
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::ToggleMovementMode] 이동 모드 활성화 - 사거리 표시 작동"));
	}
	else if (GridSystem)
	{
		GridSystem->ClearAllHighlights();
		ValidRangeTiles.Empty();
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::ToggleMovementMode] 이동 모드 비활성화 - 하이라이트 초기화"));
	}
}

void APE_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// --- 이동: 마우스 호버링 ---
	if (!bIsMovementMode || !GridSystem) return;

	// 마우스 밑의 타일 레이캐스팅 감지
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AACTile* HoveredTile = Cast<AACTile>(HitResult.GetActor());
		APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn());
		
		// UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::PlayerTick] 라인 트레이스 결과 %s"), *HitResult.GetActor()->GetName());
		// UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::PlayerTick] 타일: %s"), HoveredTile ? *HoveredTile->GetName() : TEXT("None"));
		if (HoveredTile && PC)
		{
			FIntPoint CurrentTilePos = HoveredTile->GetGridPosition();
			
			// 매 틱 연산 방지를 위해 마우스가 '새로운 타일'로 넘어갔을 때만 경로 재부각
			if (CurrentTilePos != LastHoveredTilePos)
			{
				LastHoveredTilePos = CurrentTilePos;
				GridSystem->HighlightPath(PC->GetGridMovementComponent()->GetGridPosition(), CurrentTilePos, ValidRangeTiles);
			}
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
			{
				if (IMC_DirectMove)
				{
					Subsystem->AddMappingContext(IMC_DirectMove, 0);
				}
				// 기지 모드: 마우스로 화면 회전을 하거나 조작해야 한다면 GameAndUI 모드로 설정
				FInputModeGameOnly InputModeDataGame;
				SetInputMode(InputModeDataGame);
				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::SwitchInputMode] 기지 모드로 전환"));
				break;
			}

		case EPEGameState::Battle:
			{
				if (IMC_Battle)
				{
					Subsystem->AddMappingContext(IMC_Battle, 0);
				}
				// 배틀 모드: 마우스가 UI(카드)와 전장을 자유롭게 넘나들어야 하므로 GameAndUI 모드로 설정
				FInputModeGameAndUI InputModeDataUI;
				InputModeDataUI.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				SetInputMode(InputModeDataUI);
				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::SwitchInputMode] 배틀 모드로 전환"));
				break;
			}
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
	if (!bIsMovementMode || !GridSystem) return;

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AACTile* TargetTile = Cast<AACTile>(HitResult.GetActor());
		APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn());
	
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::OnMouseClick] 타일: %s"), TargetTile ? *TargetTile->GetName() : TEXT("None"));
		
		if (TargetTile && PC && PC->GetGridMovementComponent() && PC->GetStatComponent() && ValidRangeTiles.Contains(TargetTile))
		{
			// 이동 경로 추출
			TArray<AACTile*> Path = GridSystem->CalculatePath(PC->GetGridMovementComponent()->GetGridPosition(), TargetTile->GetGridPosition());
			
			if (PC->GetStatComponent()->ConsumeAP(1)) // 이동에 필요한 AP 소모 (1AP)
			{
				// 도착지(TargetTile)를 제외한 모든 사거리 타일 원상복구
				for (AACTile* Tile : ValidRangeTiles)
				{
					if (Tile && Tile != TargetTile)
					{
						Tile->SetHighlightState(ETileHighlightType::None);
					}
				}
			
				// 캐릭터에게 이동 명령 하달 및 모드 종료
				PC->GetGridMovementComponent()->MoveAlongPath(Path); 
			
				bIsMovementMode = false;
				ValidRangeTiles.Empty();
				LastHoveredTilePos = FIntPoint(-999, -999);
			}
		}
	}
}