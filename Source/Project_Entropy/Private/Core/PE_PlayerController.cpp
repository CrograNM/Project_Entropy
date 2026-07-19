// Copyright CrograNM

#include "Core/PE_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACCameraControlComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_BattleGameMode.h"
#include "Core/PE_CheatManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ACDeckManagerComponent.h"
#include "Components/ACCardInteractionComponent.h"
#include "Cards/PE_CardData.h"

APE_PlayerController::APE_PlayerController()
{
	// 배틀 단계에서는 마우스 커서가 보여야 하므로 기본 활성화 설정 가능 (상황에 따라 토글 가능)
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	
	CheatClass = UPE_CheatManager::StaticClass();

	// 상호작용 컴포넌트 부착
	CardInteractionComp = CreateDefaultSubobject<UACCardInteractionComponent>(TEXT("CardInteractionComp"));

	// 덱 매니저 컴포넌트 부착
	DeckManagerComp = CreateDefaultSubobject<UACDeckManagerComponent>(TEXT("DeckManagerComp"));
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
		
		// APE_PlayerCharacter
		if (APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(GetPawn()))
		{
			PlayerCharacter = PC;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[APE_PlayerController::BeginPlay] 플레이어 캐릭터가 존재하지 않음"));
		}
	}

	// 테스트용 덱 데이터가 설정되어 있다면 덱 매니저 초기화 진행
	if (DeckManagerComp && TestStartingDeck.Num() > 0)
	{
		DeckManagerComp->InitializeDeck(TestStartingDeck);
	}
}

void APE_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input Component로 캐스팅하여 액션 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 1. 기지 모드 입력 바인딩
		if (IA_Move)
		{
			EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APE_PlayerController::Move);
		}

		// 2. 전투 모드 입력 바인딩
		if (IA_MouseClick)
		{
			EnhancedInputComponent->BindAction(IA_MouseClick, ETriggerEvent::Started, this, &APE_PlayerController::OnMouseClick);
		}
		if (IA_CameraMove)
		{
			EnhancedInputComponent->BindAction(IA_CameraMove, ETriggerEvent::Triggered, this, &APE_PlayerController::OnCameraMove);
		}
		if (IA_CameraRotate)
		{
			EnhancedInputComponent->BindAction(IA_CameraRotate, ETriggerEvent::Triggered, this, &APE_PlayerController::OnCameraRotate);
		}
		if (IA_CameraReset)
		{
			EnhancedInputComponent->BindAction(IA_CameraReset, ETriggerEvent::Completed, this, &APE_PlayerController::OnCameraReset);
		}
		if (IA_CameraHeight)
		{
			EnhancedInputComponent->BindAction(IA_CameraHeight, ETriggerEvent::Triggered, this, &APE_PlayerController::OnCameraHeight);
		}

		// 카드 상호작용 컴포넌트 입력 바인딩
		if (IA_MouseClick)
		{
			// 클릭 시작 (Pressed) -> 카드 잡기 시도
			EnhancedInputComponent->BindAction(IA_MouseClick, ETriggerEvent::Started, this, &APE_PlayerController::OnMouseClickStarted);

			// 클릭 종료 (Released) -> 카드 놓기 시도
			EnhancedInputComponent->BindAction(IA_MouseClick, ETriggerEvent::Completed, this, &APE_PlayerController::OnMouseClickCompleted);
		}
	}
}

