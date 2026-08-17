// Copyright CrograNM

#include "Components/ACCardInteractionComponent.h"
#include "Components/ACDeckManagerComponent.h"
#include "Components/ACTargetingVisualizerComponent.h"
#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_DataTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PE_PlayerController.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACStatComponent.h"

UACCardInteractionComponent::UACCardInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.022222f; // 약 45fps로 Tick 최적화
}

void UACCardInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	PC = Cast<APE_PlayerController>(GetOwner());
}

void UACCardInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsSuspended) return;

	// 상태 기반 분기 처리
	switch (CurrentState) {
		case EPEInteractionState::Hovering:
			ProcessHovering();
			break;
		case EPEInteractionState::Selecting:
			ProcessDragging();
			break;
		case EPEInteractionState::Disabled:
			// 해당 상태에서는 마우스 트레이싱/드래그 연산을 멈춤 (최적화)
			break;
	}
}

void UACCardInteractionComponent::ProcessHovering()
{
	if (!PC) return;

	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());

	if (HitCard != HoveredCard)
	{
		// 이전 카드 호버링 해제
		if (HoveredCard)
		{
			HoveredCard->SetHighlightState(false);
			HoveredCard->SetHoverOffsetEnabled(false);
		}

		// 새 카드 호버링 적용
		if (HitCard)
		{
			HitCard->SetHighlightState(true, FLinearColor::Yellow);
			HitCard->SetHoverOffsetEnabled(true);
		}

		HoveredCard = HitCard;
	}
}

void UACCardInteractionComponent::ProcessDragging()
{
	if (!PC || !GrabbedCard) return;

	int32 ViewportSizeX, ViewportSizeY;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);

	// 하단 1/5 지점을 넘었는지 실시간 판단 후 덱 매니저에 전달 (시전 구역 진입 여부)
	bool bIsOverCastingZone = MouseY < (ViewportSizeY * 0.8f);

	UACDeckManagerComponent* DeckManager = PC->FindComponentByClass<UACDeckManagerComponent>();
	if (DeckManager) DeckManager->SetInCastingZone(bIsOverCastingZone);

	// 즉발 카드 처리 -> 시전 구역 진입 처리하지 않고 계속 드래그 모드 유지
	bool bIsInstantCast = false;
	UPE_SkillData* SkillData = GrabbedCard->GetSkillData();
	if (SkillData)
	{
		if (SkillData->TargetType == EPESkillTargetType::All_Enemies || SkillData->TargetType == EPESkillTargetType::Self)
		{
			bIsInstantCast = true;
		}
	}

	// 1. 상태 전이 및 연출 제어
	if (bIsInstantCast)
	{
		// 즉발 카드: 시전 구역 진입 시 애니메이션 대기 없이 즉시 시전 가능 상태로 설정
		if (bIsOverCastingZone)
		{
			bIsPreparingToCast = true;
			bIsCastingReadyAnimFinished = true;
			if (DeckManager) DeckManager->SetCastingCard(GrabbedCard);
			HoveredCardDuringDrag = nullptr;
		}
		else
		{
			bIsPreparingToCast = false;
			bIsCastingReadyAnimFinished = false;
			if (DeckManager) DeckManager->SetCastingCard(nullptr);
		}
	}
	else
	{
		// 타겟팅 카드: 기존처럼 중앙 고정 및 대기 애니메이션 재생 로직 유지
		if (bIsOverCastingZone)
		{
			if (!bIsPreparingToCast)
			{
				bIsPreparingToCast = true;
				bIsCastingReadyAnimFinished = false;
				if (DeckManager) DeckManager->SetCastingCard(GrabbedCard);

				GrabbedCard->CancelMoveToTarget();
				GrabbedCard->PlayCastingReadyAnimation();
			}
			HoveredCardDuringDrag = nullptr;
		}
		else
		{
			if (bIsPreparingToCast)
			{
				bIsPreparingToCast = false;
				bIsCastingReadyAnimFinished = false;
				if (DeckManager) DeckManager->SetCastingCard(nullptr);

				if (PC)
				{
					if (APE_PlayerCharacter* PlayerChar = PC->GetCachedPlayerCharacter())
					{
						if (UACTargetingVisualizerComponent* Visualizer = PlayerChar->GetTargetingVisualizer())
							Visualizer->ClearTargeting();
					}
				}

				GrabbedCard->StopCardAnimations();
			}
		}
	}

	// 2. 물리적 이동 및 손패 스왑 처리 (즉발 카드는 위치 상관없이 항상 허용, 타겟팅 카드는 시전 구역이 아닐 때만 허용)
	if (bIsInstantCast || !bIsOverCastingZone)
	{
		if (!bIsOverCastingZone)
		{
			FHitResult HitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
			APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());

			if (HitCard && DeckManager && !DeckManager->GetHandCards().Contains(HitCard)) HitCard = nullptr;

			if (HitCard != HoveredCardDuringDrag)
			{
				HoveredCardDuringDrag = HitCard;
				if (HoveredCardDuringDrag != nullptr && HoveredCardDuringDrag != GrabbedCard)
				{
					if (DeckManager) DeckManager->ReorderHandCards(GrabbedCard, HoveredCardDuringDrag);
				}
			}
		}

		FVector WorldLocation, WorldDirection;
		if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		{
			FVector TargetWorldLoc = WorldLocation + (WorldDirection * DragDepth);
			FRotator TargetWorldRot = PC->PlayerCameraManager->GetCameraRotation();
			FTransform TargetWorldTransform(TargetWorldRot, TargetWorldLoc);
			FTransform CameraTransform = PC->PlayerCameraManager->GetTransform();
			FTransform RelativeTargetTransform = TargetWorldTransform.GetRelativeTransform(CameraTransform);

			GrabbedCard->MoveToTargetTransform(RelativeTargetTransform);
		}
	}
}

