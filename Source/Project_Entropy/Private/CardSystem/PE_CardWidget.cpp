// Copyright CrograNM

#include "CardSystem/PE_CardWidget.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_SkillData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPE_CardWidget::UpdateCardUI(const UPE_CardData* CardData)
{
	// 데이터가 유효한지 검사
	if (!CardData) return;

	if (CardName_Text)
	{
		CardName_Text->SetText(CardData->CardName);
	}

	if (Description_Text)
	{
		Description_Text->SetText(CardData->CardDescription);
	}

	if (Art_Image && CardData->CardArt)
	{
		Art_Image->SetBrushFromTexture(CardData->CardArt);
	}

	if (Cost_Text && CardData->SkillDataToCast)
	{
		int32 APCost = CardData->SkillDataToCast->BaseAPCost;
		Cost_Text->SetText(FText::AsNumber(APCost));
	}
}