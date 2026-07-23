// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PE_CardWidget.generated.h"

class UTextBlock;
class UImage;
class UPE_CardData;

UCLASS()
class PROJECT_ENTROPY_API UPE_CardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 카드 액터(PE_CardActor)가 데이터를 주입할 때 호출하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Card|UI")
	void UpdateCardUI(const UPE_CardData* CardData);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CardName_Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Description_Text;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UTextBlock> Cost_Text;

	UPROPERTY(meta = (BindWidget, OptionalWidget = true))
	TObjectPtr<UImage> Art_Image;
};