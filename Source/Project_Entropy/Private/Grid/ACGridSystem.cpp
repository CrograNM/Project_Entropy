// Copyright CrograNM

#include "Grid/ACGridSystem.h"

AACGridSystem::AACGridSystem()
{
	PrimaryActorTick.bCanEverTick = false;
	
	TileSpacing = 100.f;
	MaxWidth = 5;
	MaxHeight = 5;
	GridShape = EGridShape::Rectangle;
}

AACTile* AACGridSystem::GetTileAtPosition(FIntPoint Pos) const
{
	if (GridTiles.Contains(Pos)) return GridTiles[Pos];
	return nullptr;
}

TArray<AACTile*> AACGridSystem::ShowMovementRange(FIntPoint CenterPos, int32 Range)
{
	ClearAllHighlights();
	CurrentRangeTiles.Empty();

	for (auto& Pair : GridTiles)
	{
		FIntPoint TilePos = Pair.Key;
		// 맨해튼 거리 공식 (|x1 - x2| + |y1 - y2|)으로 사각형/마름모 형태 범위 추출
		int32 Distance = FMath::Abs(CenterPos.X - TilePos.X) + FMath::Abs(CenterPos.Y - TilePos.Y);
		
		if (Distance <= Range && Distance > 0) // 자기 자신 제외
		{
			Pair.Value->SetHighlightState(ETileHighlightType::InRange);
			CurrentRangeTiles.Add(Pair.Value);
		}
	}
	return CurrentRangeTiles;
}

TArray<AACTile*> AACGridSystem::CalculatePath(FIntPoint StartPos, FIntPoint EndPos)
{
	TArray<AACTile*> Path;
	// 프로토타입용 단순 직선 축 이동 알고리즘 (추후 장애물 인식을 위해 A* 알고리즘으로 확장 가능)
	FIntPoint Current = StartPos;
	
	while (Current != EndPos)
	{
		if (Current.X != EndPos.X)
		{
			Current.X += (EndPos.X > Current.X) ? 1 : -1;
		}
		else if (Current.Y != EndPos.Y)
		{
			Current.Y += (EndPos.Y > Current.Y) ? 1 : -1;
		}
		
		AACTile* NextTile = GetTileAtPosition(Current);
		if (NextTile) Path.Add(NextTile);
	}
	return Path;
}

void AACGridSystem::HighlightPath(FIntPoint StartPos, FIntPoint EndPos, const TArray<AACTile*>& InRangeTiles)
{
	// 기존 경로 초기화 (범위 내 색상인 InRange로 원상복구)
	for (AACTile* Tile : CurrentPathTiles)
	{
		if (InRangeTiles.Contains(Tile))
		{
			Tile->SetHighlightState(ETileHighlightType::InRange);
		}
	}
	CurrentPathTiles.Empty();

	AACTile* TargetTile = GetTileAtPosition(EndPos);
	if (!TargetTile || !InRangeTiles.Contains(TargetTile)) return;

	// 새 경로 연산 및 하이라이트
	CurrentPathTiles = CalculatePath(StartPos, EndPos);
	
	for (AACTile* Tile : CurrentPathTiles)
	{
		if (Tile == TargetTile)
		{
			Tile->SetHighlightState(ETileHighlightType::Hovered); // 목적지
		}
		else
		{
			Tile->SetHighlightState(ETileHighlightType::Path); // 이동 경로
		}
	}
}

void AACGridSystem::ClearAllHighlights()
{
	for (auto& Pair : GridTiles)
	{
		Pair.Value->SetHighlightState(ETileHighlightType::None);
	}
	CurrentRangeTiles.Empty();
	CurrentPathTiles.Empty();
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

			// 도형 규칙에 따른 스폰 여부 필터링 (원형, 다이아몬드 등)
			if (GridShape == EGridShape::Rectangle)
			{
				bShouldSpawn = true;
			}
			else if (GridShape == EGridShape::Diamond)
			{
				// 맨해튼 거리 공식(|X| + |Y| <= 범위)으로 다이아몬드 구현
				bShouldSpawn = (FMath::Abs(X) + FMath::Abs(Y)) <= MaxWidth;
			}
			else if (GridShape == EGridShape::Circle)
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

				if (AACTile* NewTile = GetWorld()->SpawnActor<AACTile>(TileClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams))
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
	for (const auto& Pair : GridTiles)
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
	// 편의성: 에디터 인펙터 수치가 바뀌면 자동으로 그리드가 실시간 재시각화됨
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AACGridSystem, GridShape) || 
		PropertyName == GET_MEMBER_NAME_CHECKED(AACGridSystem, MaxWidth))
	{
		RegenerateGrid();
	}
}
#endif