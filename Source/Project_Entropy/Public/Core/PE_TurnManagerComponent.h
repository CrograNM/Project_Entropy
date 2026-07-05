// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PE_TurnManagerComponent.generated.h"

UENUM(BlueprintType)
enum class EPEBattlePhase : uint8
{
	None,
	BattleStart,     // 전투 초기화 (덱 셔플, 초기 카드 드로우 등)
	PlayerTurn,      // 플레이어 행동 페이즈
	EnemyTurn,       // 적 AI 행동 페이즈
	EnvironmentTurn, // 환경 페이즈 (원소 타일 지속시간 감소, 화상 데미지 등)
	BattleEnd        // 전투 종료 (승/패)
};

class APE_EnemyBase;

// 턴 변경 이벤트 방송용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, EPEBattlePhase, NewPhase);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_ENTROPY_API UPE_TurnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPE_TurnManagerComponent();

	/** 페이즈가 바뀔 때마다 발동하는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Turn System")
	FOnPhaseChangedSignature OnPhaseChanged;

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void StartBattle();

	UFUNCTION(BlueprintCallable, Category = "Turn System")
	void EndCurrentPhase();

	/** 현재 페이즈 반환 */
	FORCEINLINE EPEBattlePhase GetCurrentPhase() const { return CurrentPhase; }
	FORCEINLINE int32 GetCurrentTurnCount() const { return CurrentTurnCount; }

protected:
	virtual void BeginPlay() override;

private:
	/** 실제 페이즈 변경 및 델리게이트 방송을 처리하는 내부 함수 */
	void ChangePhase(EPEBattlePhase NewPhase);
	
	void StartEnemyPhase();
	
	UFUNCTION()
	void ProcessNextEnemy();
	
	UFUNCTION()
	void TriggerNextEnemyWithDelay();

	UPROPERTY()
	TArray<APE_EnemyBase*> EnemyQueue;
	
	UPROPERTY(VisibleAnywhere, Category = "Turn System")
	EPEBattlePhase CurrentPhase;

	UPROPERTY(VisibleAnywhere, Category = "Turn System")
	int32 CurrentTurnCount;
};