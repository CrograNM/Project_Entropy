// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardInstance.h"
#include "PE_CardActor.generated.h"

class UPE_CardThemeData;
class UStaticMeshComponent;
class UWidgetComponent;
class UNiagaraComponent;
class UBoxComponent;

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

	/** 덱에서 카드를 생성하거나 가져올 때 Data를 주입하여 카드 세팅 */
	UFUNCTION(BlueprintCallable, Category = "Card|Logic")
	void InitializeCard(UPE_CardInstance* InCardInstance);

	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Logic")
	void InitializeCardVisual(UPE_CardInstance* InCardInstance);

	/** 마우스 호버링 및 특수 조건(비용 감소 등)에 따른 외곽선 하이라이트 제어 */
	UFUNCTION(BlueprintCallable, Category = "Card|Visual")
	void SetHighlightState(bool bIsHighlighted, FLinearColor OutlineColor = FLinearColor::White);

	/** 카드가 위로 들썩이는 물리적 오프셋 적용 여부 */
	UFUNCTION(BlueprintCallable, Category = "Card|Movement")
	void SetHoverOffsetEnabled(bool bEnable);

	/** --- 연출(Juicy) 이벤트: C++에서 상태 변경 시 호출하면 BP에서 애니메이션 실행 --- */

	// 1. 드로우: 빛 알갱이에서 카드로 나타나며 손패 위치로 날아오는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayDrawAnimation(FTransform TargetHandTransform);

	// 2. 사용(시전 대기): 중앙으로 날아가 찰지게 회전 후 좌측 중앙에 대기하는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayCastingReadyAnimation();

	// 2-1. 즉발 카드 사용: 중앙으로 날아가 바로 산화되는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayInstantCastingAnimation();

	// 3. 시전 취소: 스윽하고 원래 손패 위치로 자연스럽게 돌아가는 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void PlayCancelCastingAnimation(FTransform ReturnHandTransform);

	// 4. 카드 사용 완료 (산화): 빛 알갱이로 부서지며 무덤으로 날아가는 연출
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Card|Animation")
	void PlayDiscardAnimation(FVector DiscardPileWorldLocation);
	
	// 캐스팅 대기 중 마우스 호버링 상태 변경 (오프셋 조절용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Card|Animation")
	void OnCastingHoverStateChanged(bool IsHovered);

	/** --- Getter/Setter --- */
	UFUNCTION(BlueprintCallable, Category = "Card|Logic")
	UPE_CardInstance* GetCardInstance() const { return CardInstance; }

	UFUNCTION(BlueprintCallable, Category = "Card|Logic")
	UPE_SkillData* GetSkillData() const { return CardInstance ? CardInstance->GetBaseCardData()->SkillDataToCast : nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Card|Logic")
	void SetDynamicMaterial(UMaterialInstanceDynamic* InMaterial) { DynamicMaterial = InMaterial; }

	UFUNCTION(BlueprintCallable, Category = "Card|Movement")
	void MoveToTargetTransform(const FTransform& InTargetRelativeTransform);

	UFUNCTION(BlueprintCallable, Category = "Card|Movement")
	void CancelMoveToTarget() { bIsMovingToTarget = false; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|Data")
	TObjectPtr<UPE_CardInstance> CardInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card|State")
	EPECardVisualState CurrentState = EPECardVisualState::InDeck;

	// --- 테마 데이터 ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Theme")
	TObjectPtr<UPE_CardThemeData> GlobalCardTheme;

	// --- 컴포넌트 ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> CardUIWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> VFXComponent;

	// --- 머티리얼 ---
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

private:
	// --- 이동 보간용 변수 ---
	bool bIsMovingToTarget = false;

	FTransform TargetRelativeTransform;

	UPROPERTY(EditDefaultsOnly, Category = "Card|Movement")
	float MoveInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Card|Movement")
	float TransformTolerance = 0.2f;

	// --- 카드 하이라이트 변수 ---
	bool bIsHovered = false;

	UPROPERTY(EditDefaultsOnly, Category = "Card|Hover")
	FVector HoverOffset = FVector(15.f, 0.f, 20.f);
};