void UACCardInteractionComponent::ForceGrabCardByKeyboard(APE_CardActor* TargetCard)
{
	if (CurrentState == EPEInteractionState::Disabled || !TargetCard) return;

	if (HoveredCard)
	{
		HoveredCard->SetHighlightState(false);
		HoveredCard->SetHoverOffsetEnabled(false);
		HoveredCard = nullptr;
	}
	// 기존에 쥐고 있던 카드가 있다면 안전하게 내려놓음
	if (GrabbedCard) CancelCasting();
	

	GrabbedCard = TargetCard;
	bIsKeyboardCasting = true;

	GrabbedCard->SetActorEnableCollision(false);
	GrabbedCard->SetHighlightState(false);
	GrabbedCard->SetHoverOffsetEnabled(false);

	bIsPreparingToCast = false;
	bIsCastingReadyAnimFinished = false;
	CurrentState = EPEInteractionState::Selecting;

	if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
	{
		DeckManager->SetDraggedCard(GrabbedCard);
	}
}

void UACCardInteractionComponent::NotifyCastingReadyAnimFinished()
{
	if (!bIsPreparingToCast || !GrabbedCard) return;

	bIsCastingReadyAnimFinished = true;

	UPE_SkillData* SkillData = GrabbedCard->GetSkillData();
	bool bIsInstantCast = false;
	if (SkillData && (SkillData->TargetType == EPESkillTargetType::All_Enemies || SkillData->TargetType == EPESkillTargetType::Self))
	{
		bIsInstantCast = true;
	}

	if (!bIsInstantCast && SkillData && PC)
	{
		if (APE_PlayerCharacter* PlayerChar = PC->GetCachedPlayerCharacter())
		{
			if (UACTargetingVisualizerComponent* Visualizer = PlayerChar->GetTargetingVisualizer())
				Visualizer->SetTargetingMode(ETargetingMode::Skill, SkillData->BaseRange, SkillData);
		}
	}
}

