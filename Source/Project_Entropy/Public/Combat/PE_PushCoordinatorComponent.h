// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/PE_PushTypes.h"
#include "PE_PushCoordinatorComponent.generated.h"

class AACGridSystem;
class APE_CharacterBase;
class APE_GameState;

/** 지금 밀려나고 있는 대상 1명. 도착 보고를 받으면 여기 걸어둔 연쇄 요청이 투입됩니다. */
USTRUCT()
struct FPEPushInFlight
{
	GENERATED_BODY()

	UPROPERTY() APE_CharacterBase* Target = nullptr;

	// 이 밀치기가 점유한 UI 로그. 도착하면 지웁니다.
	UPROPERTY() int32 ActionLogID = -1;

	// 무언가를 들이받아 파생될 연쇄 요청. 이 대상이 실제로 멈출 때까지 대기합니다.
	UPROPERTY() bool bHasChained = false;
	UPROPERTY() FPEPushRequest Chained;
};

/**
 * 한 번의 EnqueuePush로 시작된 밀치기 묶음.
 *
 * 연쇄가 전부 끝날 때까지 액션 토큰 1개를 계속 쥐고 있습니다.
 * 그래서 "앞 캐릭터 도착 -> 다음 캐릭터 출발" 사이의 한 프레임 동안 액션 큐가 비어
 * 다음 스킬이 끼어드는 일이 없습니다. UI 로그는 밀치기 1건마다 따로 남습니다.
 */
USTRUCT()
struct FPEPushChain
{
	GENERATED_BODY()

	UPROPERTY() int32 ChainID = -1;
	UPROPERTY() int32 ActionTokenID = -1;

	// 아직 출발하지 않은 요청
	UPROPERTY() TArray<FPEPushRequest> Pending;

	// 지금 밀려나고 있는 대상들
	UPROPERTY() TArray<FPEPushInFlight> InFlight;
};

/**
 * 밀치기 실행 주체 (APE_GameState에 부착, 서버 전용).
 *
 * 밀치기를 스킬에서 떼어내는 자리입니다. 스킬 / 환경 / 함정 / AI 무엇이든
 * FPEPushRequest 목록만 만들어 EnqueuePush로 넣으면 되고, 연쇄와 타이밍과 액션 토큰은 여기가 책임집니다.
 *
 * 핵심은 연쇄를 '미리 계산해 예약'하지 않는다는 점입니다.
 * 한 세대를 출발시킨 뒤 실제 도착 보고(OnKnockbackSettled)를 기다렸다가 다음 세대를 풉니다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UPE_PushCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPE_PushCoordinatorComponent();

	/**
	 * 밀치기의 유일한 진입점.
	 * 한 번의 호출이 하나의 연쇄가 되고, 그 연쇄 전체가 액션 토큰 1개를 소유합니다.
	 */
	void EnqueuePush(TArray<FPEPushRequest> Requests, const FString& Context);

	/** 아직 정산되지 않은 연쇄가 남아 있는지 */
	bool IsResolving() const { return ActiveChains.Num() > 0; }

	AACGridSystem* GetGrid();

private:
	/** 이동 컴포넌트가 실제로 멈춘 순간 부르는 콜백. 연쇄는 오직 여기서만 시작됩니다. */
	UFUNCTION()
	void HandleKnockbackSettled(APE_CharacterBase* Mover, int32 ChainID);

	/**
	 * 출발할 요청이 남아있는 동안 세대를 반복 전개하고, 끝에 정산합니다.
	 * 경로가 빈 밀치기는 도착 보고가 동기적으로 되돌아오므로 재진입이 발생합니다.
	 * 재진입은 플래그로 흡수하고 바깥 루프가 이어서 처리합니다.
	 */
	void Drain();

	/** 지금 Pending에 있는 전부를 한 세대로 묶어 정렬 순서대로 출발시킵니다. */
	void DispatchGeneration(int32 ChainID);

	void ExecuteStep(int32 ChainID, const FPEPushRequest& Request, const FPEPushStep& Step);

	/** 대기도 비행도 없는 연쇄를 닫고 액션 토큰을 반납합니다. */
	void CloseSettledChains();

	APE_GameState* GetGameState() const;

	UPROPERTY()
	TMap<int32, FPEPushChain> ActiveChains;

	UPROPERTY()
	TObjectPtr<AACGridSystem> CachedGrid;

	int32 NextChainID = 0;

	bool bIsDraining = false;
	bool bDrainRequested = false;
};
