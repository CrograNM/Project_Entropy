// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACDeckManagerComponent.generated.h"

class UPE_CardData;
class APE_CardActor;

// UI 및 3D 더미 액터에게 카드 장수 변경을 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPileCountChangedSignature, int32, NewCount);
// 셔플 연출 시작을 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeckShuffledSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACDeckManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACDeckManagerComponent();

	/** 전투 시작 시 초기 덱 데이터 주입 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void InitializeDeck(const TArray<UPE_CardData*>& InitialDeck);

	/** 지정된 장수만큼 카드를 뽑습니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void DrawCards(int32 Count = 1);

	/** 특정 카드를 사용 또는 강제로 버립니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void DiscardCard(APE_CardActor* CardToDiscard);

	/** 버린 카드 더미를 뽑을 카드 더미로 섞어 넣습니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void ShuffleDiscardToDraw();

	/** 현재 손패에 있는 카드들의 3D 위치(아치형 등)를 다시 계산하여 정렬을 지시합니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void UpdateHandLayout();

public:
	/** [Drag] 드래그 중인 카드를 등록하여 강제 정렬(UpdateHandLayout)에서 제외합니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void SetDraggedCard(APE_CardActor* InCard) { DraggedCard = InCard; }

	/** [Drag] 두 카드의 배열 내 순서를 밀어내기(Shift) 방식으로 재배치합니다. */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void ReorderHandCards(APE_CardActor* InDraggedCard, APE_CardActor* TargetCard);

private:
	// [Drag] 현재 마우스로 잡고 있는 카드 (레이아웃 자동 정렬에서 무시됨)
	UPROPERTY()
	TObjectPtr<APE_CardActor> DraggedCard;

public:
	// --- 이벤트 방송국 (블루프린트 UI나 3D 카드더미 액터가 구독할 이벤트) ---
	UPROPERTY(BlueprintAssignable, Category = "Deck|Events")
	FOnPileCountChangedSignature OnDrawPileCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Deck|Events")
	FOnPileCountChangedSignature OnDiscardPileCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "Deck|Events")
	FOnDeckShuffledSignature OnDeckShuffled;

protected:
	virtual void BeginPlay() override;

	/** 드로우 시 스폰할 3D 카드 액터 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Deck|Setup")
	TSubclassOf<APE_CardActor> CardActorClass;

	// --- 카드 컬렉션 데이터 ---

	// 뽑을 카드 더미 (아직 스폰되지 않은 순수 데이터)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deck|State")
	TArray<TObjectPtr<UPE_CardData>> DrawPile;

	// 버린 카드 더미 (순수 데이터)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deck|State")
	TArray<TObjectPtr<UPE_CardData>> DiscardPile;

	// 현재 손패 (실제로 스폰되어 화면에 존재하는 3D 액터들)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deck|State")
	TArray<TObjectPtr<APE_CardActor>> HandCards;

	// --- 레이아웃(정렬) 세팅값 ---
	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	int32 MaxHandSize = 10;

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float BaseCardSpacing = 120.f; // 카드가 적을 때의 기본 간격

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float MaxHandWidth = 800.f; // 손패가 차지할 수 있는 최대 너비 (이 값을 넘으면 간격이 압축됨)

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float DepthSpacing = 2.f; // 카드가 겹칠 때 깜빡임(Z-Fighting)을 막기 위한 앞뒤 간격차

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float ArchCurveHeight = 15.f; // 둥근 아치형의 높이

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float FanAngle = 5.f; // 카드가 부채꼴로 벌어지는 각도 (Roll)

	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	float SqueezeTiltAngle = 15.f; // 간격이 좁아질 때 카드가 비스듬히 눕혀지는 최대 각도 (Yaw)

	/**	
		손패 오프셋 - 카메라 렌즈를 기준으로 손패 묶음이 위치할 기본 3D 오프셋 (상대 좌표)
		* X: 렌즈 앞으로의 거리 (예: 50.f)
		* Y: 좌우 오프셋 (보통 0)
		* Z: 상하 오프셋 (화면 아래쪽이므로 보통 -30.f 등 음수값) 
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Deck|Layout")
	FTransform BaseHandOffset = FTransform(FRotator(0.f, 0.f, 0.f), FVector(50.f, 0.f, -30.f));
};