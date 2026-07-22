// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACCardInteractionComponent.generated.h"

class APE_CardActor;

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

	bool IsInteractionEnabled() const { return bIsInteractionEnabled; }

protected:
	virtual void BeginPlay() override;

private:
	// 상호작용 가능 여부 
	bool bIsInteractionEnabled = true;

	void ProcessHovering();
	void ProcessDragging();

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> GrabbedCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCardDuringDrag;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragDepth = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragInterpSpeed = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	FTransform CastingReadyOffset = FTransform(FRotator(0.f, 15.f, 10.f), FVector(50.f, -20.f, -10.f));

};