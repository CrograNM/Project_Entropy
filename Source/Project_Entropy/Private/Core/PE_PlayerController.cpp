// Copyright CrograNM

#include "Core/PE_PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACCameraControlComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACTargetingVisualizerComponent.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_BattleGameMode.h"
#include "Core/PE_GameState.h"
#include "Core/PE_CheatManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ACDeckManagerComponent.h"
#include "Components/ACCardInteractionComponent.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_DataTypes.h"
#include "Characters/PE_EnemyBase.h"
#include "Components/ACSkillComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"

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

	// 멀티플레이어 환경에서는 GameMode가 서버에만 존재하므로 HasAuthority 체크를 먼저 수행
	if (HasAuthority())
	{
		APE_GameMode* GameMode = Cast<APE_GameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode)
		{
			SwitchInputMode(GameMode->GetCurrentState());
			// 서버인 경우 클라이언트의 인풋도 맞춰주기 위해 RPC 호출
			Client_SetupInputMode(GameMode->GetCurrentState());
		}
	}
	// 클라이언트/서버 공통 참조 초기화 (GameMode 의존성 제거)
	if (AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass()))
	{
		GridSystem = Cast<AACGridSystem>(FoundGridActor);
	}

	if (DeckManagerComp && TestStartingDeck.Num() > 0)
	{
		DeckManagerComp->InitializeDeck(TestStartingDeck);
	}
}
void APE_PlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// 폰이 배정되었으므로 캐싱을 갱신하고 카메라 모드를 적용합니다.
	PlayerCharacter = Cast<APE_PlayerCharacter>(InPawn);
	ApplyCameraMode();
}
void APE_PlayerController::ApplyCameraMode()
{
	if (APE_PlayerCharacter* PC = GetCachedPlayerCharacter())
	{
		if (UACCameraControlComponent* CameraComp = PC->GetCameraControlComponent())
		{
			bool bIsFreeMode = (CurrentInputMode == EPEGameState::Battle);
			CameraComp->SetCameraFreeMode(bIsFreeMode);
		}
	}
}

// 지연 초기화 헬퍼 함수 구현 (접근 시 비어있다면 동적으로 채워넣음)
APE_PlayerCharacter* APE_PlayerController::GetCachedPlayerCharacter()
{
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<APE_PlayerCharacter>(GetPawn());
	}
	return PlayerCharacter;
}
UPE_TurnManagerComponent* APE_PlayerController::GetCachedTurnManager()
{
	if (!TurnManager)
	{
		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
			TurnManager = GS->GetTurnManager();
		}
	}
	return TurnManager;
}

void APE_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input Component로 캐스팅하여 액션 바인딩
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// ----- [Base Mode Movement] -----
		if (IA_DirectMove)
		{
			EnhancedInputComponent->BindAction(IA_DirectMove, ETriggerEvent::Triggered, this, &APE_PlayerController::OnDirectMove);
		}

		// ----- [Select Action] -----
		if (IA_Select)
		{
			EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Started, this, &APE_PlayerController::OnSelect);

			// 클릭 시작 (Pressed) -> 카드 잡기 시도
			EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Started, this, &APE_PlayerController::OnCardSelect);

			// 클릭 종료 (Released) -> 카드 놓기 시도
			EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Completed, this, &APE_PlayerController::OnCardRelease);
		}
		
		// ----- [Universal Cancel Action] -----
		if (IA_Cancel)
		{
			EnhancedInputComponent->BindAction(IA_Cancel, ETriggerEvent::Triggered, this, &APE_PlayerController::OnCancelAction);
		}

		// ----- [Camera Control] -----
		if (IA_CameraControlTrigger)
		{
			EnhancedInputComponent->BindAction(IA_CameraControlTrigger, ETriggerEvent::Started, this, &APE_PlayerController::OnCameraControlStarted);
			EnhancedInputComponent->BindAction(IA_CameraControlTrigger, ETriggerEvent::Completed, this, &APE_PlayerController::OnCameraControlCompleted);
			EnhancedInputComponent->BindAction(IA_CameraControlTrigger, ETriggerEvent::Canceled, this, &APE_PlayerController::OnCameraControlCompleted);
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
	}
}
void APE_PlayerController::Client_SetupInputMode_Implementation(EPEGameState NewState)
{
	SwitchInputMode(NewState);
}

