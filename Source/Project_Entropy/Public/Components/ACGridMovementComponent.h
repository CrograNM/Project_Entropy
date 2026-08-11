// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACGridMovementComponent.generated.h"

class AACTile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMovementFinishedSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACGridMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Grid Movement")
	FOnMovementFinishedSignature OnMovementFinished;

	/** --- 그리드 데이터 Getter / Setter --- */
	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }

	// 멀티플레이어: 다른 액터가 이 컴포넌트의 최종 목적지를 확인할 수 있도록 Getter 추가
	FIntPoint GetTargetGridPosition() const { return TargetGridPosition; }

	/** 주어진 타일 경로를 따라 순차 이동을 시작 (서버에서 호출 시 모든 클라이언트로 전송) */
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = true);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Replicated, Category = "Grid")
	FIntPoint GridPosition;

	// 멀티플레이어: 선점 처리를 위해 현재 이동하려는 최종 타일의 좌표 보관
	UPROPERTY(VisibleAnywhere, Replicated, Category = "Grid")
	FIntPoint TargetGridPosition = FIntPoint(-999, -999);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float GridMoveSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float AcceptanceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Movement")
	float RotationSpeed;

private:
	void MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = true);
	void SetNextPathStep();

	UPROPERTY()
	TArray<AACTile*> SavedPath;

	bool bShouldRotate = true;

	int32 CurrentPathIndex = 0;
	bool bIsMovingOnGrid = false;
	FVector TargetWorldLocation;
};