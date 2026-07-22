// Copyright CrograNM

#include "Components/ACDeckManagerComponent.h"
#include "Core/PE_PlayerController.h"
#include "Core/PE_RunManagerSubsystem.h"
#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardInstance.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

UACDeckManagerComponent::UACDeckManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UACDeckManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACDeckManagerComponent::InitializeDeck(const TArray<UPE_CardData*>& InitialDeckData)
{
	DrawPile.Empty();
	DiscardPile.Empty();
	HandCards.Empty();

	for (UPE_CardData* Data : InitialDeckData)
	{
		if (Data)
		{
			// NewObject를 통해 UObject 인스턴스 생성
			UPE_CardInstance* NewInstance = NewObject<UPE_CardInstance>(this);
			NewInstance->Initialize(Data);

			DrawPile.Add(NewInstance);
		}
	}

	// 셔플 로직
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UPE_RunManagerSubsystem* RunManager = GI->GetSubsystem<UPE_RunManagerSubsystem>())
		{
			// 인스턴스 배열을 셔플
			DrawPile.Sort([RunManager](const UPE_CardInstance& A, const UPE_CardInstance& B) {
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
	FString ErrorMsg = TEXT("더 이상 카드를 뽑을 수 없습니다.");

	int32 CurrentHandSize = HandCards.Num();
	if (CurrentHandSize >= MaxHandSize)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOwner()))
		{
			PC->ShowToastMessage(FText::FromString(ErrorMsg));
		}
		return;
	}

	int32 AvailableSpace = MaxHandSize - CurrentHandSize;
	int32 ActualDrawCount = FMath::Min(Count, AvailableSpace);

	for (int32 i = 0; i < ActualDrawCount; ++i)
	{
		// 뽑을 카드가 없다면 무덤을 섞음
		if (DrawPile.IsEmpty())
		{
			ShuffleDiscardToDraw();

			// 섞었는데도 비어있다면(덱 0장) 드로우 종료
			if (DrawPile.IsEmpty()) break;
		}

		// 덱의 맨 위(마지막 인덱스)에서 카드 데이터를 꺼냄
		UPE_CardInstance* DrawnCardInstance = DrawPile.Pop();
		OnDrawPileCountChanged.Broadcast(DrawPile.Num());

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		FTransform SpawnTransform = FTransform::Identity;
		APE_CardActor* NewCardActor = GetWorld()->SpawnActor<APE_CardActor>(CardActorClass, SpawnTransform, SpawnParams);

		if (NewCardActor)
		{
			// 생성된 카드를 플레이어의 카메라 컴포넌트에 부착 (Attach)
			if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					if (UCameraComponent* CameraComp = Pawn->FindComponentByClass<UCameraComponent>())
					{
						// 카메라에 부착하되, 트랜스폼은 상대 좌표를 유지하도록 설정
						NewCardActor->AttachToComponent(CameraComp, FAttachmentTransformRules::KeepRelativeTransform);
					}
				}
			}

			// 카드 액터에게 인스턴스를 통째로 넘겨주고 핸드 배열에 추가
			NewCardActor->InitializeCard(DrawnCardInstance);
			HandCards.Add(NewCardActor);
		}
	}

	if (ActualDrawCount < Count)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(GetOwner()))
		{
			PC->ShowToastMessage(FText::FromString(ErrorMsg));
		}
	}

	// 드로우가 끝난 뒤 손패를 재정렬
	UpdateHandLayout();
}

void UACDeckManagerComponent::DiscardCard(APE_CardActor* CardToDiscard)
{
	if (!CardToDiscard || !HandCards.Contains(CardToDiscard)) return;

	// 손패에서 제거하고 무덤에 인스턴스 추가
	HandCards.Remove(CardToDiscard);
	DiscardPile.Add(CardToDiscard->GetCardInstance());

	OnDiscardPileCountChanged.Broadcast(DiscardPile.Num());

	// 남은 손패 재정렬
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
			// 인스턴스 배열 셔플
			DrawPile.Sort([RunManager](const UPE_CardInstance& A, const UPE_CardInstance& B) {
				return RunManager->GetRandomBool();
				});
		}
	}

	OnDeckShuffled.Broadcast();
	OnDrawPileCountChanged.Broadcast(DrawPile.Num());
	OnDiscardPileCountChanged.Broadcast(DiscardPile.Num());
}

