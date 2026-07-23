// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACCardInteractionComponent.generated.h"

class APE_CardActor;
class APlayerController;

/** 상호작용 상태 정의 */
UENUM(BlueprintType)
enum class EPEInteractionState : uint8
{
	Hovering	UMETA(DisplayName = "Hovering"),
	Selecting	UMETA(DisplayName = "Selecting (Dragging)"),
	Waiting		UMETA(DisplayName = "Waiting (Animation/Input Lock)"),
	Casting		UMETA(DisplayName = "Casting (Targeting)"),
	Disabled	UMETA(DisplayName = "Disabled (Moving Mode)")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACCardInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACCardInteractionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void GrabCard();

	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void ReleaseCard();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void CancelCasting();

	/** BP에서 시전 대기 애니메이션이 끝났을 때 C++로 알려주는 콜백 */
	UFUNCTION(BlueprintCallable, Category = "Interaction|State")
	void OnCastingReadyFinished();

	// --- Getter ---
	EPEInteractionState GetCurrentState() const { return CurrentState; }
	APE_CardActor* GetCastingCard() const { return CastingCard; }

protected:
	virtual void BeginPlay() override;

private:
	/** 현재 상호작용의 상태 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction|State")
	EPEInteractionState CurrentState = EPEInteractionState::Hovering;

	void ProcessHovering();
	void ProcessDragging();
	void ProcessCasting();

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> GrabbedCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> CastingCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCardDuringDrag;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragDepth = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragInterpSpeed = 15.f;

	UPROPERTY()
	APlayerController* PC; // Tick에서 매번 GetOwner()->Cast<APlayerController>()를 호출하지 않도록 캐싱
};