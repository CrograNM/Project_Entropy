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

	/** 마우스 왼쪽 버튼 클릭 시 호출 (잡기) */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void GrabCard();

	/** 마우스 왼쪽 버튼 뗐을 때 호출 (놓기/시전) */
	UFUNCTION(BlueprintCallable, Category = "Interaction|Card")
	void ReleaseCard();

	// 상호작용 가능 여부를 설정 (이동 모드 시 차단용)
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	bool IsInteractionEnabled() const { return bIsInteractionEnabled; }

protected:
	virtual void BeginPlay() override;

private:
	// 상호작용 가능 여부 (이동 모드 등에서 카드 상호작용을 차단할 때 false로 설정)
	bool bIsInteractionEnabled = true;

	/** 마우스 호버링 감지 처리 */
	void ProcessHovering();

	/** 드래그 중인 카드의 3D 위치 업데이트 */
	void ProcessDragging();

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> GrabbedCard;

	UPROPERTY()
	TObjectPtr<APE_CardActor> HoveredCardDuringDrag;

	/** 드래그 시 카메라 렌즈로부터 카드가 떨어져 있을 거리 (깊이) */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragDepth = 150.f;

	/** 드래그 시 카드가 마우스를 따라가는 보간 속도 (찰진 느낌 조절) */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	float DragInterpSpeed = 15.f;

	// 시전 대기(타겟팅) 상태일 때 카드가 고정될 카메라 기준 상대 좌표 (좌측 중앙쯤)
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Settings")
	FTransform CastingReadyOffset = FTransform(FRotator(0.f, 15.f, 10.f), FVector(50.f, -20.f, -10.f));

};