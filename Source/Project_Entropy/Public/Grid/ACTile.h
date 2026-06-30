// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACTile.generated.h"

UCLASS()
class PROJECT_ENTROPY_API AACTile : public AActor
{
	GENERATED_BODY()
	
public:	
	AACTile();

protected:
	virtual void BeginPlay() override;

public:
	/** 타일의 그리드 좌표 (X, Y) */
	void SetGridPosition(FIntPoint InPos) { GridPosition = InPos; }
	FIntPoint GetGridPosition() const { return GridPosition; }

	/** 캐릭터가 이동할 타일 중심점의 월드 위치 반환 */
	FVector GetCenterWorldLocation() const;

protected:
	/** 마우스 클릭 레이캐스트를 감지할 타일 메쉬 (에디터에서 납작한 사각판 지정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UStaticMeshComponent> TileMesh;

	/** 이 타일의 고유 그리드 좌표 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Data")
	FIntPoint GridPosition;
};