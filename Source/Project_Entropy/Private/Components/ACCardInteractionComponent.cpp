// Copyright CrograNM

#include "Components/ACCardInteractionComponent.h"
#include "Components/ACDeckManagerComponent.h"
#include "CardSystem/PE_CardActor.h"
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

		APlayerController* PC = Cast<APlayerController>(GetOwner());
		UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>();

		if (PC && DeckManager)
		{
			// 뷰포트 크기
			int32 ViewportSizeX, ViewportSizeY;
			PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

			// 마우스 위치
			float MouseX, MouseY;
			PC->GetMousePosition(MouseX, MouseY);

			// 마우스 Y좌표가 화면 하단 1/3 위쪽인지 검사
			if (MouseY < (ViewportSizeY * 0.66f))
			{
				UPE_CardData* CardData = GrabbedCard->GetCardData();

				// 즉발 스킬 / 타겟팅 스킬 분기
				// [임시] - CardType이 Power(버프/즉발)면 즉시 사용, Attack(공격)이면 타겟팅 대기로 가정
				if (CardData && CardData->CardType == EPECardType::Power)
				{
					// [즉발 스킬 처리]
					UE_LOG(LogTemp, Warning, TEXT("즉시 시전!: %s"), *CardData->CardName.ToString());

					// TODO: 스킬 이펙트 발동 로직 호출

					DeckManager->SetDraggedCard(nullptr);
					DeckManager->DiscardCard(GrabbedCard);
				}
				else
				{
					// [타겟팅 스킬: 시전 대기 상태 돌입]
					UE_LOG(LogTemp, Warning, TEXT("시전 대기 (타겟팅 모드): %s"), *CardData->CardName.ToString());

					// 덱 매니저의 정렬 로직에서 이 카드를 계속 무시하도록 유지한 채,
					// 시전 대기 전용 좌표(좌측 중앙)로 카드를 쇽! 하고 날려보냅니다.
					GrabbedCard->MoveToTargetTransform(CastingReadyOffset);

					// TODO: 플레이어 컨트롤러의 모드를 '타겟팅 모드'로 변경하고, 타겟팅 화살표(Spline) 그리기 시작
				}
			}
			else
			{
				// 기준선 아래에서 놓았다면 사용 취소 (원래 자리로 롤백)
				UE_LOG(LogTemp, Log, TEXT("사용 취소, 원래 손패로 돌아갑니다."));
				DeckManager->SetDraggedCard(nullptr);
				DeckManager->UpdateHandLayout(); // 비워져 있던 원래 자기 자리로 스르륵 돌아감
			}
		}
		GrabbedCard = nullptr;
		HoveredCardDuringDrag = nullptr;
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

		HoveredCardDuringDrag = nullptr;

		// TODO: 시전 준비 중이던 카드가 있다면 시전 취소 처리
	}
}
