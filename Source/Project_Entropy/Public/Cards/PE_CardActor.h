// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PE_CardActor.generated.h"

class UPE_CardData;
class UStaticMeshComponent;
class UWidgetComponent;
class UNiagaraComponent;

/** 카드의 현재 3D 공간 상 상태 */
UENUM(BlueprintType)
enum class EPECardVisualState : uint8
{
	InDeck      UMETA(DisplayName = "InDeck, 덱 안에 있음 (숨김)"),
	Drawing     UMETA(DisplayName = "Drawing, 드로우 중 (빛 알갱이 -> 카드)"),
	InHand      UMETA(DisplayName = "InHand, 손패 대기 중"),
	Dragging    UMETA(DisplayName = "Dragging, 드래그 중"),
	Casting     UMETA(DisplayName = "Casting, 시전 대기 중 (중앙 회전 후 좌측 대기)"),
	Discarding  UMETA(DisplayName = "Discarding, 버려지는 중 (빛 알갱이 산화)")
};

/** 3D 공간 상에 존재할 카드 액터 */
UCLASS()
class PROJECT_ENTROPY_API APE_CardActor : public AActor
{
	GENERATED_BODY()

public:
	APE_CardActor();

	virtual void Tick(float DeltaTime) override;

	/** 덱에서 카드를 생성하거나 가져올 때 DataAsset을 주입하여 카드 세팅 */
	UFUNCTION(BlueprintCallable, Category = "Card|Logic")
	void InitializeCard(UPE_CardData* InCardData);

	/** 마우스 호버링 및 특수 조건(비용 감소 등)에 따른 외곽선 하이라이트 제어 */
	UFUNCTION(BlueprintCallable, Category = "Card|Visual")
	void SetHighlightState(bool bIsHighlighted, FLinearColor OutlineColor = FLinearColor::White);

	/** --- 연출(Juicy) 이벤트: C++에서 상태 변경 시 호출하면 BP에서 애니메이션 실행 --- */

	// 1. 드로우: 빛 알갱이에서 카드로 나타나며 손패 위치로 날아오는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayDrawAnimation(FTransform TargetHandTransform);

	// 2. 사용(시전 대기): 중앙으로 날아가 찰지게 회전 후 좌측 중앙에 대기하는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayCastingReadyAnimation();

	// 3. 시전 취소: 스윽하고 원래 손패 위치로 자연스럽게 돌아가는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayCancelCastingAnimation(FTransform ReturnHandTransform);

	// 4. 카드 사용 완료 (산화): 빛 알갱이로 부서지며 무덤으로 날아가는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayDiscardAnimation(FVector DiscardPileWorldLocation);

	/** --- Getter --- */
	TObjectPtr<UPE_CardData> GetCardData() const { return CardData; }

	UFUNCTION(BlueprintCallable, Category = "Card|Movement")
	void MoveToTargetTransform(const FTransform& InTargetTransform);

protected:
	virtual void BeginPlay() override;

	/** 카드의 핵심 데이터 (이름, 스킬클래스, 비용 등) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Data")
	TObjectPtr<UPE_CardData> CardData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|State")
	EPECardVisualState CurrentState = EPECardVisualState::InDeck;

	// --- 컴포넌트 ---

	/** 카드의 3D 형태 (클릭 충돌체 역할 포함) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CardMesh;

	/** 카드 위에 띄울 텍스트, 아이콘 (3D 위젯) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> CardUIWidget;

	/** 마법진, 빛 알갱이 등을 재생할 범용 파티클 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> VFXComponent;

	// --- 머티리얼 ---
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

private:
	// --- 이동 보간용 변수 ---

	/** 현재 카드가 목표를 향해 이동 중인지 여부 */
	bool bIsMovingToTarget = false;

	/** 도달해야 할 최종 3D 좌표 및 회전값 */
	FTransform TargetTransform;

	/** 카드가 날아가는 속도 (값이 클수록 빠르고 딱딱하게, 작을수록 부드럽게 이동) */
	UPROPERTY(EditDefaultsOnly, Category = "Card|Movement")
	float MoveInterpSpeed = 10.f;
};