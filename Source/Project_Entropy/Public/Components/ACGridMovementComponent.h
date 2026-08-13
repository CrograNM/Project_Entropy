// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACGridMovementComponent.generated.h"

class AACTile;

// 큐에 담아둘 단일 이동 명령 구조체
USTRUCT()
struct FGridMoveCommand
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<AACTile*> Path;

	UPROPERTY()
	bool bRotate = false;

	// 명령이 수신된 시간 + Delay를 합산한 절대 실행 시간
	UPROPERTY()
	float AbsoluteStartTime = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridMovementFinished);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACGridMovementComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, float Delay = 0.f);

	void MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, float Delay = 0.f);

	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }
	FIntPoint GetTargetGridPosition() const { return TargetGridPosition; }
	float GetGridMoveSpeed() const { return GridMoveSpeed; }

	UPROPERTY(BlueprintAssignable)
	FOnGridMovementFinished OnMovementFinished;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	FIntPoint GridPosition;

	UPROPERTY(Replicated)
	FIntPoint TargetGridPosition;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float GridMoveSpeed;

	// [수정됨: 튕겨나가는 탄성 강도 조절용 변수 추가]
	UPROPERTY(EditAnywhere, Category = "Movement")
	float OvershootFactor = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationSpeed;

private:
	void ProcessNextCommand();
	void StartMoving();
	void SetNextPathStep();

	UPROPERTY()
	TArray<FGridMoveCommand> MoveCommandQueue;

	bool bIsWaitingDelay = false;
	float DelayTimer = 0.f;

	bool bIsMovingOnGrid;
	bool bShouldRotate;
	int32 CurrentPathIndex;
	FVector TargetWorldLocation;

	// 시간 기반 보간을 위한 변수들
	FVector StepStartLocation;
	float StepDuration;
	float StepElapsedTime;

	UPROPERTY()
	TArray<AACTile*> SavedPath;
};