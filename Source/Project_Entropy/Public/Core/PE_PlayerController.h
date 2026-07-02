// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/PE_GameMode.h" // EPEGameState 사용 위함
#include "Grid/ACGridSystem.h"
#include "PE_PlayerController.generated.h"

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

	/** UI 버튼이나 단축키 'M'을 누르면 호출될 함수 */
	UFUNCTION(BlueprintCallable, Category = "Battle Input")
	void ToggleMovementMode();
	
	/** 외부에 의해 게임 모드가 바뀔 때 호출될 함수 */
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
	
	/** 월드의 GridSystem 참조 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battle")
	TObjectPtr<AACGridSystem> GridSystem;

private:
	/** 기지 모드에서 키보드 이동 처리를 위한 함수 */
	void Move(const FInputActionValue& Value);

	/** 전투 모드에서 마우스 클릭 처리를 위한 함수 */
	void OnMouseClick(const FInputActionValue& Value);
	
private:
	bool bIsMovementMode = false;

	UPROPERTY()
	TArray<AACTile*> ValidRangeTiles;

	FIntPoint LastHoveredTilePos = FIntPoint(-999, -999);
};
