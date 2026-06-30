// Copyright CrograNM

// #include "Grid/ACTile.h"
#include "Grid/ACGridSystem.h"

AACGridSystem::AACGridSystem()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TileSpacing = 100.f;
	MaxWidth = 5;
	MaxHeight = 5;
	GridShapeShape = TEXT("Rectangle");
}

void AACGridSystem::BeginPlay()
{
	Super::BeginPlay();
}

void AACGridSystem::RegenerateGrid()
{
	ClearGrid();

	if (!TileClass) return;

	FVector OriginLocation = GetActorLocation();

	for (int32 X = -MaxWidth; X <= MaxWidth; ++X)
	{
		for (int32 Y = -MaxHeight; Y <= MaxHeight; ++Y)
		{
			bool bShouldSpawn = false;

			// 개발자가 설정한 도형 규칙에 따라 스폰 여부 필터링 (원형, 다이아몬드 등)
			if (GridShapeShape == TEXT("Rectangle"))
			{
				bShouldSpawn = true;
			}
			else if (GridShapeShape == TEXT("Diamond"))
			{
				// 마인크래프트 등에서 쓰이는 맨해튼 거리 공식(|X| + |Y| <= 범위)으로 다이아몬드 구현
				bShouldSpawn = (FMath::Abs(X) + FMath::Abs(Y)) <= MaxWidth;
			}
			else if (GridShapeShape == TEXT("Circle"))
			{
				// 피타고라스 정리(정원형 공식)를 이용한 원형 범위 타일 추출
				bShouldSpawn = (X*X + Y*Y) <= (MaxWidth * MaxWidth);
			}

			if (bShouldSpawn)
			{
				FIntPoint GridPos(X, Y);
				
				// 월드 스폰 위치 계산 (GridSystem 액터의 중심점 기준 정렬)
				FVector SpawnLoc = OriginLocation + FVector(X * TileSpacing, Y * TileSpacing, 0.f);
				
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				
				AACTile* NewTile = GetWorld()->SpawnActor<AACTile>(TileClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
				if (NewTile)
				{
					NewTile->SetGridPosition(GridPos);
#if WITH_EDITOR
					NewTile->SetActorLabel(FString::Printf(TEXT("Tile_%d_%d"), X, Y)); // 에디터 계층 구조 정돈
#endif
					GridTiles.Add(GridPos, NewTile);
				}
			}
		}
	}
}

void AACGridSystem::ClearGrid()
{
	for (auto& Pair : GridTiles)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	GridTiles.Empty();
}

#if WITH_EDITOR
void AACGridSystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 디자이너 편의성: 에디터 인펙터 수치가 바뀌면 자동으로 그리드가 실시간 재시각화됨
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AACGridSystem, GridShapeShape) || 
		PropertyName == GET_MEMBER_NAME_CHECKED(AACGridSystem, MaxWidth))
	{
		RegenerateGrid();
	}
}
#endif