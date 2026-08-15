// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACDeckManagerComponent.generated.h"

class UPE_CardData;
class APE_CardActor;
class UPE_CardInstance;

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

	TArray<TObjectPtr<APE_CardActor>> GetHandCards() const { return HandCards; }

	/** 초기 덱 데이터 주입 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void InitializeDeck(const TArray<UPE_CardData*>& InitialDeckData);

	UFUNCTION(BlueprintCallable, Category = "Deck")
	void DrawCards(int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Deck")
	void DiscardCard(APE_CardActor* CardToDiscard);

	/** 핸드 제외 셔플 */
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void ShuffleDiscardToDraw();

	/** 핸드 정렬 */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void UpdateHandLayout();

public:
	/** [Drag] - 카드 재배치 */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void ReorderHandCards(APE_CardActor* InDraggedCard, APE_CardActor* TargetCard);

	/** [Drag] - 드래그 중인 카드 등록 */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void SetDraggedCard(APE_CardActor* InCard) { DraggedCard = InCard; }

	/** [Drag] - 드래그 중인 카드가 시전 구역(상단)에 진입했는지 여부 설정 */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void SetInCastingZone(bool bInZone);

	/** [Casting] - 시전 대기 중인 카드 등록 */
	UFUNCTION(BlueprintCallable, Category = "Deck|Layout")
	void SetCastingCard(APE_CardActor * InCard) { CastingCard = InCard; }

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
	TArray<TObjectPtr<UPE_CardInstance>> DrawPile;

	// 버린 카드 더미 (순수 데이터)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deck|State")
	TArray<TObjectPtr<UPE_CardInstance>> DiscardPile;

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

private:
	// [Drag] 시전 구역 진입 여부
	bool bInCastingZone = false;

	// [Drag] 현재 마우스로 잡고 있는 카드
	UPROPERTY()
	TObjectPtr<APE_CardActor> DraggedCard;

	// 시전 대기(중앙으로 띄워진) 중인 카드
	UPROPERTY()
	TObjectPtr<APE_CardActor> CastingCard;

public:
	/** 서버 검증을 기다리며 숨겨진 카드들 */
	void QueueCard(APE_CardActor* Card);
	void ConfirmQueuedCard(APE_CardActor* Card);
	void RevertQueuedCard(APE_CardActor* Card);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Deck|State", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<APE_CardActor>> QueuedCards;
};