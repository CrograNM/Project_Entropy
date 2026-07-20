// Copyright CrograNM

#include "Components/ACCardInteractionComponent.h"
#include "Components/ACDeckManagerComponent.h"
#include "Cards/PE_CardActor.h"
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
}

void UACCardInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsInteractionEnabled) return;

	// 카드를 쥐고 있다면 드래그 로직, 아니라면 호버링 탐색
	if (GrabbedCard)
	{
		ProcessDragging();
	}
	else
	{
		ProcessHovering();
	}
}

void UACCardInteractionComponent::ProcessHovering()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC) return;

	FHitResult HitResult;
	// 카드 메쉬는 BlockAllDynamic 등으로 설정되어 있어야 감지됨
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());

	if (HitCard != HoveredCard)
	{
		// 이전 카드 호버링 해제
		if (HoveredCard)
		{
			HoveredCard->SetHighlightState(false);
			// TODO: HoveredCard->PlayUnhoverAnimation(); (아래로 살짝 내려가는 연출)
		}

		// 새 카드 호버링 적용
		if (HitCard)
		{
			HitCard->SetHighlightState(true, FLinearColor::Yellow);
			// TODO: HitCard->PlayHoverAnimation(); (위로 살짝 들썩이는 연출)
		}

		HoveredCard = HitCard;
	}
}

void UACCardInteractionComponent::ProcessDragging()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !GrabbedCard) return;

	// [정렬 스왑 로직] - 마우스 아래에 다른 카드가 있는지 확인
	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult);

	APE_CardActor* HitCard = Cast<APE_CardActor>(HitResult.GetActor());
	if (HitCard && HitCard != GrabbedCard)
	{
		// 다른 카드를 감지했다면 즉시 배열 인덱스를 밀어내고 레이아웃을 갱신합니다.
		if (UACDeckManagerComponent* DeckManager = PC->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->ReorderHandCards(GrabbedCard, HitCard);
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
	if (!bIsInteractionEnabled) return;

	if (HoveredCard)
	{
		GrabbedCard = HoveredCard;
		GrabbedCard->SetActorEnableCollision(false); // 드래그 시 충돌 비활성화 (레이캐스트 방해 방지)

		// 드래그 중인 카드를 DeckManager에 등록 (강제 정렬 무효화)
		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->SetDraggedCard(GrabbedCard);
		}
	}
}

void UACCardInteractionComponent::ReleaseCard()
{
	if (GrabbedCard)
	{
		GrabbedCard->SetActorEnableCollision(true);	// 드래그 종료 시 충돌 활성화

		// TODO: 사용 불가 위치(허공 등)라면 취소 처리 -> 원래 손패 자리로 복귀
		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->SetDraggedCard(nullptr); // 드래그 해제
			DeckManager->UpdateHandLayout();      // 비워져 있던 원래 자기 자리로 스르륵 돌아감
		}

		GrabbedCard = nullptr;
	}
}

void UACCardInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	bIsInteractionEnabled = bEnabled;

	// 비활성화될 때, 쥐고 있거나 호버링 중인 카드가 있다면 즉시 취소 처리
	if (!bIsInteractionEnabled)
	{
		if (HoveredCard)
		{
			HoveredCard->SetHighlightState(false);
			HoveredCard = nullptr;
		}

		if (GrabbedCard)
		{
			ReleaseCard();
		}

		// TODO: 시전 준비 중이던 카드가 있다면 시전 취소 처리
	}
}
