// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACTargetingVisualizerComponent.generated.h"

class AACTile;

UENUM(BlueprintType)
enum class ETargetingMode : uint8
{
	None,
	Movement,
	Skill
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_ENTROPY_API UACTargetingVisualizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UACTargetingVisualizerComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- [로컬 컨트롤러 호출용 인터페이스] ---
	void SetTargetingMode(ETargetingMode NewMode, int32 InRange);
	void UpdateHoveredTile(FIntPoint NewPos);
	void ClearTargeting();

	// [추가됨] 컨트롤러가 클릭 시 사거리 유효성을 검사할 수 있도록 제공하는 헬퍼 함수
	bool IsTileInRange(AACTile* TargetTile) const;

	// 로컬 예측(Local Prediction) 및 동기화를 위한 핵심 렌더링 함수
	void RefreshVisuals();

protected:
	virtual void BeginPlay() override;

private:
	// --- [네트워크 동기화 상태 변수] ---
	UFUNCTION() void OnRep_TargetingState();
	UFUNCTION() void OnRep_HoveredTile();

	UPROPERTY(ReplicatedUsing = OnRep_TargetingState)
	ETargetingMode RepTargetingMode = ETargetingMode::None;

	UPROPERTY(ReplicatedUsing = OnRep_TargetingState)
	int32 RepRange = 0;

	UPROPERTY(ReplicatedUsing = OnRep_HoveredTile)
	FIntPoint RepHoveredTile = FIntPoint(-999, -999);

	// 현재 시각화에 사용 중인 유효 타일 목록 (내부 보관용)
	UPROPERTY()
	TArray<AACTile*> CurrentValidTiles;

	// --- [서버 상태 업데이트 RPC] ---
	UFUNCTION(Server, Reliable)
	void Server_SetTargetingState(ETargetingMode NewMode, int32 InRange);

	UFUNCTION(Server, Reliable)
	void Server_UpdateHoveredTile(FIntPoint NewPos);
};