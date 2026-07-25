// Copyright CrograNM

#include "Components/ACCardInteractionComponent.h"
#include "Components/ACDeckManagerComponent.h"
#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_CardData.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UACCardInteractionComponent::UACCardInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.022222f; // 약 45fps로 Tick 최적화
}

void UACCardInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	PC = Cast<APlayerController>(GetOwner());
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
		case EPEInteractionState::Casting:
			ProcessCasting();
			break;
		case EPEInteractionState::Waiting:
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

	// 하단 1/3 지점을 넘었는지 실시간 판단 후 덱 매니저에 전달 (시전 구역 진입 여부)
	bool bIsOverCastingZone = MouseY < (ViewportSizeY * 0.66f);

	UACDeckManagerComponent* DeckManager = PC->FindComponentByClass<UACDeckManagerComponent>();
	if (DeckManager)
	{
		DeckManager->SetInCastingZone(bIsOverCastingZone);
	}

	// [스왑(정렬) 로직] - 카드가 손패 영역에 있을 때만
	if (!bIsOverCastingZone)
	{
		FHitResult HitResult;
		PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

		APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());
		if (HitCard != HoveredCardDuringDrag)
		{
			HoveredCardDuringDrag = HitCard;
			if (HoveredCardDuringDrag != nullptr && HoveredCardDuringDrag != GrabbedCard)
			{
				if (DeckManager)
				{
					DeckManager->ReorderHandCards(GrabbedCard, HoveredCardDuringDrag);
				}
			}
		}
	}
	else
	{
		// 시전 구역으로 올라갔을 때는 카드 간 스왑 타겟팅을 초기화합니다.
		HoveredCardDuringDrag = nullptr;
	}

	// [드래그 이동 로직]
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

void UACCardInteractionComponent::ProcessCasting()
{
	if (!PC || !CastingCard) return;

	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());

	// 마우스가 캐스팅 중인 카드 위에 올라갔을 때만
	if (HitCard == CastingCard)
	{
		if (HoveredCard != HitCard)
		{
			if (HoveredCard)
			{
				HoveredCard->SetHighlightState(false);
				HoveredCard->OnCastingHoverStateChanged(false);
			}

			// 외곽선 빛 켜기 + BP 연출(오프셋) 시작 알림
			HitCard->SetHighlightState(true, FLinearColor::Yellow);
			HitCard->OnCastingHoverStateChanged(true);

			HoveredCard = HitCard;
		}
	}
	else
	{
		// 캐스팅 카드 밖으로 마우스가 나가면 빛 끄기 + 오프셋 복구 알림
		if (HoveredCard)
		{
			HoveredCard->SetHighlightState(false);
			HoveredCard->OnCastingHoverStateChanged(false); 

			HoveredCard = nullptr;
		}
	}
}

void UACCardInteractionComponent::GrabCard()
{
	// [Casting] - 캐스팅 취소
	if (CurrentState == EPEInteractionState::Casting)
	{
		if (HoveredCard && HoveredCard == CastingCard)
		{
			CancelCasting();
		}
		return;
	}

	if (CurrentState != EPEInteractionState::Hovering) return;

	if (HoveredCard)
	{
		GrabbedCard = HoveredCard;
		GrabbedCard->SetActorEnableCollision(false); // 드래그 시 충돌 비활성화 (레이캐스트 방해 방지)
		
		GrabbedCard->SetHighlightState(false);
		GrabbedCard->SetHoverOffsetEnabled(false);

		// 상태 전환
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

	GrabbedCard->SetActorEnableCollision(true);

	UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>();

	if (PC && DeckManager)
	{
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

		float MouseX, MouseY;
		PC->GetMousePosition(MouseX, MouseY);

		// 마우스를 놓은 시점에 1/3 상단 지점에 있었는지 검사
		bool bWasInCastingZone = MouseY < (ViewportSizeY * 0.66f);

		if (bWasInCastingZone && !bIsSuspended)
		{
			UPE_CardInstance* CardInst = GrabbedCard->GetCardInstance();
			UPE_CardData* BaseData = CardInst ? CardInst->GetBaseCardData() : nullptr;

			if (false /* 즉발(Instant)일 경우 */)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardInteraction] 즉발 카드 사용"));
				// 사용했으므로 깔끔하게 비움
				DeckManager->SetDraggedCard(nullptr);
				DeckManager->SetInCastingZone(false);
				DeckManager->DiscardCard(GrabbedCard);
				CurrentState = EPEInteractionState::Hovering;
			}
			else // 지정(Target)형 스킬일 경우
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardInteraction] 캐스팅 시작"));

				CastingCard = GrabbedCard;
				CastingCard->CancelMoveToTarget();
				CurrentState = EPEInteractionState::Waiting;

				// 시전 카드로 먼저 꽂아 넣고 드래그를 해제 (** 순서 중요 **)
				DeckManager->SetCastingCard(CastingCard);
				DeckManager->SetDraggedCard(nullptr);

				// 마지막에 구역 진입 상태를 꺼주면 내부에서 UpdateHandLayout()이 발동
				DeckManager->SetInCastingZone(false);

				CastingCard->PlayCastingReadyAnimation();
			}
		}
		else
		{
			// 기준선 아래에서 놓았다면 사용 취소
			CurrentState = EPEInteractionState::Hovering;
			DeckManager->SetDraggedCard(nullptr);
			DeckManager->SetInCastingZone(false);
			DeckManager->UpdateHandLayout(); // 원래 자기 자리 이빨로 다시 쏙 들어감
		}
	}

	GrabbedCard = nullptr;
	HoveredCardDuringDrag = nullptr;
}

void UACCardInteractionComponent::OnCastingReadyFinished()
{
	// BP에서 애니메이션이 끝났다고 알려주면, 타겟팅 시작 상태로 전환
	if (CurrentState == EPEInteractionState::Waiting && CastingCard)
	{
		CurrentState = EPEInteractionState::Casting;
		UE_LOG(LogTemp, Warning, TEXT("[CardInteraction] 시전 대기 완료, 타겟팅 시작!"));
	}
}

void UACCardInteractionComponent::CancelCasting()
{
	// 시전 중일 때만 취소 가능 (우클릭 등)
	if (CurrentState == EPEInteractionState::Casting && CastingCard)
	{
		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->SetCastingCard(nullptr);
			DeckManager->UpdateHandLayout();
		}

		// BP 연출 이벤트 호출 (스윽 하고 원래 손패 자리로 돌아감)
		// 도착지는 위 UpdateHandLayout에서 자동으로 카드 액터의 TargetRelativeTransform에 세팅되었음
		CastingCard->PlayCancelCastingAnimation(CastingCard->GetActorTransform()); // 더미값 전달, 내부 C++ 틱 보간 사용 지시 등 응용 가능

		CastingCard = nullptr;
		CurrentState = EPEInteractionState::Hovering; // 상태 복구
		UE_LOG(LogTemp, Log, TEXT("[CardInteraction] 시전 취소됨. 빈 자리로 돌아갑니다."));
	}
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
		// if (HoveredCard) -->SetInteractionSuspended 에서 처리됨
		if (GrabbedCard) { ReleaseCard(); }
		if (CastingCard) { CancelCasting(); }

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
		else if (CurrentState == EPEInteractionState::Casting)
		{
			HoveredCard->OnCastingHoverStateChanged(false);
		}

		HoveredCard = nullptr;
	}
}