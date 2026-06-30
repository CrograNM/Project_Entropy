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
};