// Copyright CrograNM

#include "CardSystem/PE_CardWidget.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_CardThemeData.h"
#include "CardSystem/PE_SkillData.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPE_CardWidget::UpdateCardUI(UPE_CardInstance* InCardInstance)
{
	// 데이터가 유효한지 검사
	if (!IsValid(InCardInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UPE_CardWidget] UpdateCardUI 실패: 인스턴스가 유효하지 않습니다."));
		return;
	}

	UPE_CardData* BaseData = InCardInstance->GetBaseCardData();
	if (!IsValid(BaseData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UPE_CardWidget] UpdateCardUI 실패: BaseCardData가 유효하지 않습니다."));
		return;
	}

	if (!IsValid(GlobalCardTheme))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UPE_CardWidget] GlobalCardTheme이 설정되지 않았습니다. BP 위젯 속성 확인 필요."));
	}

	// 실제 시각적 변경은 BP의 이벤트로 위임
	OnCardUIUpdated(InCardInstance, BaseData, GlobalCardTheme);
}