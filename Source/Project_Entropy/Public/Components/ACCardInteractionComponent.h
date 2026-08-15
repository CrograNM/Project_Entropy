// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACCardInteractionComponent.generated.h"

class APE_CardActor;
class APE_PlayerController;

/** 상호작용 상태 정의 */
UENUM(BlueprintType)
enum class EPEInteractionState : uint8
{
	Hovering	UMETA(DisplayName = "Hovering"),
	Selecting	UMETA(DisplayName = "Selecting (Dragging)"),
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

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionSuspended(bool bSuspend);

	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void CancelCasting();

	// --- Getter ---
	EPEInteractionState GetCurrentState() const { return CurrentState; }
	APE_CardActor* GetGrabbedCard() const { return GrabbedCard; }
	bool IsPreparingToCast() const { return bIsPreparingToCast; }

protected:
	virtual void BeginPlay() override;

private:
	/** 현재 상호작용의 상태 */
	UPROPERTY(VisibleAnywhere, Category = "Interaction|State")
	EPEInteractionState CurrentState = EPEInteractionState::Hovering;

	bool bIsSuspended = false;
	bool bIsPreparingToCast = false;

	void ProcessHovering();
	void ProcessDragging();

	UPROPERTY() TObjectPtr<APE_CardActor> HoveredCard;
	UPROPERTY() TObjectPtr<APE_CardActor> GrabbedCard;
	UPROPERTY() TObjectPtr<APE_CardActor> HoveredCardDuringDrag;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragDepth = 150.f;

	UPROPERTY()
	APE_PlayerController* PC; // Tick에서 매번 Cast<APE_PlayerController>()를 호출하지 않도록 캐싱
};