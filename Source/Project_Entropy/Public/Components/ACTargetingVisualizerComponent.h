// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACTargetingVisualizerComponent.generated.h"

class AACTile;
class UPE_SkillData;
class USplineComponent;

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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // 화살표 렌더링용

	// --- [로컬 컨트롤러 호출용 인터페이스] ---
	void SetTargetingMode(ETargetingMode NewMode, int32 InRange, const UPE_SkillData* InSkillData = nullptr);
	void UpdateHoveredTile(FIntPoint NewPos);
	void ClearTargeting();
	bool IsTileInRange(AACTile* TargetTile) const;

	// 로컬 예측(Local Prediction) 및 동기화를 위한 핵심 렌더링 함수
	void RefreshVisuals();

protected:
	virtual void BeginPlay() override;

	// --- [서버 상태 업데이트 RPC] ---
	UFUNCTION(Server, Reliable)
	void Server_SetTargetingState(ETargetingMode NewMode, int32 InRange, const UPE_SkillData* InSkillData);

	UFUNCTION(Server, Reliable)
	void Server_UpdateHoveredTile(FIntPoint NewPos);

	// --- [네트워크 동기화 상태 변수] ---
	UFUNCTION() void OnRep_TargetingState();
	UFUNCTION() void OnRep_HoveredTile();

	UPROPERTY(ReplicatedUsing = OnRep_TargetingState)
	ETargetingMode RepTargetingMode = ETargetingMode::None;

	UPROPERTY(ReplicatedUsing = OnRep_TargetingState)
	int32 RepRange = 0;

	// 시각화를 동기화할 스킬 데이터 캐싱
	UPROPERTY(ReplicatedUsing = OnRep_TargetingState)
	TObjectPtr<const UPE_SkillData> RepSkillData;

	UPROPERTY(ReplicatedUsing = OnRep_HoveredTile)
	FIntPoint RepHoveredTile = FIntPoint(-999, -999);

	// --- [궤적 및 밀치기 시각화용 스플라인] ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	TObjectPtr<USplineComponent> TrajectorySpline; // 스킬이 날아가는 선

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	TObjectPtr<USplineComponent> PushSpline; // 밀치기로 날아갈 선

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	FColor TrajectoryColor = FColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	float TrajectoryArrowSize = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	float TrajectoryThickness = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	FColor PushColor = FColor::Orange;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	float PushArrowSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	float PushArrowExtension = 30.f; // 밀쳐짐 화살표가 타일 중앙보다 살짝 넘어가도록 연장

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer | Spline")
	float PushThickness = 8.f;

private:
	// 현재 시각화에 사용 중인 유효 타일 목록 (내부 보관용)
	UPROPERTY()
	TArray<AACTile*> CurrentValidTiles;
};