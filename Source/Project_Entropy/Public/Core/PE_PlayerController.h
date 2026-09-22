// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/PE_GameMode.h" // EPEGameState 사용 위함
#include "Grid/ACGridSystem.h"
#include "PE_PlayerController.generated.h"

class APE_PlayerCharacter;
class UPE_TurnManagerComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UACCardInteractionComponent;
class UACDeckManagerComponent;
class UPE_CardData;

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	APE_PlayerController();

	// ----- [Update] -----
	virtual void PlayerTick(float DeltaTime) override;
	void UpdateGridHovering(); 

	/** 커서 아래의 타일. 캐릭터를 맞았다면 그 캐릭터가 서 있는 칸으로 환산합니다. */
	class AACTile* GetTileUnderCursor();

	// ----- [Public Functions] -----
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void ToggleGridMovementActivation(); // 이동 모드 [On/Off]
	
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void CancelCurrentAction(); // 범용 취소 함수

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI")
	void ShowToastMessage(const FText& Message); // [UI] 토스트 메시지 출력

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "UI")
	void SetBattleUIVisibility(bool bVisible); // [UI] 배틀 UI 표시/숨김

	// ----- [Test Functions] -----
	UFUNCTION(BlueprintCallable, Category = "Test")
	void OnTestDrawCard(int32 Count); // [Test] 카드 드로우

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchInputMode(EPEGameState NewState); // [Test] 인풋 모드 전환 (Base/Battle) 

	// ----- [Multiplayer Network Functions] -----

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestGridMove(AACTile* TargetTile); // 클라이언트에서 타일 클릭 시 서버로 이동 요청 (충돌 처리 포함)

	UFUNCTION(Client, Reliable)
	void Client_CancelCurrentAction();

	// --- [Turn End System] ---
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void ToggleTurnReadyState();

	UFUNCTION(Server, Reliable)
	void Server_SetTurnReadyState(bool bReady);

	UFUNCTION(Client, Reliable)
	void Client_ResetReadyState(); 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn System")
	bool bIsReadyForTurnEnd = false;

	// --- [Getter] ---
	UFUNCTION(BlueprintCallable, Category = "References")
	APE_PlayerCharacter* GetCachedPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "References")
	UPE_TurnManagerComponent* GetCachedTurnManager();

	// --- [Getter, Components] ---
	FORCEINLINE class UPE_CardCastComponent* GetCardCast() const { return CardCastComp; }
	FORCEINLINE UACCardInteractionComponent* GetCardInteraction() const { return CardInteractionComp; }
	FORCEINLINE UACDeckManagerComponent* GetDeckManager() const { return DeckManagerComp; }
	FORCEINLINE AACGridSystem* GetGridSystem() const { return GridSystem; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	virtual void SetPawn(APawn* InPawn) override;
	void ApplyCameraMode();

	// ----- [Input Mapping Contexts & Actions] -----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_DirectMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_Battle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_DirectMove; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Select; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraControlTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Cancel;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraRotate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraReset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraHeight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_SelectCardByIndex;
	
	// ----- [References] -----
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<AACGridSystem> GridSystem; // 그리드 시스템 참조
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager; // 턴 매니저 참조
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<APE_PlayerCharacter> PlayerCharacter = nullptr; // 플레이어 캐릭터 참조

	// ----- [Components] -----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACDeckManagerComponent> DeckManagerComp; // 덱 매니저 컴포넌트

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UACCardInteractionComponent> CardInteractionComp; // 카드 상호작용 컴포넌트

	// 카드 시전 요청 -> 서버 확정/취소 왕복을 전담합니다. 컨트롤러는 입력만 넘깁니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPE_CardCastComponent> CardCastComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPE_CheatComponent> CheatNetworkComp;

	// ----- [Test Deck] -----
	UPROPERTY(EditDefaultsOnly, Category = "Test Deck")
	TArray<TObjectPtr<UPE_CardData>> TestStartingDeck;

private:
	// ----- [Direct Move] -----
	void OnDirectMove(const FInputActionValue& Value);

	// ----- [Select Action] -----
	void OnSelect(const FInputActionValue& Value);
	void OnCardSelect(const FInputActionValue& Value);
	void OnCardRelease(const FInputActionValue& Value);
	void OnSelectCardByIndexStarted(const FInputActionValue& Value);
	void OnSelectCardByIndexCompleted(const FInputActionValue& Value);

	// ----- [Universal Cancel Action] -----
	void OnCancelAction(const FInputActionValue& Value);

	// ----- [Camera Control] -----
	void OnCameraControlStarted(const FInputActionValue& Value);
	void OnCameraControlCompleted(const FInputActionValue& Value);
	void OnCameraMove(const FInputActionValue& Value);
	void OnCameraRotate(const FInputActionValue& Value);
	void OnCameraReset(const FInputActionValue& Value);
	void OnCameraHeight(const FInputActionValue& Value);

private:
	// ----- [State Variables] -----
	EPEGameState CurrentInputMode = EPEGameState::Base; // 현재 입력 모드 (Base/Battle)
	bool bIsGridMoveActivated = false;

	bool IsMyTurn() const; 

	// ----- [Temporary Variables] -----
	float StoredMouseX = 0.f;
	float StoredMouseY = 0.f;
};
