// Copyright CrograNM

#include "Components/ACDeckManagerComponent.h"
#include "Cards/PE_CardActor.h"
#include "Cards/PE_CardData.h"
#include "Kismet/GameplayStatics.h"
#include "Core/PE_RunManagerSubsystem.h"

UACDeckManagerComponent::UACDeckManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACDeckManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACDeckManagerComponent::InitializeDeck(const TArray<UPE_CardData*>& InitialDeck)
{
	DrawPile = InitialDeck;
	DiscardPile.Empty();
	HandCards.Empty();

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UPE_RunManagerSubsystem* RunManager = GI->GetSubsystem<UPE_RunManagerSubsystem>())
		{
			DrawPile.Sort([RunManager](const UPE_CardData& A, const UPE_CardData& B) {
				return RunManager->GetRandomBool();
				});
		}
	}

	OnDrawPileCountChanged.Broadcast(DrawPile.Num());
	OnDiscardPileCountChanged.Broadcast(DiscardPile.Num());
}

void UACDeckManagerComponent::DrawCards(int32 Count)
{
	if (!CardActorClass || Count <= 0) return;

	for (int32 i = 0; i < Count; ++i)
	{
		// 1. 뽑을 카드가 없다면 무덤을 섞음
		if (DrawPile.IsEmpty())
		{
			ShuffleDiscardToDraw();

			// 섞었는데도 비어있다면(덱 0장) 드로우 종료
			if (DrawPile.IsEmpty()) break;
		}

		// 2. 덱의 맨 위(마지막 인덱스)에서 카드 데이터를 꺼냄
		UPE_CardData* DrawnCardData = DrawPile.Pop();
		OnDrawPileCountChanged.Broadcast(DrawPile.Num());

		// 3. 실제 3D 액터 생성 (카메라가 가지고 있는 컴포넌트이므로 Owner 위치 근처에서 스폰)
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();

		FTransform SpawnTransform = GetOwner()->GetActorTransform(); // 일단 부모(카메라/거치대) 위치에 스폰
		APE_CardActor* NewCardActor = GetWorld()->SpawnActor<APE_CardActor>(CardActorClass, SpawnTransform, SpawnParams);

		if (NewCardActor)
		{
			// 데이터 주입 및 손패 배열에 추가
			NewCardActor->InitializeCard(DrawnCardData);
			HandCards.Add(NewCardActor);

			// TODO: NewCardActor->PlayDrawAnimation(...) 호출 
			// (오른쪽으로 쌓이며 빛 알갱이가 카드로 변하는 연출 지시)
		}
	}

	// 드로우가 끝난 뒤 손패를 예쁘게 재정렬
	UpdateHandLayout();
}

void UACDeckManagerComponent::DiscardCard(APE_CardActor* CardToDiscard)
{
	if (!CardToDiscard || !HandCards.Contains(CardToDiscard)) return;

	// 1. 손패에서 제거하고 무덤에 데이터 추가
	HandCards.Remove(CardToDiscard);
	DiscardPile.Add(CardToDiscard->GetCardData());

	OnDiscardPileCountChanged.Broadcast(DiscardPile.Num());

	// 2. 카드 액터에게 산화 연출 지시
	// TODO: CardToDiscard->PlayDiscardAnimation(...) 호출

	// 연출이 끝난 뒤 액터를 파괴하는 로직은 CardActor 내부의 애니메이션 종료 이벤트에서 처리하도록 위임

	// 3. 남은 손패 재정렬
	UpdateHandLayout();
}

void UACDeckManagerComponent::ShuffleDiscardToDraw()
{
	if (DiscardPile.IsEmpty()) return;

	// 무덤의 데이터를 덱으로 옮기고 비움
	DrawPile.Append(DiscardPile);
	DiscardPile.Empty();

	// 셔플
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UPE_RunManagerSubsystem* RunManager = GI->GetSubsystem<UPE_RunManagerSubsystem>())
		{
			DrawPile.Sort([RunManager](const UPE_CardData& A, const UPE_CardData& B) {
				return RunManager->GetRandomBool();
				});
		}
	}

	OnDeckShuffled.Broadcast(); // 셔플 이펙트 연출 지시
	OnDrawPileCountChanged.Broadcast(DrawPile.Num());
	OnDiscardPileCountChanged.Broadcast(DiscardPile.Num());
}

void UACDeckManagerComponent::UpdateHandLayout()
{
	int32 CardCount = HandCards.Num();
	if (CardCount == 0) return;

	// 카드가 한 장일 때는 중앙(0), 여러 장일 때는 대칭 오프셋을 구함
	float OffsetX = (CardCount - 1) * CardSpacing * -0.5f;

	for (int32 i = 0; i < CardCount; ++i)
	{
		APE_CardActor* Card = HandCards[i];
		if (!Card) continue;

		// 1. 좌우(X축) 간격 계산
		float TargetX = OffsetX + (i * CardSpacing);

		// 2. 상하(Y축, 아치형 궤적) 계산: 중심에서 멀어질수록 아래로 내려감 (포물선 y = -x^2 형태)
		// 중심을 0으로 맞춘 Normalized Index (-1.0 ~ 1.0)
		float NormalizedIndex = (CardCount > 1) ? ((float)i / (CardCount - 1)) * 2.f - 1.f : 0.f;
		float TargetY = -FMath::Abs(NormalizedIndex * NormalizedIndex) * ArchCurveHeight;

		// 3. 회전(부채꼴) 계산: 좌측 카드는 오른쪽으로, 우측 카드는 왼쪽으로 기울어짐
		float TargetRoll = NormalizedIndex * FanAngle;

		// 최종 목표 로컬 Transform 생성
		FTransform TargetTransform;
		TargetTransform.SetLocation(FVector(0.f, TargetX, TargetY));
		TargetTransform.SetRotation(FRotator(0.f, TargetRoll, 0.f).Quaternion());

		// C++에서 위치를 즉시 세팅하지 않고, Card 액터에게 "이 위치로 부드럽게 이동해!"라고 명령
		// Card->MoveToTargetTransform(TargetTransform); 
	}
}