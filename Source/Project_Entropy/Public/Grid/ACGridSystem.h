// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/ACTile.h"
#include "ACGridSystem.generated.h"

UENUM (BlueprintType)
enum class EGridShape : uint8
{
	Rectangle UMETA(DisplayName = "Rectangle"),
	Diamond UMETA(DisplayName = "Diamond"),
	Circle UMETA(DisplayName = "Circle")
};

UCLASS()
class PROJECT_ENTROPY_API AACGridSystem : public AActor
{
	GENERATED_BODY()
	
public:	
	AACGridSystem();

	// 특정 좌표의 타일 반환
	AACTile* GetTileAtPosition(FIntPoint Pos) const;

	TArray<AACTile*> HighlightArea(AActor* Requester, FIntPoint CenterPos, int32 Range);
	void HighlightPath(AActor* Requester, FIntPoint StartPos, FIntPoint EndPos, const TArray<AACTile*>& InRangeTiles);
	void HighlightTarget(AActor* Requester, FIntPoint TargetPos);
	void HighlightAoE(AActor* Requester, const TSet<FIntPoint>& AoEPositions);

	void ClearAllHighlightsFor(AActor* Requester);
	void ClearPathFor(AActor* Requester);
	void ClearRangeFor(AActor* Requester);

	bool IsTileOccupied(FIntPoint Pos, AActor* IgnoreActor = nullptr) const;

	// 시작점-도착점 단순 그리드 최단 경로 반환
	TArray<AACTile*> CalculatePath(FIntPoint StartPos, FIntPoint EndPos);
	
protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** 에디터 버튼용: 설정된 구역 모양에 맞춰 타일을 자동 생성하는 툴 함수 */
	UFUNCTION(CallInEditor, Category = "Grid Tool")
	void RegenerateGrid();

	/** 전장의 모든 타일을 깔끔하게 지우는 함수 */
	UFUNCTION(CallInEditor, Category = "Grid Tool")
	void ClearGrid();
	
	UFUNCTION(BlueprintCallable, Category = "Grid Tool")
	void GetGridWorldBounds(FVector& OutMin, FVector& OutMax) const;
	
protected:
	/** 스폰할 타일 클래스 블루프린트 (BP_ACTile) */
	UPROPERTY(EditAnywhere, Category = "Grid Setup")
	TSubclassOf<AACTile> TileClass;

	/** 타일 간의 간격 (센티미터 단위, 기본 1m = 100.f) */
	UPROPERTY(EditAnywhere, Category = "Grid Setup")
	float TileSpacing;

	/** 툴에서 사용할 기본 사각형 최대 범위 */
	UPROPERTY(EditAnywhere, Category = "Grid Tool|Generator")
	int32 MaxWidth;
	
	UPROPERTY(EditAnywhere, Category = "Grid Tool|Generator")
	int32 MaxHeight;

	/** 다이아몬드, 원형 등 브러시 형태 선택 옵션 */
	UPROPERTY(EditAnywhere, Category = "Grid Tool|Generator")
	EGridShape GridShape;

	/** 실제 전장에 배치된 타일들의 2차원 데이터 맵 (Key: 좌표, Value: 타일 액터) */
	UPROPERTY(VisibleAnywhere, Category = "Grid Data")
	TMap<FIntPoint, AACTile*> GridTiles;
	
private:
	// 각 플레이어(Instigator)가 활성화한 타일 목록을 개별 추적합니다.
	TMap<AActor*, TArray<AACTile*>> PlayerRangeTiles;
	TMap<AActor*, TArray<AACTile*>> PlayerPathTiles;
};