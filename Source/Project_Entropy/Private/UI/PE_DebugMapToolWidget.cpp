// Copyright CrograNM

#include "UI/PE_DebugMapToolWidget.h"

#include "Core/PE_CheatManager.h"
#include "Grid/ACTile.h"
#include "GameFramework/PlayerController.h"

void UPE_DebugMapToolWidget::SetBrushType(EPEDebugBrushType NewBrush)
{
	CurrentBrush = NewBrush;
	
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UPE_CheatManager* CM = Cast<UPE_CheatManager>(PC->CheatManager))
		{
			// 브러시가 None이면 툴 비활성화, 다른 브러시면 활성화
			bool bIsToolActive = (CurrentBrush != EPEDebugBrushType::None);
			CM->SetMapToolActive(bIsToolActive);
		}
	}
}

FReply UPE_DebugMapToolWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 브러시가 'None'이라면 이벤트 처리를 포기(Unhandled)하여, 뒤에 있는 PlayerController에게 클릭 이벤트가 넘어가게 둠
	if (CurrentBrush == EPEDebugBrushType::None)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// 왼쪽 마우스 클릭이고 브러시가 활성화되어 있다면 동작
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		APlayerController* PC = GetOwningPlayer();
		if (!PC) return FReply::Handled();

		FHitResult HitResult;
		
		// UI 자체에서 커서 아래의 타일을 탐색합니다.
		PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

		if (AACTile* ClickedTile = Cast<AACTile>(HitResult.GetActor()))
		{
			// 타일 상태 변경 조작
			switch (CurrentBrush)
			{
			case EPEDebugBrushType::Obstacle:
				ClickedTile->SetObstacle(!ClickedTile->IsObstacle()); // 토글(On/Off)
				break;
			case EPEDebugBrushType::Water:
				// TODO: ClickedTile->SetTileElement(ETileElement::Water); 
				break;
			case EPEDebugBrushType::Fire:
				// TODO: ClickedTile->SetTileElement(ETileElement::Fire);
				break;
			}

			// Handled()를 반환하면 클릭이 여기서 "소비"됨 -> PlayerController 로직 발동 x
			return FReply::Handled(); 
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}