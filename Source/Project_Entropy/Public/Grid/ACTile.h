// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACTile.generated.h"

UENUM(BlueprintType)
enum class ETileHighlightType : uint8
{
	None,
	InRange,    // 이동 사거리 내 타일 (은은한 불빛)
	Hovered,    // 마우스가 올라간 도착 타일 (강한 불빛)
	Path,       // 이동 경로 상의 타일들 (경로 불빛)
	SkillTarget // 스킬 사용 방향/목표 타일 (위험 불빛)
};

UCLASS()
class PROJECT_ENTROPY_API AACTile : public AActor
{
	GENERATED_BODY()
	
public:	
	AACTile();

protected:
	virtual void BeginPlay() override;

public:
	void SetGridPosition(FIntPoint InPos) { GridPosition = InPos; }
	FIntPoint GetGridPosition() const { return GridPosition; }
	FVector GetCenterWorldLocation() const;

	// 단일 상태 지정에서, 특정 액터(플레이어)의 요청을 추가/삭제하는 방식으로 변경
	void RequestHighlight(AActor* Requester, ETileHighlightType Type);

	void SetHighlightState(ETileHighlightType NewState);

	// 장애물 시스템 관련 함수
	UFUNCTION(BlueprintCallable, Category = "Tile|Obstacle")
	bool IsObstacle() const { return bIsObstacle; }

	UFUNCTION(BlueprintCallable, Category = "Tile|Obstacle")
	void SetObstacle(bool bInObstacle);
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UStaticMeshComponent> TileMesh;

	// 장애물용 보조 메쉬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Obstacle")
	TObjectPtr<UStaticMeshComponent> ObstacleMesh;

	// 장애물 상태 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile|State")
	bool bIsObstacle = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Data")
	FIntPoint GridPosition;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tile|Visual")
	FName EmissiveParamName = TEXT("EmissiveColor");

	UPROPERTY(EditDefaultsOnly, Category = "Tile|Visual")
	FName InsideOpacityParamName = TEXT("InsideOpacity");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	float InsideOpacityValue = 0.15f;

	// --- [로컬 플레이어 전용 색상] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor DefaultColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor InRangeColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor HoveredColor = FLinearColor(0.0f, 0.5f, 0.8f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor PathColor = FLinearColor(0.0f, 1.0f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor SkillTargetColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);

	// --- [다른 플레이어 시각화 전용 색상] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor OtherInRangeColor = FLinearColor(0.2f, 0.8f, 0.2f, 0.5f); // 흐릿한 연두색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor OtherHoveredColor = FLinearColor(0.0f, 0.4f, 0.0f, 1.0f); // 진한 녹색
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor OtherPathColor = FLinearColor(0.1f, 0.6f, 0.1f, 0.8f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor OtherSkillTargetColor = FLinearColor(0.0f, 0.4f, 0.0f, 1.0f); // 진한 녹색

private:
	// 타일 색상을 재계산하여 메터리얼에 적용하는 내부 함수
	void UpdateVisuals();

	// 어떤 액터(플레이어)가 나에게 무슨 색깔을 켜달라고 요청했는지 기억하는 맵
	TMap<AActor*, ETileHighlightType> HighlightRequests;
};