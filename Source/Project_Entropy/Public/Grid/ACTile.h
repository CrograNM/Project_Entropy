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
	Path        // 이동 경로 상의 타일들 (경로 불빛)
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

	/** 타일의 하이라이트 상태를 변경하여 이미시브 컬러를 제어하는 함수 */
	void SetHighlightState(ETileHighlightType NewState);
	
	// 장애물 시스템 관련 함수
	UFUNCTION(BlueprintCallable, Category = "Tile|Obstacle")
	bool IsObstacle() const { return bIsObstacle; }

	UFUNCTION(BlueprintCallable, Category = "Tile|Obstacle")
	void SetObstacle(bool bInObstacle);
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UStaticMeshComponent> TileMesh;

	// 장애물용 보조 메쉬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Obstacle")
	TObjectPtr<UStaticMeshComponent> ObstacleMesh;

	// 장애물 상태 변수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|State")
	bool bIsObstacle = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Data")
	FIntPoint GridPosition;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
	
	UPROPERTY(EditDefaultsOnly, Category = "Tile|Visual")
	FName EmissiveParamName = TEXT("EmissiveColor");
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor DefaultColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor InRangeColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor HoveredColor = FLinearColor(0.0f, 0.5f, 0.8f, 1.0f); 
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	FLinearColor PathColor = FLinearColor(0.0f, 1.0f, 0.8f, 1.0f);
};