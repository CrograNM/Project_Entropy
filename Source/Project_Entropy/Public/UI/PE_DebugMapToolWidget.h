// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PE_DebugMapToolWidget.generated.h"

class AACTile;

UENUM(BlueprintType)
enum class EPEDebugBrushType : uint8
{
	None        UMETA(DisplayName = "선택 안함 (일반 조작)"),
	Obstacle    UMETA(DisplayName = "장애물 생성"),
	Water       UMETA(DisplayName = "물 타일 생성"),
	Fire        UMETA(DisplayName = "불 타일 생성")
};

UCLASS()
class PROJECT_ENTROPY_API UPE_DebugMapToolWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** UI 버튼을 통해 현재 마우스에 칠할 브러시 타입을 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "Map Tool")
	void SetBrushType(EPEDebugBrushType NewBrush);

protected:
	/** 마우스 클릭을 가로채는 핵심 UMG 함수 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Tool")
	EPEDebugBrushType CurrentBrush = EPEDebugBrushType::None;
};