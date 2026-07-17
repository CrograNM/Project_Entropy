// Copyright CrograNM

#include "Grid/ACGridSystem.h"
#include "Containers/Queue.h"

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

	// 타일을 밟으며 퍼져나가는 BFS (너비 우선 탐색) 로직 적용
	TQueue<TPair<FIntPoint, int32>> Queue;
	TSet<FIntPoint> Visited;

	Queue.Enqueue(TPair<FIntPoint, int32>(CenterPos, 0));
	Visited.Add(CenterPos);

	FIntPoint Directions[4] = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };

	while (!Queue.IsEmpty())
	{
		TPair<FIntPoint, int32> CurrentNode;
		Queue.Dequeue(CurrentNode);

		FIntPoint CurrentPos = CurrentNode.Key;
		int32 CurrentCost = CurrentNode.Value;

		// 타일을 찾아서 하이라이트 (시작점 자신은 시각화에서 뺄 수 있지만 경로 탐색을 위해 맵에는 포함)
		if (AACTile* Tile = GetTileAtPosition(CurrentPos))
		{
			if (CurrentCost > 0) // 시작점(나 자신) 하이라이트 제외
			{
				Tile->SetHighlightState(ETileHighlightType::InRange);
				CurrentRangeTiles.Add(Tile);
			}
		}

		// 이동력(Range) 한계에 도달하면 더 이상 이 길로는 뻗어나가지 않음
		if (CurrentCost >= Range) continue;

		for (const FIntPoint& Dir : Directions)
		{
			FIntPoint NeighborPos = CurrentPos + Dir;

			if (!Visited.Contains(NeighborPos))
			{
				AACTile* NeighborTile = GetTileAtPosition(NeighborPos);
				
				// 이웃 타일이 존재하고, '장애물이 아닐 때만' 이동 가능 구역으로 취급
				if (NeighborTile && !NeighborTile->IsObstacle())
				{
					Visited.Add(NeighborPos);
					Queue.Enqueue(TPair<FIntPoint, int32>(NeighborPos, CurrentCost + 1));
				}
			}
		}
	}

	return CurrentRangeTiles;
}

TArray<AACTile*> AACGridSystem::CalculatePath(FIntPoint StartPos, FIntPoint EndPos)
{
	TArray<AACTile*> Path;
	
	// 장애물을 우회하는 BFS 기반 최단거리 길찾기 알고리즘
	if (StartPos == EndPos) return Path;

	TQueue<FIntPoint> Queue;
	TMap<FIntPoint, FIntPoint> CameFrom; // <현재 위치, 나를 발견한 이전 위치>

	Queue.Enqueue(StartPos);
	CameFrom.Add(StartPos, StartPos);

	FIntPoint Directions[4] = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };
	bool bFound = false;

	while (!Queue.IsEmpty())
	{
		FIntPoint Current;
		Queue.Dequeue(Current);

		if (Current == EndPos)
		{
			bFound = true;
			break;
		}

		for (const FIntPoint& Dir : Directions)
		{
			FIntPoint NextPos = Current + Dir;
			AACTile* NextTile = GetTileAtPosition(NextPos);

			// 다음 칸이 장애물이 아니고, 아직 방문하지 않은 칸이어야 함
			if (NextTile && !NextTile->IsObstacle() && !CameFrom.Contains(NextPos))
			{
				CameFrom.Add(NextPos, Current);
				Queue.Enqueue(NextPos);
			}
		}
	}

	// 경로를 찾았다면 EndPos부터 StartPos까지 거꾸로 추적하여 배열 생성
	if (bFound)
	{
		FIntPoint Curr = EndPos;
		while (Curr != StartPos)
		{
			Path.Insert(GetTileAtPosition(Curr), 0); // 배열의 맨 앞에 밀어넣어 순서를 뒤집음 (Start->End)
			Curr = CameFrom[Curr];
		}
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

void AACGridSystem::GetGridWorldBounds(FVector& OutMin, FVector& OutMax) const
{
	// 타일이 하나도 없다면 기본값 반환
	if (GridTiles.IsEmpty())
	{
		OutMin = FVector(-1000.f, -1000.f, 0.f);
		OutMax = FVector(1000.f, 1000.f, 0.f);
		return;
	}

	// 최대/최소값을 구하기 위해 초기값 설정
	OutMin = FVector(MAX_FLT, MAX_FLT, MAX_FLT);
	OutMax = FVector(-MAX_FLT, -MAX_FLT, -MAX_FLT);

	// 모든 타일을 순회하며 외곽선(Bounds) 한계점을 갱신
	for (const auto& Pair : GridTiles)
	{
		if (AACTile* Tile = Pair.Value)
		{
			FVector Loc = Tile->GetActorLocation();
			OutMin.X = FMath::Min(OutMin.X, Loc.X);
			OutMin.Y = FMath::Min(OutMin.Y, Loc.Y);
			OutMin.Z = FMath::Min(OutMin.Z, Loc.Z);

			OutMax.X = FMath::Max(OutMax.X, Loc.X);
			OutMax.Y = FMath::Max(OutMax.Y, Loc.Y);
			OutMax.Z = FMath::Max(OutMax.Z, Loc.Z);
		}
	}
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