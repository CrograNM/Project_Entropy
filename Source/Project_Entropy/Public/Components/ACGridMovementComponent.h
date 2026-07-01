// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACGridMovementComponent.generated.h"

class AACTile;

// 이동이 완전히 끝났을 때 외부(턴 매니저 등)에 알리기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementFinishedSignature);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACGridMovementComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 외부 이벤트 바인딩용 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Grid Movement")
	FOnMovementFinishedSignature OnMovementFinished;

	/** --- 그리드 데이터 Getter / Setter --- */
	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }

	/** 주어진 타일 경로를 따라 순차 이동을 시작 */
	void MoveAlongPath(const TArray<AACTile*>& InPath);
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, Category = "Grid")
	FIntPoint GridPosition;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float GridMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float AcceptanceRadius;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float RotationSpeed;

private:
	void SetNextPathStep();

	UPROPERTY()
	TArray<AACTile*> SavedPath;
	
	int32 CurrentPathIndex = 0;
	bool bIsMovingOnGrid = false;
	FVector TargetWorldLocation;
};