void APE_PlayerController::ToggleGridMovementActivation()
{
	// 맵 툴이 켜져 있다면 이동 모드 진입을 차단
	if (UPE_CheatManager* CM = Cast<UPE_CheatManager>(CheatManager))
	{
		if (CM->IsMapToolActive()) return; 
	}
	
	// 필터링: 배틀모드, 플레이어 턴, GridSystem 존재 여부
	if (!GridSystem) return;
	if (CurrentInputMode != EPEGameState::Battle) return;
	if (TurnManager->GetCurrentPhase() != EPEBattlePhase::PlayerTurn) return;
	
	// 이동 모드 토글
	if (!bIsGridMoveActivated)
	{
		// 이동 모드 활성화
		bIsGridMoveActivated = true;

		// 카드 상호작용 비활성화
		if (CardInteractionComp)
		{
			CardInteractionComp->SetInteractionEnabled(false);
		}

		// 이동 범위 표시
		if (PlayerCharacter && PlayerCharacter->GetGridMovementComponent() && PlayerCharacter->GetStatComponent())
		{
			const int32 Range = PlayerCharacter->GetStatComponent()->GetMoveRange();
			ValidRangeTiles = GridSystem->ShowMovementRange(PlayerCharacter->GetGridMovementComponent()->GetGridPosition(), Range);
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
	// 카메라 리셋
	if (PlayerCharacter && PlayerCharacter->GetCameraControlComponent())
	{
		PlayerCharacter->GetCameraControlComponent()->ResetCameraPosition();
	}
	
	// 이동 모드 취소
	if (bIsGridMoveActivated)
	{
		bIsGridMoveActivated = false;

		// 카드 상호작용 재활성화
		if (CardInteractionComp)
		{
			CardInteractionComp->SetInteractionEnabled(true);
		}

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

	// 이동 모드: 마우스 호버링 - 타일 감지 및 경로 하이라이트
	if (!bIsGridMoveActivated || !GridSystem) return;
	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		AACTile* HoveredTile = Cast<AACTile>(HitResult.GetActor());
		if (HoveredTile && PlayerCharacter)
		{
			FIntPoint CurrentTilePos = HoveredTile->GetGridPosition();
			
			if (CurrentTilePos != LastHoveredTilePos)
			{
				LastHoveredTilePos = CurrentTilePos;
				GridSystem->HighlightPath(PlayerCharacter->GetGridMovementComponent()->GetGridPosition(), CurrentTilePos, ValidRangeTiles);
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
		
		// 유효한 타일이 아니거나 사거리 밖이면 이동 취소
		if (!TargetTile || !ValidRangeTiles.Contains(TargetTile))
		{
			CancelCurrentAction();
			return; 
		}
		
		// 정상적인 사거리 내 타일을 클릭 시 이동 확정
		if (TargetTile && PlayerCharacter && PlayerCharacter->GetGridMovementComponent() && PlayerCharacter->GetStatComponent() && PlayerCharacter->GetCameraControlComponent() && ValidRangeTiles.Contains(TargetTile))
		{
			// 이동 경로 추출
			TArray<AACTile*> Path = GridSystem->CalculatePath(PlayerCharacter->GetGridMovementComponent()->GetGridPosition(), TargetTile->GetGridPosition());
			
			// 이동에 필요한 AP 소모 (1AP)
			if (PlayerCharacter->GetStatComponent()->ConsumeAP(1))
			{
				// 도착지(TargetTile)를 제외한 모든 사거리 타일 원상복구
				GridSystem->ClearAllHighlights();
				TargetTile->SetHighlightState(ETileHighlightType::Hovered);
			
				// 캐릭터에게 이동 명령 하달 및 모드 종료
				PlayerCharacter->GetGridMovementComponent()->MoveAlongPath(Path); 
			
				// 정상적으로 이동 '완료(초기화)' 처리
				bIsGridMoveActivated = false;
				ValidRangeTiles.Empty();
				LastHoveredTilePos = FIntPoint(-999, -999);
				
				// 카메라 리셋
				PlayerCharacter->GetCameraControlComponent()->ResetCameraPosition();
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

void APE_PlayerController::OnCameraMove(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	FVector2D MoveVector = Value.Get<FVector2D>();
	if (IsValid(PlayerCharacter) && PlayerCharacter->GetCameraControlComponent())
	{
		PlayerCharacter->GetCameraControlComponent()->PanCamera(MoveVector);
	}
}

void APE_PlayerController::OnCameraRotate(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	FVector2D RotateVector = Value.Get<FVector2D>();
	if (IsValid(PlayerCharacter) && PlayerCharacter->GetCameraControlComponent())
	{
		PlayerCharacter->GetCameraControlComponent()->RotateCamera(RotateVector);
	}
}

void APE_PlayerController::OnCameraReset(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	if (IsValid(PlayerCharacter) && PlayerCharacter->GetCameraControlComponent())
	{
		// 캐릭터 카메라 리셋 호출
		PlayerCharacter->GetCameraControlComponent()->ResetCameraPosition();
	}
}

void APE_PlayerController::OnCameraHeight(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	// Axis1D 타입이므로 float 값으로 받아옴
	float HeightInput = Value.Get<float>();
	if (IsValid(PlayerCharacter) && PlayerCharacter->GetCameraControlComponent())
	{
		PlayerCharacter->GetCameraControlComponent()->AdjustCameraHeight(HeightInput);
	}
}

void APE_PlayerController::OnMouseClickStarted(const FInputActionValue& Value)
{
	if (bIsGridMoveActivated) return;

	// 기존 이동/타일 클릭 로직 전에, 카드를 잡을 수 있는지 상호작용 컴포넌트에 우선 권한을 넘김
	if (CardInteractionComp)
	{
		CardInteractionComp->GrabCard();
	}

	// TODO: 카드를 잡지 못했을 경우에만 기존 Grid 이동 로직(OnMouseClick) 실행하도록 분기 처리 필요
}

void APE_PlayerController::OnMouseClickCompleted(const FInputActionValue& Value)
{
	if (CardInteractionComp)
	{
		CardInteractionComp->ReleaseCard();
	}
}

void APE_PlayerController::OnTestDrawCard()
{
	if (DeckManagerComp)
	{
		// D 키를 누를 때마다 1장씩 드로우 테스트
		DeckManagerComp->DrawCards(1);
	}
}