// ----- [Update] -----
void APE_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateGridHovering();
}
void APE_PlayerController::UpdateGridHovering() 
{
	if (!bShowMouseCursor || !GridSystem) return;

	FHitResult HitResult;
	bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	APE_PlayerCharacter* PC = GetCachedPlayerCharacter();
	if (!PC || !PC->GetTargetingVisualizer()) return;

	// 1. [이동 모드]
	if (bIsGridMoveActivated)
	{
		AACTile* HoveredTile = Cast<AACTile>(HitResult.GetActor());
		if (HoveredTile)
		{
			// [수정됨] 매 프레임 그리지 않고, 마우스가 위치한 타일 좌표만 상태로 넘깁니다.
			PC->GetTargetingVisualizer()->UpdateHoveredTile(HoveredTile->GetGridPosition());
		}
	}

	// 2. [캐스팅 모드]
	else if (CardInteractionComp && CardInteractionComp->GetCurrentState() == EPEInteractionState::Casting)
	{
		APE_CardActor* CastingCard = CardInteractionComp->GetCastingCard();
		if (!CastingCard || !CastingCard->GetSkillData()) return;

		if (bHit)
		{
			AACTile* HoveredTile = Cast<AACTile>(HitResult.GetActor());
			if (!HoveredTile)
			{
				APE_CharacterBase* HitChar = Cast<APE_CharacterBase>(HitResult.GetActor());
				if (HitChar && HitChar->GetGridMovementComponent())
				{
					HoveredTile = GridSystem->GetTileAtPosition(HitChar->GetGridMovementComponent()->GetGridPosition());
				}
			}

			if (HoveredTile)
			{
				// 유효성 검사 후 Visualizer에게 마우스 위치 전달
				bool bIsValidTarget = false;
				if (PC->GetTargetingVisualizer()->IsTileInRange(HoveredTile))
				{
					if (CastingCard->GetSkillData()->TargetType == EPESkillTargetType::Tile)
					{
						bIsValidTarget = true;
					}
					else if (CastingCard->GetSkillData()->TargetType == EPESkillTargetType::Snap_Enemy)
					{
						TArray<AActor*> Enemies;
						UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_EnemyBase::StaticClass(), Enemies);
						for (AActor* EnemyActor : Enemies)
						{
							APE_EnemyBase* Enemy = Cast<APE_EnemyBase>(EnemyActor);
							if (Enemy && Enemy->GetGridMovementComponent()->GetGridPosition() == HoveredTile->GetGridPosition())
							{
								bIsValidTarget = true;
								break;
							}
						}
					}
				}

				if (bIsValidTarget)
				{
					PC->GetTargetingVisualizer()->UpdateHoveredTile(HoveredTile->GetGridPosition());
				}
				else
				{
					PC->GetTargetingVisualizer()->UpdateHoveredTile(FIntPoint(-999, -999));
				}
			}
		}
	}
}

