// Copyright CrograNM

#include "UI/PE_DebugMapToolWidget.h"

#include "Core/PE_CheatManager.h"
#include "Core/PE_CheatComponent.h"
#include "Grid/ACTile.h"
#include "GameFramework/PlayerController.h"

void UPE_DebugMapToolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 생성 시 CheatManager의 델리게이트를 구독하여 외부 비활성화 신호를 감지
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UPE_CheatManager* CM = Cast<UPE_CheatManager>(PC->CheatManager))
		{
			CM->OnMapToolStateChanged.AddDynamic(this, &UPE_DebugMapToolWidget::OnCheatManagerStateChanged);
		}
	}
}

void UPE_DebugMapToolWidget::SetEditModeActive(bool bActive)
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UPE_CheatManager* CM = Cast<UPE_CheatManager>(PC->CheatManager))
		{
			CM->SetMapToolActive(bActive);
		}
	}
}

// 외부에서 강제로 상태가 바뀌었을 때 
void UPE_DebugMapToolWidget::OnCheatManagerStateChanged(bool bIsActive)
{
	bIsEditMode = bIsActive;
	OnEditModeStateChangedUI(bIsActive); // 블루프린트 체크박스 갱신 호출

	// 모드가 꺼지면 칠해뒀던 임시 호버링 색상을 삭제
	if (!bIsEditMode && LastHoveredTile.IsValid())
	{
		LastHoveredTile->SetHighlightState(ETileHighlightType::None);
		LastHoveredTile.Reset();
	}
}

void UPE_DebugMapToolWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsEditMode) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	AACTile* HoveredTile = Cast<AACTile>(HitResult.GetActor());

	// 마우스가 새로운 타일로 넘어갔을 때만 처리
	if (HoveredTile != LastHoveredTile.Get())
	{
		if (LastHoveredTile.IsValid())
		{
			LastHoveredTile->SetHighlightState(ETileHighlightType::None); // 이전 타일 복구
		}

		if (HoveredTile)
		{
			HoveredTile->SetHighlightState(ETileHighlightType::Hovered); // 새 타일 강조
		}
		LastHoveredTile = HoveredTile;
	}
}

FReply UPE_DebugMapToolWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bIsEditMode)
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
			UPE_CheatComponent* CheatNet = PC->FindComponentByClass<UPE_CheatComponent>();
			if (!CheatNet) return FReply::Handled();

			// 타일 상태 변경 조작
			switch (CurrentBrush)
			{
			case EPEDebugBrushType::Reset:
				CheatNet->Server_CheatSetTileObstacle(ClickedTile, false); 
				break;
			case EPEDebugBrushType::Obstacle:
				CheatNet->Server_CheatSetTileObstacle(ClickedTile, !ClickedTile->IsObstacle());
				break;
			case EPEDebugBrushType::Water:
				// TODO: ClickedTile->AddTileElement(ETileElement::Water); 
				break;
			case EPEDebugBrushType::Fire:
				// TODO: ClickedTile->AddTileElement(ETileElement::Fire); 
				break;
			}

			// Handled()를 반환하면 클릭이 여기서 "소비"됨 -> PlayerController 로직 발동 x
			return FReply::Handled(); 
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}