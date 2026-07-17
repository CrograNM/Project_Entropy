// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PE_DebugMapToolWidget.generated.h"

class AACTile;

UENUM(BlueprintType)
enum class EPEDebugBrushType : uint8
{
	Reset 		UMETA(DisplayName = "초기화"),
	Obstacle    UMETA(DisplayName = "장애물 생성"),
	Water       UMETA(DisplayName = "물 타일 생성"),
	Fire        UMETA(DisplayName = "불 타일 생성")
};

UCLASS()
class PROJECT_ENTROPY_API UPE_DebugMapToolWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 브러시 선택
	UFUNCTION(BlueprintCallable, Category = "Map Tool")
	void SetBrushType(EPEDebugBrushType NewBrush) { CurrentBrush = NewBrush; }

	// 편집 모드를 On/Off 하는 함수
	UFUNCTION(BlueprintCallable, Category = "Map Tool")
	void SetEditModeActive(bool bActive);
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	/** 마우스 클릭을 가로채는 핵심 UMG 함수 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	/** CheatManager에서 툴이 꺼졌을 때 블루프린트 체크박스 UI를 갱신하라고 던져주는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Map Tool")
	void OnEditModeStateChangedUI(bool bIsActive);
	
private:
	/** 치트 매니저의 상태 변경 신호를 받을 콜백 */
	UFUNCTION()
	void OnCheatManagerStateChanged(bool bIsActive);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Tool")
	EPEDebugBrushType CurrentBrush = EPEDebugBrushType::Obstacle; // 기본 브러시 설정

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Tool")
	bool bIsEditMode = false;

private:
	// 호버링 색상을 끄기 위해 기억해두는 이전 타일
	TWeakObjectPtr<AACTile> LastHoveredTile;
};