// ----- [Public Functions] -----
void APE_PlayerController::ToggleGridMovementActivation()
{
	if (bIsReadyForTurnEnd) return; // 레디 상태면 이동 불가

	// 맵 툴이 켜져 있다면 이동 모드 진입을 차단
	if (UPE_CheatManager* CM = Cast<UPE_CheatManager>(CheatManager))
	{
		if (CM->IsMapToolActive()) return; 
	}
	
	// 필터링: 배틀모드, 플레이어 턴, GridSystem 존재 여부
	if (!GridSystem) return;
	if (CurrentInputMode != EPEGameState::Battle) return;

	// 동적 턴 매니저 호출 및 Null 예외 처리
	UPE_TurnManagerComponent* TM = GetCachedTurnManager();
	if (!TM || TM->GetCurrentPhase() != EPEBattlePhase::PlayerTurn) return;

	// 이동 모드 토글
	if (!bIsGridMoveActivated)
	{
		// 이동 모드 활성화
		bIsGridMoveActivated = true;

		// 카드 상호작용 취소 및 비활성화
		if (CardInteractionComp)
		{
			CardInteractionComp->SetInteractionEnabled(false);
		}

		// 동적 캐릭터 호출
		APE_PlayerCharacter* PC = GetCachedPlayerCharacter();
		if (PC && PC->GetStatComponent() && PC->GetTargetingVisualizer())
		{
			// 이동 모드 진입: 시각화 컴포넌트에 모드 및 사거리 설정
			PC->GetTargetingVisualizer()->SetTargetingMode(ETargetingMode::Movement, PC->GetStatComponent()->GetMoveRange());
		}
	}
	else
	{
		// 이동 모드 비활성화, 조작 초기화
		CancelCurrentAction();
	}
}

// 클라이언트 강제 조작 취소 RPC 구현부 추가
void APE_PlayerController::Client_CancelCurrentAction_Implementation()
{
	CancelCurrentAction(); // 기존 로컬 함수 호출
}

void APE_PlayerController::CancelCurrentAction()
{
	APE_PlayerCharacter* PC = GetCachedPlayerCharacter();

	// [카메라 리셋]
	if (PC && PC->GetCameraControlComponent())
	{
		PC->GetCameraControlComponent()->ResetCameraPosition();
	}

	// [이동 모드 취소]
	if (bIsGridMoveActivated)
	{
		bIsGridMoveActivated = false;
		if (CardInteractionComp) CardInteractionComp->SetInteractionEnabled(true);

		if (PC && PC->GetTargetingVisualizer())
		{
			PC->GetTargetingVisualizer()->ClearTargeting();
		}
	}

	// [카드 상호작용 취소]
	if (CardInteractionComp && CardInteractionComp->GetCurrentState() == EPEInteractionState::Casting)
	{
		CardInteractionComp->CancelCasting();

		if (PC && PC->GetTargetingVisualizer())
		{
			PC->GetTargetingVisualizer()->ClearTargeting();
		}
	}
}

// ----- [Direct Move] -----
void APE_PlayerController::OnDirectMove(const FInputActionValue& Value)
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

