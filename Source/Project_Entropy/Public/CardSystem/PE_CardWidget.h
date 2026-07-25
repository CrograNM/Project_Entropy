// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PE_CardWidget.generated.h"

class UTextBlock;
class UImage;
class UPE_CardData;
class UPE_CardInstance;
class UPE_CardThemeData;

UCLASS()
class PROJECT_ENTROPY_API UPE_CardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 카드 액터(PE_CardActor)가 데이터를 주입할 때 호출하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Card|UI")
	void UpdateCardUI(UPE_CardInstance* InCardInstance);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Card UI")
	void OnCardUIUpdated(UPE_CardInstance* CardInstance, UPE_CardData* BaseData, UPE_CardThemeData* ThemeData);

	// 이 위젯이 참조할 테마 데이터 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card UI|Theme")
	TObjectPtr<UPE_CardThemeData> GlobalCardTheme;
};