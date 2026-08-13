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

	/** 주어진 타일 경로를 따라 순차 이동을 시작 (서버에서 호출 시 모든 클라이언트로 전송) */
	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = true, float Delay = 0.f);

	void MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = true, float Delay = 0.f);

	/** --- 그리드 데이터 Getter / Setter --- */
	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }

	// 멀티플레이어: 다른 액터가 이 컴포넌트의 최종 목적지를 확인할 수 있도록 Getter 추가
	FIntPoint GetTargetGridPosition() const { return TargetGridPosition; }

	// 스킬 이펙트 연산을 위해 현재 스피드를 반환하는 Getter
	float GetGridMoveSpeed() const { return GridMoveSpeed; }

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
	void ProcessNextCommand();
	void StartMoving();
	void SetNextPathStep();

	// 이동 명령 대기열
	UPROPERTY()
	TArray<FGridMoveCommand> MoveCommandQueue;

	bool bIsWaitingDelay = false;
	float DelayTimer = 0.f;

	bool bIsMovingOnGrid = false;
	bool bShouldRotate = true;

	int32 CurrentPathIndex = 0;
	FVector TargetWorldLocation;

	UPROPERTY()
	TArray<AACTile*> SavedPath;
};