void UACCardInteractionComponent::GrabCard()
{
	if (CurrentState != EPEInteractionState::Hovering) return;

	if (HoveredCard)
	{
		GrabbedCard = HoveredCard;
		HoveredCard = nullptr;

		GrabbedCard->SetActorEnableCollision(false); // 드래그 시 충돌 비활성화 (레이캐스트 방해 방지)
		GrabbedCard->SetHighlightState(false);
		GrabbedCard->SetHoverOffsetEnabled(false);

		// 상태 전환
		bIsPreparingToCast = false;
		bIsCastingReadyAnimFinished = false;
		CurrentState = EPEInteractionState::Selecting; 

		// 드래그 중인 카드 등록 (강제 정렬 무효화)
		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->SetDraggedCard(GrabbedCard);
		}
	}
}

void UACCardInteractionComponent::ReleaseCard()
{
	if (CurrentState != EPEInteractionState::Selecting || !GrabbedCard) return;

	if (bIsPreparingToCast)
	{
		// 애니메이션이 완전히 끝나기 전에 마우스를 놓으면 시전 취소로 간주
		if (bIsCastingReadyAnimFinished)
		{
			if (PC) PC->TryExecuteCardDrop(GrabbedCard);
		}
		else
		{
			CancelCasting();
		}
	}
	else
	{
		CancelCasting();
	}
}

void UACCardInteractionComponent::CompleteCasting()
{
	if (GrabbedCard)
	{
		GrabbedCard->CancelMoveToTarget();
	}

	if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
	{
		DeckManager->SetCastingCard(nullptr);
		DeckManager->SetDraggedCard(nullptr);
		DeckManager->SetInCastingZone(false);
		DeckManager->UpdateHandLayout();
	}

	if (PC)
	{
		if (APE_PlayerCharacter* PlayerChar = PC->GetCachedPlayerCharacter())
		{
			if (UACTargetingVisualizerComponent* Visualizer = PlayerChar->GetTargetingVisualizer())
				Visualizer->ClearTargeting();
		}
	}

	bIsPreparingToCast = false;
	bIsCastingReadyAnimFinished = false;
	bIsKeyboardCasting = false; 
	GrabbedCard = nullptr;
	HoveredCard = nullptr; 
	HoveredCardDuringDrag = nullptr;
	CurrentState = EPEInteractionState::Hovering;
}

void UACCardInteractionComponent::CancelCasting()
{
	if (GrabbedCard)
	{
		GrabbedCard->CancelMoveToTarget();

		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->SetCastingCard(nullptr);
			DeckManager->SetDraggedCard(nullptr);
			DeckManager->SetInCastingZone(false);
			DeckManager->UpdateHandLayout();
		}

		if (PC)
		{
			if (APE_PlayerCharacter* PlayerChar = PC->GetCachedPlayerCharacter())
			{
				if (UACTargetingVisualizerComponent* Visualizer = PlayerChar->GetTargetingVisualizer())
					Visualizer->ClearTargeting();
			}
		}

		GrabbedCard->StopCardAnimations();
		GrabbedCard->SetActorEnableCollision(true);
	}

	bIsPreparingToCast = false;
	bIsCastingReadyAnimFinished = false;
	bIsKeyboardCasting = false;
	GrabbedCard = nullptr;
	HoveredCard = nullptr;
	HoveredCardDuringDrag = nullptr;
	CurrentState = EPEInteractionState::Hovering;
}

void UACCardInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	SetInteractionSuspended(!bEnabled);

	if (bEnabled)
	{
		CurrentState = EPEInteractionState::Hovering;
	}
	else
	{
		if (GrabbedCard) { CancelCasting(); }
		CurrentState = EPEInteractionState::Disabled;
	}
}

void UACCardInteractionComponent::SetInteractionSuspended(bool bSuspend)
{
	bIsSuspended = bSuspend;

	// 일시 정지
	if (bIsSuspended && HoveredCard)
	{
		HoveredCard->SetHighlightState(false);

		if (CurrentState == EPEInteractionState::Hovering)
		{
			HoveredCard->SetHoverOffsetEnabled(false);
		}

		HoveredCard = nullptr;
	}
}