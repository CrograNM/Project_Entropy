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

	// 상태 기반 분기 처리
	switch (CurrentState) {
		case EPEInteractionState::Hovering:
			ProcessHovering();
			break;
		case EPEInteractionState::Selecting:
			ProcessDragging();
			break;
		case EPEInteractionState::Waiting:
		case EPEInteractionState::Casting:
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
		}

		// 새 카드 호버링 적용
		if (HitCard)
		{
			HitCard->SetHighlightState(true, FLinearColor::Yellow);
		}

		HoveredCard = HitCard;
	}
}

void UACCardInteractionComponent::ProcessDragging()
{
	if (!PC || !GrabbedCard) return;

	// [정렬 스왑 로직] - 마우스 아래에 다른 카드가 있는지 확인
	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());
	if (HitCard != HoveredCardDuringDrag)
	{
		HoveredCardDuringDrag = HitCard;

		// 실제 새로운 카드에 '진입(Enter)' 했을 때만 스왑
		if (HoveredCardDuringDrag != nullptr && HoveredCardDuringDrag != GrabbedCard)
		{
			if (UACDeckManagerComponent* DeckManager = PC->FindComponentByClass<UACDeckManagerComponent>())
			{
				DeckManager->ReorderHandCards(GrabbedCard, HoveredCardDuringDrag);
			}
		}
	}

	// [드래그 이동 로직] - 마우스의 월드 좌표를 카메라 상대 좌표로 변환하여 부드럽게 이동
	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// 카메라로부터 DragDepth만큼 떨어진 월드 목표 좌표
		FVector TargetWorldLoc = WorldLocation + (WorldDirection * DragDepth);
		FRotator TargetWorldRot = PC->PlayerCameraManager->GetCameraRotation();
		FTransform TargetWorldTransform(TargetWorldRot, TargetWorldLoc);

		// 플레이어 카메라의 트랜스폼 가져오기
		FTransform CameraTransform = PC->PlayerCameraManager->GetTransform();

		// 월드 좌표 -> 카메라 기준 로컬(Relative) 좌표로 변환
		FTransform RelativeTargetTransform = TargetWorldTransform.GetRelativeTransform(CameraTransform);

		// SetActorLocation이 아닌, 카드 액터 자체의 보간 함수에 목표점 하달
		GrabbedCard->MoveToTargetTransform(RelativeTargetTransform);
	}
}

void UACCardInteractionComponent::GrabCard()
{
	if (CurrentState != EPEInteractionState::Hovering) return;

	if (HoveredCard)
	{
		GrabbedCard = HoveredCard;
		GrabbedCard->SetActorEnableCollision(false); // 드래그 시 충돌 비활성화 (레이캐스트 방해 방지)
		
		GrabbedCard->SetHighlightState(false);

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

		// 마우스 Y좌표가 화면 하단 1/3 (약 66%) 위쪽인지 검사
		if (MouseY < (ViewportSizeY * 0.66f))
		{
			UPE_CardInstance* CardInst = GrabbedCard->GetCardInstance();
			UPE_CardData* BaseData = CardInst ? CardInst->GetBaseCardData() : nullptr;

			if (BaseData)
			{
				// TODO: 실제로는 BaseData->TargetType 을 통해 즉발/지정 분기 처리
				if (false /* 즉발(Instant)일 경우 */)
				{
					DeckManager->SetDraggedCard(nullptr);
					DeckManager->DiscardCard(GrabbedCard);
					CurrentState = EPEInteractionState::Hovering;
				}
				else // 지정(Target)형 스킬일 경우
				{
					CastingCard = GrabbedCard;
					CastingCard->CancelMoveToTarget(); // 애니메이션을 위해 객체 자체의 보간 연산 중지
					CurrentState = EPEInteractionState::Waiting; // 애니메이션 대기 상태로 진입 (입력 락)

					// BP 연출 이벤트 호출
					CastingCard->PlayCastingReadyAnimation();
				}
			}
		}
		else
		{
			// 기준선 아래에서 놓았다면 사용 취소 (원래 자리로 롤백)
			CurrentState = EPEInteractionState::Hovering; // 상태 복구
			DeckManager->SetDraggedCard(nullptr);
			DeckManager->UpdateHandLayout();
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
			// 덱 매니저의 무시(DraggedCard) 상태를 해제하고 재정렬 지시
			DeckManager->SetDraggedCard(nullptr);
			DeckManager->UpdateHandLayout();
		}

		// BP 연출 이벤트 호출 (스윽 하고 원래 손패 자리로 돌아감)
		// 도착지는 위 UpdateHandLayout에서 자동으로 카드 액터의 TargetRelativeTransform에 세팅되었음
		CastingCard->PlayCancelCastingAnimation(CastingCard->GetActorTransform()); // 더미값 전달, 내부 C++ 틱 보간 사용 지시 등 응용 가능

		CastingCard = nullptr;
		CurrentState = EPEInteractionState::Hovering; // 상태 복구
		UE_LOG(LogTemp, Log, TEXT("[CardInteraction] 시전 취소됨."));
	}
}

void UACCardInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		CurrentState = EPEInteractionState::Hovering;
	}
	else
	{
		CurrentState = EPEInteractionState::Disabled;

		if (HoveredCard) { HoveredCard->SetHighlightState(false); HoveredCard = nullptr; }
		if (GrabbedCard) { ReleaseCard(); }
		if (CastingCard) { CancelCasting(); }
	}
}
