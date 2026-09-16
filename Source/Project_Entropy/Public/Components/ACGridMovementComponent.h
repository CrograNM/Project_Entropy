// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CardSystem/PE_SkillData.h"
#include "ACGridMovementComponent.generated.h"

class AACTile;
class AACGridSystem;
class APE_CharacterBase;

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

	/**
	 * 이 밀치기를 지시한 연쇄의 ID (UPE_PushCoordinatorComponent가 발급).
	 * 도착 보고(OnKnockbackSettled)에 그대로 실어 돌려주므로 코디네이터가 역추적 맵을 들 필요가 없습니다.
	 *
	 * 액션 큐 토큰과 UI 로그는 더 이상 여기 담기지 않습니다. 연쇄 전체를 코디네이터가 소유합니다.
	 */
	UPROPERTY() int32 ChainID = -1;
};

// 큐에 담아둘 단일 이동 명령 구조체
USTRUCT()
struct FGridMoveCommand
{
	GENERATED_BODY()

	UPROPERTY() TArray<AACTile*> Path;
	UPROPERTY() bool bRotate = false;
	UPROPERTY() FGridKnockbackPayload Payload;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridMovementFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGridKnockbackImpact);

/**
 * 넉백 이동이 '실제로 멈춘 순간' 발사됩니다.
 *
 * 피해 유무와 무관하게 넉백 명령 1건당 정확히 한 번 나가며, 대상이 처리 도중 파괴되어도 보장됩니다.
 * 연쇄 밀치기는 예측된 지연이 아니라 오직 이 이벤트로만 출발합니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridKnockbackSettled, APE_CharacterBase*, Mover, int32, ChainID);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACGridMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACGridMovementComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SnapCharacterToNearestTile();

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticast_MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, FGridKnockbackPayload Payload = FGridKnockbackPayload());
	void MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate = false, FGridKnockbackPayload Payload = FGridKnockbackPayload());

	void SetGridPosition(FIntPoint NewPos);

	AACGridSystem* GetCachedGridSystem();
	FIntPoint GetGridPosition() const { return GridPosition; }
	float GetGridMoveSpeed() const { return GridMoveSpeed; }

	UPROPERTY(BlueprintAssignable)
	FOnGridMovementFinished OnMovementFinished;
	UPROPERTY(BlueprintAssignable, Category = "Movement|Events")
	FOnGridKnockbackImpact OnKnockbackImpact;
	UPROPERTY(BlueprintAssignable, Category = "Movement|Events")
	FOnGridKnockbackSettled OnKnockbackSettled;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AACGridSystem> CachedGridSystem;

	UPROPERTY(EditAnywhere, Category = "Movement") float GridMoveSpeed = 1000.f;
	// 튕겨나가는 탄성 강도 조절용 변수
	UPROPERTY(EditAnywhere, Category = "Movement") float OvershootFactor = 3.f;
	UPROPERTY(EditAnywhere, Category = "Movement") float RotationSpeed = 2000.f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_GridPosition)
	FIntPoint GridPosition;

	UFUNCTION()
	void OnRep_GridPosition(FIntPoint OldPos);

	void ProcessNextCommand();
	void StartMoving();
	void SetNextPathStep();
	void ExecuteKnockbackPayload();

	UPROPERTY() TArray<FGridMoveCommand> MoveCommandQueue;

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
