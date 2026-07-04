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

UCLASS()
class PROJECT_ENTROPY_API APE_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	APE_PlayerController();
	virtual void PlayerTick(float DeltaTime) override;

	// 이동 모드 [On/Off], UI 버튼이나 단축키를 눌러 호출
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void ToggleGridMovementActivation();
	
	// 현재 진행 중인 조작(이동, 카드 타겟팅 등)을 일괄 취소/초기화
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void CancelCurrentAction();
	
	// 외부에 의해 게임 모드가 바뀔 때 호출될 함수, **이후 패키징 단계에서 단축키에서 제거해야함**
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchInputMode(EPEGameState NewState);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	// --- 1. 에디터에서 할당할 인풋 에셋들 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_DirectMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Context", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> IMC_Battle;

	// --- 2. 인풋 액션들 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_Move; // 기지용 이동 액션

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_MouseClick; // 전투용 마우스 클릭 액션
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraRotate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraReset;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> IA_CameraHeight;
	
	// 월드의 GridSystem 참조
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<AACGridSystem> GridSystem;
	
	// 게임 모드의 TurnManager 참조
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<UPE_TurnManagerComponent> TurnManager;
	
	// 현재 플레이어 참조
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<APE_PlayerCharacter> PlayerCharacter = nullptr;

private:
	// 기지 모드에서 키보드 이동 처리
	void Move(const FInputActionValue& Value);

	// 전투 모드 입력 처리
	void OnMouseClick(const FInputActionValue& Value);
	void OnCameraMove(const FInputActionValue& Value);
	void OnCameraRotate(const FInputActionValue& Value);
	void OnCameraReset(const FInputActionValue& Value);
	void OnCameraHeight(const FInputActionValue& Value);
	
private:
	// Input Mode (Base/Battle)
	EPEGameState CurrentInputMode = EPEGameState::Base; 
	
	bool bIsGridMoveActivated = false;

	UPROPERTY()
	TArray<AACTile*> ValidRangeTiles;

	FIntPoint LastHoveredTilePos = FIntPoint(-999, -999);
};