// ----- [Select Action] -----
void APE_PlayerController::OnSelect(const FInputActionValue& Value)
{
	if (bIsReadyForTurnEnd) return; // 레디 상태면 클릭 불가
	if (!GridSystem) return;

	APE_PlayerCharacter* PC = GetCachedPlayerCharacter();
	if (!PC || !PC->GetTargetingVisualizer()) return;

	// [그리드 이동 모드] - 타일 선택
	if (bIsGridMoveActivated) 
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			AACTile* TargetTile = Cast<AACTile>(HitResult.GetActor());

			// 컨트롤러의 사거리 검사 -> TargetingVisualizer에게 위임
			if (!TargetTile || !PC->GetTargetingVisualizer()->IsTileInRange(TargetTile))
			{
				CancelCurrentAction();
				return;
			}

			// 로컬에서 직접 움직이는 대신 서버로 이동 및 검증 요청
			Server_RequestGridMove(TargetTile);

			// 이동 요청 후 로컬 클라이언트의 UI 및 모드는 즉시 초기화, 추후 조작감 수정 작업 필요
			CancelCurrentAction();
			if (PC->GetCameraControlComponent())
			{
				PC->GetCameraControlComponent()->ResetCameraPosition();
			}
		}
	}

	// [스킬 시전 모드 클릭]
	else if (CardInteractionComp && CardInteractionComp->GetCurrentState() == EPEInteractionState::Casting)
	{
		FHitResult HitResult;
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

		// 캐스팅 중인 카드를 직접 클릭한 경우
		APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());
		if (HitCard && HitCard == CardInteractionComp->GetCastingCard())
		{
			CancelCurrentAction();
			return;
		}

		AACTile* TargetTile = Cast<AACTile>(HitResult.GetActor());
		APE_CharacterBase* TargetCharacter = Cast<APE_CharacterBase>(HitResult.GetActor());
		APE_CardActor* CastingCard = CardInteractionComp->GetCastingCard();

		if (!TargetTile && TargetCharacter && TargetCharacter->GetGridMovementComponent())
		{
			TargetTile = GridSystem->GetTileAtPosition(TargetCharacter->GetGridMovementComponent()->GetGridPosition());
		}
		else if (!TargetTile && !TargetCharacter)
		{
			// 타일도 캐릭터도 아닌 경우 (예: 배경 클릭) -> 시전 취소
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 스킬 시전 취소: 타일/캐릭터 선택 X"));
			CancelCurrentAction();
			return;
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 스킬 시도: 타일 %s, 캐릭터 %s, 스킬: %s"),
				TargetTile ? *TargetTile->GetName() : TEXT("null"),
				TargetCharacter ? *TargetCharacter->GetName() : TEXT("null"),
				CastingCard && CastingCard->GetSkillData() ? *CastingCard->GetSkillData()->GetName() : TEXT("null"));
		}

		if (CastingCard && CastingCard->GetSkillData() && TargetTile)
		{
			UPE_SkillData* SkillData = CastingCard->GetSkillData();

			// 유효성 검사 (타겟 스냅 룰 적용), 사거리 유효성 검사를 TargetingVisualizer로부터 인계받음
			bool bIsValidTarget = false;
			if (PC->GetTargetingVisualizer()->IsTileInRange(TargetTile)) 
			{
				if (SkillData->TargetType == EPESkillTargetType::Tile)
				{
					bIsValidTarget = true;
				}
				else if (SkillData->TargetType == EPESkillTargetType::Snap_Enemy)
				{
					TArray<AActor*> Enemies;
					UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_EnemyBase::StaticClass(), Enemies);
					for (AActor* EnemyActor : Enemies)
					{
						APE_EnemyBase* Enemy = Cast<APE_EnemyBase>(EnemyActor);
						if (Enemy && Enemy->GetGridMovementComponent()->GetGridPosition() == TargetTile->GetGridPosition())
						{
							TargetCharacter = Enemy; // 스냅된 적 캐릭터 캐싱
							bIsValidTarget = true;
							break;
						}
					}
				}
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 스킬 시전 취소: 타겟 타일이 유효 범위 밖"));
				CancelCurrentAction();
			}
			if (bIsValidTarget)
			{
				// 로컬에서 직접 쏘는 대신, 서버로 RPC를 보내 안전하게 결제/시전
				Server_RequestSkillCast(SkillData, TargetTile, TargetCharacter);

				CardInteractionComp->OnInstantCastFinished();
				if (PC->GetTargetingVisualizer())
				{
					PC->GetTargetingVisualizer()->ClearTargeting();
				}
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 스킬 시도 실패: 유효하지 않은 타겟"));
				CancelCurrentAction();
			}
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 스킬 시도 실패: CastingCard 또는 SkillData 또는 TargetTile이 null"));
			CancelCurrentAction();
		}
	}
}
void APE_PlayerController::OnCardSelect(const FInputActionValue& Value)
{
	if (bIsReadyForTurnEnd) return; // 레디 상태면 클릭 불가
	if (bIsGridMoveActivated) return;

	if (CardInteractionComp)
	{
		CardInteractionComp->GrabCard();
	}
}
void APE_PlayerController::OnCardRelease(const FInputActionValue& Value)
{
	if (bIsReadyForTurnEnd) return; // 레디 상태면 클릭 불가

	if (CardInteractionComp)
	{
		CardInteractionComp->ReleaseCard();

		// 카드를 놓아 캐스팅 모드로 진입했을 때 사거리 표시 확정
		APE_PlayerCharacter* PC = GetCachedPlayerCharacter();
		APE_CardActor* CastingCard = CardInteractionComp->GetCastingCard();
		if (PC && PC->GetTargetingVisualizer() && CastingCard && CastingCard->GetSkillData())
		{
			PC->GetTargetingVisualizer()->SetTargetingMode(ETargetingMode::Skill, CastingCard->GetSkillData()->BaseRange);
		}
	}
}

