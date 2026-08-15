// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACTargetingVisualizerComponent.generated.h"

class AACTile;
class UPE_SkillData;
class USplineComponent;
class UStaticMesh;
class UMaterialInterface;
class UMeshComponent;

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
	void SetTargetingMode(ETargetingMode NewMode, int32 InRange, const UPE_SkillData* InSkillData = nullptr);
	void UpdateHoveredTile(FIntPoint NewPos);
	void ClearTargeting();
	bool IsTileInRange(AACTile* TargetTile) const;

	// 로컬 예측(Local Prediction) 및 동기화를 위한 핵심 렌더링 함수
	void RefreshVisuals();

	ETargetingMode GetTargetingMode() const { return RepTargetingMode; }
	FIntPoint GetHoveredTile() const { return RepHoveredTile; }

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualizer|Spline")
	TObjectPtr<USplineComponent> TrajectorySpline; // 스킬이 날아가는 선

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualizer|Spline")
	TObjectPtr<USplineComponent> PushSpline; // 밀치기로 날아갈 선

	// --- [추가됨: 메쉬 및 머티리얼 세팅] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UStaticMesh> LineMesh; // 몸통용 메쉬 (원기둥 권장)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UStaticMesh> ArrowHeadMesh; // 화살촉 메쉬 (원뿔 권장)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UMaterialInterface> TrajectoryMaterial; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UMaterialInterface> PushMaterial; 

	// 화살촉 뒤로 당기기 배율
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float ArrowPullbackMultiplier = 1.0f; 

	// 궤적 선 두께
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float TrajectoryThickness = 1.0f; 

	// 궤적 화살촉 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float TrajectoryArrowSize = 1.5f; 

	// 밀치기 선 두께
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float PushThickness = 1.0f; 

	// 밀치기 화살촉 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float PushArrowSize = 1.5f; 

	// 밀치기 화살촉을 연장할 거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	float PushArrowExtension = 25.f;

private:
	// 현재 시각화에 사용 중인 유효 타일 목록 (내부 보관용)
	UPROPERTY()
	TArray<AACTile*> CurrentValidTiles;

	UPROPERTY()
	TArray<UMeshComponent*> GeneratedMeshes;

	void ClearGeneratedMeshes();
	void GenerateMeshesAlongSpline(USplineComponent* Spline, UMaterialInterface* Mat, float Thickness, float HeadSize);
};