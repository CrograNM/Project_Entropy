// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/PE_TargetRules.h"
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

	/**
	 * 이 조준 상태에 대응하는 타겟 판정 컨텍스트.
	 *
	 * 사거리 BFS가 여기서 한 번만 돌고, 컨트롤러는 매 프레임 Evaluate로 조회만 합니다.
	 * 칠해진 칸과 허용되는 칸이 같은 계산에서 나오므로 어긋날 수 없습니다.
	 */
	const FPETargetContext& GetTargetContext() const { return TargetContext; }

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

	// --- [메쉬 및 머티리얼 세팅] ---
	// 몸통용 메쉬 (원기둥 권장)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UStaticMesh> LineMesh; 

	// 화살촉 메쉬 (원뿔 권장)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UStaticMesh> ArrowHeadMesh; 

	// 중도 충돌 및 최종 착탄 지점을 보여줄 반투명 마커
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UStaticMesh> ImpactSphereMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UMaterialInterface> TrajectoryMaterial; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UMaterialInterface> PushMaterial; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Assets")
	TObjectPtr<UMaterialInterface> ImpactSphereMaterial;

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

	// 구체 메쉬의 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visualizer|Settings")
	FVector ImpactSphereScale = FVector(0.5f);

private:
	// CurrentValidTiles와 같은 계산에서 나온 판정용 컨텍스트 (RefreshVisuals에서 함께 생성)
	FPETargetContext TargetContext;

	// 현재 시각화에 사용 중인 유효 타일 목록 (내부 보관용)
	UPROPERTY()
	TArray<AACTile*> CurrentValidTiles;

	UPROPERTY()
	TArray<UMeshComponent*> GeneratedMeshes;

	void ClearGeneratedMeshes();
	void GenerateMeshesAlongSpline(USplineComponent* Spline, UMaterialInterface* Mat, float Thickness, float HeadSize);
};