// ----- [Universal Cancel Action] -----
void APE_PlayerController::OnCancelAction(const FInputActionValue& Value)
{
	CancelCurrentAction();
}

// ----- [Camera Control] -----
void APE_PlayerController::OnCameraControlStarted(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	GetMousePosition(StoredMouseX, StoredMouseY);

	bShowMouseCursor = false;

	// 카메라 조작 중에는 카드 상호작용 일시중지 (드래그/캐스팅 방지)
	if (CardInteractionComp)
	{
		CardInteractionComp->SetInteractionSuspended(true);
	}
}
void APE_PlayerController::OnCameraControlCompleted(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	SetMouseLocation(FMath::RoundToInt(StoredMouseX), FMath::RoundToInt(StoredMouseY));

	bShowMouseCursor = true;

	// 그리드 이동 모드가 아닐 때만 카드 상호작용을 다시 활성화
	if (CardInteractionComp && !bIsGridMoveActivated)
	{
		CardInteractionComp->SetInteractionSuspended(false);
	}
}
void APE_PlayerController::OnCameraMove(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	bShowMouseCursor = false;

	FVector2D MoveVector = Value.Get<FVector2D>();
	// 동적 캐릭터 호출을 통한 컴포넌트 접근
	if (APE_PlayerCharacter* PC = GetCachedPlayerCharacter())
	{
		PC->GetCameraControlComponent()->PanCamera(MoveVector);
	}
}
void APE_PlayerController::OnCameraRotate(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	bShowMouseCursor = false;

	FVector2D RotateVector = Value.Get<FVector2D>();
	if (APE_PlayerCharacter* PC = GetCachedPlayerCharacter())
	{
		PC->GetCameraControlComponent()->RotateCamera(RotateVector);
	}
}
void APE_PlayerController::OnCameraReset(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	if (APE_PlayerCharacter* PC = GetCachedPlayerCharacter())
	{
		PC->GetCameraControlComponent()->ResetCameraPosition();
	}
}
void APE_PlayerController::OnCameraHeight(const FInputActionValue& Value)
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	// Axis1D 타입이므로 float 값으로 받아옴
	float HeightInput = Value.Get<float>();
	if (APE_PlayerCharacter* PC = GetCachedPlayerCharacter())
	{
		PC->GetCameraControlComponent()->AdjustCameraHeight(HeightInput);
	}
}

// ----- [Test Functions] -----
void APE_PlayerController::OnTestDrawCard(int32 Count)
{
	if (DeckManagerComp)
	{
		// D 키를 누를 때마다 드로우 테스트
		DeckManagerComp->DrawCards(Count);
	}
}
void APE_PlayerController::SwitchInputMode(EPEGameState NewState)
{
	CurrentInputMode = NewState;

	ApplyCameraMode();

	CancelCurrentAction(); // 모드 전환 시 모든 액션 초기화

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

		SetBattleUIVisibility(false);

		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::SwitchInputMode] 기지 모드로 전환"));
		break;
	}

	case EPEGameState::Battle:
	{
		if (IMC_Battle)
		{
			Subsystem->AddMappingContext(IMC_Battle, 0);
		}
		// 배틀 모드: GameAndUI, 마우스 커서가 기본적으로 보이도록 설정
		FInputModeGameAndUI InputModeDataUI;
		InputModeDataUI.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputModeDataUI.SetHideCursorDuringCapture(false);
		SetInputMode(InputModeDataUI);

		SetBattleUIVisibility(true);

		UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController::SwitchInputMode] 배틀 모드로 전환"));
		break;
	}
	}
}

