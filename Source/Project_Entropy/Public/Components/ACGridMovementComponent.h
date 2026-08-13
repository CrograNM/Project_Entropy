// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardSystem/PE_SkillData.h" 
#include "ACGridMovementComponent.generated.h"

class AACTile;

// 이동 완료 시 터뜨릴 데미지 및 이펙트 정보 캡슐화
USTRUCT()
struct FGridKnockbackPayload
{
	GENERATED_BODY()

	UPROPERTY() bool bIsActive = false;
	UPROPERTY() TObjectPtr<AActor> Instigator = nullptr;
	UPROPERTY() TObjectPtr<AActor> HitCharacter = nullptr;
	UPROPERTY() float TargetDamage = 0.f;
	UPROPERTY() float OtherDamage = 0.f;
	UPROPERTY() TObjectPtr<const UPE_SkillData> SkillData = nullptr;
};

// 큐에 담아둘 단일 이동 명령 구조체
USTRUCT()
struct FGridMoveCommand
{
	GENERATED_BODY()

	UPROPERTY() TArray<AACTile*> Path;
	UPROPERTY() bool bRotate = false;
	UPROPERTY() float AbsoluteStartTime = 0.f;
	UPROPERTY() FGridKnockbackPayload Payload;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridMovementFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridKnockbackImpact);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACGridMovementComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, float Delay = 0.f, FGridKnockbackPayload Payload = FGridKnockbackPayload());
	void MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, float Delay = 0.f, FGridKnockbackPayload Payload = FGridKnockbackPayload());

	FIntPoint GetGridPosition() const { return GridPosition; }
	void SetGridPosition(FIntPoint NewPos) { GridPosition = NewPos; }
	FIntPoint GetTargetGridPosition() const { return TargetGridPosition; }
	float GetGridMoveSpeed() const { return GridMoveSpeed; }

	UPROPERTY(BlueprintAssignable)
	FOnGridMovementFinished OnMovementFinished;
	UPROPERTY(BlueprintAssignable, Category = "Movement|Events")
	FOnGridKnockbackImpact OnKnockbackImpact;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	FIntPoint GridPosition;

	UPROPERTY(Replicated)
	FIntPoint TargetGridPosition;

	UPROPERTY(EditAnywhere, Category = "Movement") float GridMoveSpeed = 1000.f;
	// 튕겨나가는 탄성 강도 조절용 변수
	UPROPERTY(EditAnywhere, Category = "Movement") float OvershootFactor = 3.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float RotationSpeed = 2000.f;

private:
	void ProcessNextCommand();
	void StartMoving();
	void SetNextPathStep();
	void ExecuteKnockbackPayload();

	UPROPERTY() TArray<FGridMoveCommand> MoveCommandQueue;

	bool bIsWaitingDelay = false;
	float DelayTimer = 0.f;

	bool bIsMovingOnGrid;
	bool bShouldRotate;
	bool bHasFiredPayload;

	int32 CurrentPathIndex;
	FVector TargetWorldLocation;

	// 시간 기반 보간을 위한 변수들
	FVector StepStartLocation;
	float StepDuration;
	float StepElapsedTime;

	UPROPERTY() TArray<AACTile*> SavedPath;
	UPROPERTY() FGridKnockbackPayload CurrentPayload;
};