// Copyright CrograNM

#include "Components/ACCardInteractionComponent.h"
#include "Cards/PE_CardActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UACCardInteractionComponent::UACCardInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UACCardInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACCardInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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

	FVector WorldLocation, WorldDirection;
	// 마우스의 2D 화면 좌표를 3D 공간의 방향 벡터로 변환 (Deproject)
	if (PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// 카메라 위치에서 마우스 방향으로 DragDepth 만큼 떨어진 곳이 목표 3D 좌표
		FVector TargetLocation = WorldLocation + (WorldDirection * DragDepth);

		// VInterpTo를 사용해 찰지게 마우스를 따라오도록 보간 (Juicy 연출)
		FVector NewLocation = FMath::VInterpTo(GrabbedCard->GetActorLocation(), TargetLocation, GetWorld()->GetDeltaSeconds(), DragInterpSpeed);

		// 약간 비스듬하게 눕혀지는 회전 효과 추가 시 더욱 역동적
		FRotator TargetRotation = PC->PlayerCameraManager->GetCameraRotation();

		GrabbedCard->SetActorLocationAndRotation(NewLocation, TargetRotation);
	}
}

void UACCardInteractionComponent::GrabCard()
{
	if (HoveredCard)
	{
		GrabbedCard = HoveredCard;
		// 드래그 중일 때는 다른 UI나 타일에 방해받지 않도록 충돌체 임시 비활성화
		GrabbedCard->SetActorEnableCollision(false);

		UE_LOG(LogTemp, Warning, TEXT("카드 잡기: %s"), *GrabbedCard->GetName());
	}
}

void UACCardInteractionComponent::ReleaseCard()
{
	if (GrabbedCard)
	{
		UE_LOG(LogTemp, Warning, TEXT("카드 놓기: %s"), *GrabbedCard->GetName());

		// 충돌체 원상 복구
		GrabbedCard->SetActorEnableCollision(true);

		// TODO: 현재 마우스 위치가 '전장(적/타일)'인지 '허공'인지 레이캐스트로 2차 검사
		// 1. 적절한 사용 위치라면: Card->PlayCastingReadyAnimation() 및 스킬 발동 대기 상태 진입
		// 2. 사용 불가 위치라면: 취소 처리

		// 임시 취소 처리: 덱 매니저의 UpdateHandLayout()을 호출하면 알아서 원래 아치형 자리로 돌아감
		/*
		if (UACDeckManagerComponent* DeckManager = GetOwner()->FindComponentByClass<UACDeckManagerComponent>())
		{
			DeckManager->UpdateHandLayout();
		}
		*/

		GrabbedCard = nullptr;
	}
}