void UACDeckManagerComponent::UpdateHandLayout()
{
	int32 CardCount = HandCards.Num();
	if (CardCount == 0) return;

	// 동적 간격(Spacing) 및 압축 비율 계산
	float CurrentSpacing = BaseCardSpacing;
	float DesiredWidth = (CardCount - 1) * BaseCardSpacing;
	float SqueezeRatio = 0.f; // 0.0 (안 좁아짐) ~ 1.0 (최대로 좁아짐)

	// 카드가 많아져서 최대 너비를 초과하면 간격을 좁힘
	if (DesiredWidth > MaxHandWidth && CardCount > 1)
	{
		CurrentSpacing = MaxHandWidth / (CardCount - 1);
		SqueezeRatio = 1.f - (CurrentSpacing / BaseCardSpacing);
	}

	// 전체 손패의 시작점(가장 왼쪽) 오프셋 계산
	float OffsetY = (CardCount - 1) * CurrentSpacing * -0.5f;

	for (int32 i = 0; i < CardCount; ++i)
	{
		APE_CardActor* Card = HandCards[i];
		if (!Card) continue;
		if (Card == DraggedCard) continue;
		
		// --- 위치(Location) 계산 ---

		// 깊이(X축): 나중에 뽑은 카드(오른쪽)가 미세하게 앞쪽에 배치되어 겹치도록 설정
		float TargetX = i * DepthSpacing;

		// 좌우(Y축): 줄어든 간격(CurrentSpacing)을 반영하여 배치
		float TargetY = OffsetY + (i * CurrentSpacing);

		// 상하(Z축): 중심에서 멀어질수록 아래로 내려가는 아치형 포물선 궤적
		float NormalizedIndex = (CardCount > 1) ? ((float)i / (CardCount - 1)) * 2.f - 1.f : 0.f;
		float TargetZ = -FMath::Abs(NormalizedIndex * NormalizedIndex) * ArchCurveHeight;

		// --- 회전(Rotation) 계산 ---

		// 부채꼴 회전(Roll)
		float TargetRoll = NormalizedIndex * FanAngle;

		// 카드가 압축될수록(SqueezeRatio 상승) 도미노처럼 비스듬히 겹쳐지는 회전(Yaw) 추가
		float TargetYaw = SqueezeRatio * SqueezeTiltAngle;

		// 로컬 트랜스폼을 계산한 뒤, BaseHandOffset(기준점)과 곱하여 최종 상대 좌표 산출
		FTransform LayoutTransform;
		LayoutTransform.SetLocation(FVector(TargetX, TargetY, TargetZ));
		LayoutTransform.SetRotation(FRotator(0.f, TargetYaw, TargetRoll).Quaternion());

		// 언리얼의 FTransform 곱셈: LayoutTransform을 BaseHandOffset 기준으로 적용
		FTransform FinalRelativeTransform = LayoutTransform * BaseHandOffset;

		// 카드에게 '상대 좌표계'에서의 이동을 명령
		Card->MoveToTargetTransform(FinalRelativeTransform);
	}
}

void UACDeckManagerComponent::ReorderHandCards(APE_CardActor* InDraggedCard, APE_CardActor* TargetCard)
{
	if (!InDraggedCard || !TargetCard || InDraggedCard == TargetCard) return;

	int32 OldIndex = HandCards.Find(InDraggedCard);
	int32 TargetIndex = HandCards.Find(TargetCard);

	if (OldIndex != INDEX_NONE && TargetIndex != INDEX_NONE)
	{
		// 1. 드래그 중인 카드를 배열에서 잠시 뺍니다.
		HandCards.RemoveAt(OldIndex);

		// 2. 빠진 후 타겟 카드의 새로운 인덱스를 찾습니다.
		int32 NewTargetIndex = HandCards.Find(TargetCard);

		// 3. 우측으로 드래그했으면 타겟의 오른쪽(뒤)에, 좌측으로 했으면 타겟의 왼쪽(앞)에 삽입(Insert)합니다.
		int32 InsertIndex = (OldIndex < TargetIndex) ? NewTargetIndex + 1 : NewTargetIndex;
		HandCards.Insert(InDraggedCard, InsertIndex);

		// 4. 재배치된 배열을 바탕으로 나머지 카드들의 위치를 갱신합니다.
		UpdateHandLayout();
	}
}
