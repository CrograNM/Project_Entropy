// Copyright CrograNM

#include "Core/PE_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_BattleGameMode.h"
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

	// 게임 모드에서 인풋 모드를 변경 해주지만 한번 더 호출
	APE_GameMode* GameMode = Cast<APE_GameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		SwitchInputMode(GameMode->GetCurrentState());
	}
	
	// ----- 참조 초기화 -----
	APE_BattleGameMode* BattleGameMode = Cast<APE_BattleGameMode>(UGameplayStatics::GetGameMode(this));
	if (BattleGameMode)
	{
		// AACGridSystem
		if (AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass()))
		{
			GridSystem = Cast<AACGridSystem>(FoundGridActor);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[APE_PlayerController::BeginPlay] 월드에 GridSystem 액터가 없음"));
		}
		
		// UPE_TurnManagerComponent
		if (BattleGameMode->GetTurnManager())
		{
			TurnManager = BattleGameMode->GetTurnManager();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[APE_PlayerController::BeginPlay] 턴 매니저가 존재하지 않음"));
		}
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

void APE_PlayerController::ToggleGridMovementActivation()
{
	// 필터링: 배틀모드, 플레이어 턴, GridSystem 존재 여부
	if (!GridSystem) return;
	if (CurrentInputMode != EPEGameState::Battle) return;
	if (TurnManager->GetCurrentPhase() != EPEBattlePhase::PlayerTurn) return;
	
	// 이동 모드 토글
	if (!bIsGridMoveActivated)
	{
		// 이동 모드 활성화, 사거리 표시
		bIsGridMoveActivated = true;
		APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn());
		if (PC && PC->GetGridMovementComponent() && PC->GetStatComponent())
		{
			const int32 Range = PC->GetStatComponent()->GetMoveRange();
			ValidRangeTiles = GridSystem->ShowMovementRange(PC->GetGridMovementComponent()->GetGridPosition(), Range);
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::ToggleMovementMode] 이동 모드 활성화 - 사거리 표시 작동"));
		}
	}
	else
	{
		// 이동 모드 비활성화, 조작 초기화
		CancelCurrentAction();
	}
}

void APE_PlayerController::CancelCurrentAction()
{
	// 1. 이동 모드 활성화 상태라면 끄고 불빛을 초기화합니다.
	if (bIsGridMoveActivated)
	{
		bIsGridMoveActivated = false;
		if (GridSystem)
		{
			GridSystem->ClearAllHighlights();
		}
		ValidRangeTiles.Empty();
		LastHoveredTilePos = FIntPoint(-999, -999);
		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::CancelCurrentAction] 이동 조작이 취소되었습니다."));
	}

	// 2. [추후 추가될 기능]: 카드 타겟팅 취소 등
	/*
	if (bIsCardTargetingMode)
	{
		bIsCardTargetingMode = false;
		// 카드 UI 원래 위치로 되돌리기 등...
	}
	*/
}

void APE_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// --- 이동: 마우스 호버링 ---
	if (!bIsGridMoveActivated || !GridSystem) return;

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
	CurrentInputMode = NewState;
	
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
	if (!bIsGridMoveActivated || !GridSystem) return;

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AACTile* TargetTile = Cast<AACTile>(HitResult.GetActor());
		APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn());
		
		// 유효한 타일이 아니거나 사거리 밖이면 이동 취소
		if (!TargetTile || !ValidRangeTiles.Contains(TargetTile))
		{
			CancelCurrentAction();
			return; 
		}
		
		// 정상적인 사거리 내 타일을 클릭 시 이동 확정
		if (TargetTile && PC && PC->GetGridMovementComponent() && PC->GetStatComponent() && ValidRangeTiles.Contains(TargetTile))
		{
			// 이동 경로 추출
			TArray<AACTile*> Path = GridSystem->CalculatePath(PC->GetGridMovementComponent()->GetGridPosition(), TargetTile->GetGridPosition());
			
			// 이동에 필요한 AP 소모 (1AP)
			if (PC->GetStatComponent()->ConsumeAP(1))
			{
				// 도착지(TargetTile)를 제외한 모든 사거리 타일 원상복구
				GridSystem->ClearAllHighlights();
				TargetTile->SetHighlightState(ETileHighlightType::Hovered);
			
				// 캐릭터에게 이동 명령 하달 및 모드 종료
				PC->GetGridMovementComponent()->MoveAlongPath(Path); 
			
				// 정상적으로 이동 '완료(초기화)' 처리
				bIsGridMoveActivated = false;
				ValidRangeTiles.Empty();
				LastHoveredTilePos = FIntPoint(-999, -999);
			}
			else
			{	
				// AP 부족 시 이동 취소
				CancelCurrentAction();
				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::OnMouseClick] 이동 실패: AP 부족"));
			}
		}
	}
}