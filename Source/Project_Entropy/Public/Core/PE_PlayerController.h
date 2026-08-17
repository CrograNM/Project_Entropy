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

	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void SendSkillCastRequest(class UPE_SkillData* SkillData, class AACTile* TargetTile, class APE_CharacterBase* TargetCharacter, class APE_CardActor* SourceCard, bool bIsFreeCast = false);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSkillCast(class UPE_SkillData* SkillData, class AACTile* TargetTile, class APE_CharacterBase* TargetCharacter, int32 ClientRequestID, bool bIsFreeCast);
	
	// 서버가 액션 큐 차례가 되어 클라이언트 측 애니메이션 재생을 명령
	UFUNCTION(Client, Reliable)
	void Client_PlaySkillAnim(int32 ClientRequestID);

	// 애니메이션 재생 후 C++ 이벤트를 거쳐 클라이언트가 서버에 완료 보고
	UFUNCTION(Server, Reliable)
	void Server_NotifySkillAnimFinished(int32 ClientRequestID);

	// Blueprint 타임라인이 끝나고 CardActor에서 PC를 역참조할 때 사용되는 브릿지
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void NotifyDiscardAnimFinishedForCard(class APE_CardActor* Card);

	// 서버 측에서 최종적으로 발사체 스폰 후 클라이언트에 논리적인 버리기를 통보
	UFUNCTION(Client, Reliable)
	void Client_ConfirmSkillExecution(int32 ClientRequestID);

	UFUNCTION(Client, Reliable)
	void Client_CancelSkillExecution(int32 ClientRequestID);

	UFUNCTION(Client, Reliable)
	void Client_CancelCurrentAction();

	// --- [Turn End System] ---
	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void ToggleTurnReadyState();

	UFUNCTION(Server, Reliable)
	void Server_SetTurnReadyState(bool bReady);

	UFUNCTION(Client, Reliable)
	void Client_ResetReadyState(); 

	UFUNCTION(Client, Reliable)
	void Client_TriggerTurnEndCards();

	UFUNCTION(Server, Reliable)
	void Server_TurnEndCardsFinished();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn System")
	bool bIsReadyForTurnEnd = false;

	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void TryExecuteCardDrop(class APE_CardActor* DroppedCard);

	UFUNCTION(BlueprintCallable, Category = "Battle Input|Trigger")
	void ForceTriggerCardLocally(class APE_CardActor* TriggeredCard);

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void NotifyTurnEndCardReadyAnimFinished(class APE_CardActor* Card);

	// --- [안전한 멀티플레이어 참조 헬퍼 함수] ---
	UFUNCTION(BlueprintCallable, Category = "References")
	APE_PlayerCharacter* GetCachedPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "References")
	UPE_TurnManagerComponent* GetCachedTurnManager();

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

	bool GetRandomValidTargetForSkill(class UPE_SkillData* SkillData, class AACTile*& OutTile, class APE_CharacterBase*& OutChar);

	// 카드 정보 전송용 클라이언트 로컬 매핑 데이터 (시전 요청 ID -> 카드 액터)
	int32 CurrentSkillRequestID = 0;
	UPROPERTY()
	TMap<int32, class APE_CardActor*> PendingSkillRequests;

	TMap<int32, bool> PendingSkillAutoCastFlags;

	// 턴 종료 카드 순차 발동 지원을 위한 임시 캐싱
	UPROPERTY()
	class APE_CardActor* PendingTurnEndCard = nullptr;

	UPROPERTY()
	class APE_CardActor* FailedTurnEndCard = nullptr;

	// ----- [Temporary Variables] -----
	float StoredMouseX = 0.f;
	float StoredMouseY = 0.f;
};