// 스킬 시전 서버 검증 및 실행 RPC
bool APE_PlayerController::Server_RequestSkillCast_Validate(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	return SkillData != nullptr;
}
void APE_PlayerController::Server_RequestSkillCast_Implementation(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	APE_PlayerCharacter* PC = GetCachedPlayerCharacter();
	if (!PC) return;

	UACSkillComponent* SkillComp = PC->FindComponentByClass<UACSkillComponent>();
	if (SkillComp)
	{
		float FinalDamage = SkillData->BaseDamage;
		// 서버에서 AP 차감 및 로직 실행
		SkillComp->TryExecuteSkillByData(SkillData, TargetTile, TargetCharacter, FinalDamage);
	}
}

bool APE_PlayerController::Server_RequestGridMove_Validate(AACTile* TargetTile)
{
	return TargetTile != nullptr;
}
void APE_PlayerController::Server_RequestGridMove_Implementation(AACTile* TargetTile)
{
	APE_PlayerCharacter* PC = GetCachedPlayerCharacter();

	if (!GridSystem || !PC || !PC->GetGridMovementComponent() || !PC->GetStatComponent()) return;

	FIntPoint StartPos = PC->GetGridMovementComponent()->GetGridPosition();
	TArray<AACTile*> Path = GridSystem->CalculatePath(StartPos, TargetTile->GetGridPosition());

	if (Path.IsEmpty()) return;

	AACTile* FinalDestination = Path.Last();

	// [도착지 충돌 보정 로직]: 모든 컨트롤러를 순회하며 겹치는 목적지가 있는지 확인
	bool bDestinationOccupied = false;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APE_PlayerController* OtherPC = Cast<APE_PlayerController>(Iterator->Get());
		if (OtherPC && OtherPC != this && OtherPC->GetCachedPlayerCharacter())
		{
			if (UACGridMovementComponent* OtherMoveComp = OtherPC->GetCachedPlayerCharacter()->GetGridMovementComponent())
			{
				FIntPoint OtherPos = OtherMoveComp->GetGridPosition();
				FIntPoint OtherTarget = OtherMoveComp->GetTargetGridPosition();
				FIntPoint MyTarget = FinalDestination->GetGridPosition();

				// 이미 누군가 위치해 있거나 해당 지점을 향해 이동(예약) 중이라면 충돌
				if (MyTarget == OtherPos || MyTarget == OtherTarget)
				{
					bDestinationOccupied = true;
					break;
				}
			}
		}
	}

	// 겹친다면 이전 타일로 보정
	if (bDestinationOccupied)
	{
		if (Path.Num() > 1)
		{
			Path.Pop();
			FinalDestination = Path.Last();
		}
		else
		{
			// 제자리에서 막힌 경우 이동 취소
			UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerController] 도착지가 모두 막혀 이동을 취소합니다."));
			return;
		}
	}

	// 서버 권한으로 AP 소모 후 멀티캐스트 전송
	if (PC->GetStatComponent()->ConsumeAP(1))
	{
		// 이 순간부터 해당 PC의 TargetGridPosition이 갱신됨
		PC->GetGridMovementComponent()->NetMulticast_MoveAlongPath(Path);
	}
}

void APE_PlayerController::ToggleTurnReadyState()
{
	if (CurrentInputMode != EPEGameState::Battle) return;

	UPE_TurnManagerComponent* TM = GetCachedTurnManager();
	if (!TM || TM->GetCurrentPhase() != EPEBattlePhase::PlayerTurn) return;

	bIsReadyForTurnEnd = !bIsReadyForTurnEnd;
	Server_SetTurnReadyState(bIsReadyForTurnEnd);

	// 레디를 눌렀다면 쥐고 있던 카드나 조준, 이동 모드를 모두 강제로 취소하여 손을 비웁니다.
	if (bIsReadyForTurnEnd)
	{
		CancelCurrentAction();
	}
}

void APE_PlayerController::Server_SetTurnReadyState_Implementation(bool bReady)
{
	bIsReadyForTurnEnd = bReady;
	if (UPE_TurnManagerComponent* TM = GetCachedTurnManager())
	{
		TM->RequestTurnEnd(this, bReady);
	}
}

void APE_PlayerController::Client_ResetReadyState_Implementation()
{
	bIsReadyForTurnEnd = false;
}