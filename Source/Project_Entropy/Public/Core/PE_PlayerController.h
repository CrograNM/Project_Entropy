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

	// ----- [Test Functions] -----
	UFUNCTION(BlueprintCallable, Category = "Test")
	void OnTestDrawCard(int32 Count); // [Test] 카드 드로우

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchInputMode(EPEGameState NewState); // [Test] 인풋 모드 전환 (Base/Battle) 

	// ----- [Multiplayer Network Functions] -----
	UFUNCTION(Client, Reliable)
	void Client_SetupInputMode(EPEGameState NewState); // 클라이언트 접속 시 인풋 모드를 강제 세팅하기 위한 RPC

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestGridMove(AACTile* TargetTile); // 클라이언트에서 타일 클릭 시 서버로 이동 요청 (충돌 처리 포함)

	// 스킬 시전 서버 요청
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSkillCast(class UPE_SkillData* SkillData, class AACTile* TargetTile, class APE_CharacterBase* TargetCharacter);

	// 서버 측에서 강제로 클라이언트의 이동/캐스팅 액션을 취소 (턴 종료 시 사용)
	UFUNCTION(Client, Reliable)
	void Client_CancelCurrentAction();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// --- [안전한 멀티플레이어 참조 헬퍼 함수] ---
	UFUNCTION(BlueprintCallable, Category = "References")
	APE_PlayerCharacter* GetCachedPlayerCharacter();

	UFUNCTION(BlueprintCallable, Category = "References")
	UPE_TurnManagerComponent* GetCachedTurnManager();

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

	// ----- [Test] -----
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TArray<TObjectPtr<UPE_CardData>> TestStartingDeck;

private:
	// ----- [Direct Move] -----
	void OnDirectMove(const FInputActionValue& Value);

	// ----- [Select Action] -----
	void OnSelect(const FInputActionValue& Value);
	void OnCardSelect(const FInputActionValue& Value);
	void OnCardRelease(const FInputActionValue& Value);

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
	bool bIsGridMoveActivated = false;	// 이동 모드 활성화 여부

	// ----- [Temporary Variables] -----
	float StoredMouseX = 0.f;
	float StoredMouseY = 0.f;
};
