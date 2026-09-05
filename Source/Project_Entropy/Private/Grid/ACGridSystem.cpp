// Copyright CrograNM

#include "Grid/ACGridSystem.h"
#include "Containers/Queue.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"

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

APE_CharacterBase* AACGridSystem::GetCharacterAtPosition(FIntPoint Pos, AActor* IgnoreActor) const
{
	APE_CharacterBase* Occupant = OccupancyMap.FindRef(Pos);
	return (Occupant && Occupant != IgnoreActor) ? Occupant : nullptr;
}

bool AACGridSystem::IsTileOccupied(FIntPoint Pos, AActor* IgnoreActor) const
{
	// OccupancyMap으로 해당 좌표에 캐릭터가 존재하는지 확인
	const APE_CharacterBase* Occupant = OccupancyMap.FindRef(Pos);
	return Occupant != nullptr && Occupant != IgnoreActor;
}

void AACGridSystem::UpdateOccupancy(APE_CharacterBase* Char, FIntPoint OldPos, FIntPoint NewPos)
{
	if (!Char) return;
	if (IsTileOccupied(NewPos, Char))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Occupancy] %s 가 이미 점유된 곳으로 이동을 시도했습니다."), *GetNameSafe(Char));
	}

	// 이전 위치에서 제거
	if (OccupancyMap.Contains(OldPos) && OccupancyMap[OldPos] == Char)
	{
		OccupancyMap.Remove(OldPos);
	}

	// 새 위치에 추가
	OccupancyMap.Add(NewPos, Char);
	
	return;
}

TArray<AACTile*> AACGridSystem::CalculatePath(AActor* Requester, FIntPoint StartPos, FIntPoint EndPos)
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
				// 목표 지점이 아닌 중간 경로에 누군가(적, 다른 플레이어, 장애물) 서 있다면 지나갈 수 없음
				if (NextPos != EndPos && IsTileOccupied(NextPos, Requester))
				{
					continue;
				}

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

TArray<AACTile*> AACGridSystem::HighlightArea(AActor* Requester, FIntPoint StartPos, int32 Range, bool bIsMovement, const UPE_SkillData* SkillData)
{
	ClearRangeFor(Requester);

	TArray<AACTile*>& ValidTiles = PlayerRangeTiles.FindOrAdd(Requester);
	TQueue<TPair<FIntPoint, int32>> Queue; // <좌표, 거리>
	TMap<FIntPoint, int32> Visited;

	Queue.Enqueue(TPair<FIntPoint, int32>(StartPos, 0));
	Visited.Add(StartPos, 0);

	FIntPoint Directions[4] = { FIntPoint(1,0), FIntPoint(-1,0), FIntPoint(0,1), FIntPoint(0,-1) };

	while (!Queue.IsEmpty())
	{
		TPair<FIntPoint, int32> Current;
		Queue.Dequeue(Current);

		if (AACTile* Tile = GetTileAtPosition(Current.Key))
		{
			ValidTiles.Add(Tile);
			Tile->RequestHighlight(Requester, ETileHighlightType::InRange);
		}

		if (Current.Value >= Range) continue; // 최대 사거리 도달 시 더 이상 뻗어나가지 않음

		for (const FIntPoint& Dir : Directions)
		{
			FIntPoint NextPos = Current.Key + Dir;
			if (!Visited.Contains(NextPos))
			{
				AACTile* NextTile = GetTileAtPosition(NextPos);
				bool bCanPass = false;

				if (bIsMovement)
				{
					// 이동: 맵에 없는 공간(낙사), 비파괴 장애물, 타일 점유(유닛) 모두 통과 불가
					if (NextTile && !NextTile->IsObstacle() && !IsTileOccupied(NextPos, Requester))
					{
						bCanPass = true;
					}
				}
				else
				{
					// 스킬: 맵에 없는 공간(낙사)이나 비파괴 장애물(지진, 용암)은 무조건 범위가 통과함
					if (SkillData && SkillData->HitPhases[0].ProjectileSpeed > 0.f && SkillData->HitPhases[0].ProjectileGravity == 0.f)
					{
						// 직사 스킬: 타일 점유(캐릭터/동적장애물)를 통과할 수 없음
						if (!IsTileOccupied(NextPos, Requester))
						{
							bCanPass = true;
						}
					}
					else
					{
						// 곡사 스킬: 모든 장애물과 유닛 점유를 통과 가능
						bCanPass = true;
					}
				}

				if (bCanPass)
				{
					Visited.Add(NextPos, Current.Value + 1);
					Queue.Enqueue(TPair<FIntPoint, int32>(NextPos, Current.Value + 1));
				}
			}
		}
	}
	return ValidTiles;
}

void AACGridSystem::HighlightPath(AActor* Requester, FIntPoint StartPos, FIntPoint EndPos, const TArray<AACTile*>& InRangeTiles)
{
	ClearPathFor(Requester);

	AACTile* TargetTile = GetTileAtPosition(EndPos);
	if (!TargetTile || !InRangeTiles.Contains(TargetTile)) return;

	TArray<AACTile*> NewPath = CalculatePath(Requester, StartPos, EndPos);
	TArray<AACTile*>& PathArray = PlayerPathTiles.FindOrAdd(Requester);

	for (AACTile* Tile : NewPath)
	{
		ETileHighlightType Type = (Tile == TargetTile) ? ETileHighlightType::Hovered : ETileHighlightType::Path;
		Tile->RequestHighlight(Requester, Type);
		PathArray.Add(Tile);
	}
}

void AACGridSystem::HighlightTarget(AActor* Requester, FIntPoint TargetPos)
{
	ClearPathFor(Requester);

	if (AACTile* Tile = GetTileAtPosition(TargetPos))
	{
		Tile->RequestHighlight(Requester, ETileHighlightType::SkillTarget);
		PlayerPathTiles.FindOrAdd(Requester).Add(Tile);
	}
}

void AACGridSystem::HighlightAoE(AActor* Requester, const TSet<FIntPoint>& AoEPositions)
{
	ClearPathFor(Requester); // 기존 타겟 시각화 지우기

	TArray<AACTile*>& PathArray = PlayerPathTiles.FindOrAdd(Requester);

	for (const FIntPoint& Pos : AoEPositions)
	{
		if (AACTile* Tile = GetTileAtPosition(Pos))
		{
			Tile->RequestHighlight(Requester, ETileHighlightType::SkillTarget);
			PathArray.Add(Tile);
		}
	}
}

void AACGridSystem::ClearPathFor(AActor* Requester)
{
	if (TArray<AACTile*>* PathArray = PlayerPathTiles.Find(Requester))
	{
		TArray<AACTile*>* RangeArray = PlayerRangeTiles.Find(Requester);
		for (AACTile* Tile : *PathArray)
		{
			if (Tile)
			{
				if (RangeArray && RangeArray->Contains(Tile))
				{
					Tile->RequestHighlight(Requester, ETileHighlightType::InRange);
				}
				else
				{
					Tile->RequestHighlight(Requester, ETileHighlightType::None);
				}
			}
		}
		PathArray->Empty();
	}
}

void AACGridSystem::ClearRangeFor(AActor* Requester)
{
	if (TArray<AACTile*>* RangeArray = PlayerRangeTiles.Find(Requester))
	{
		for (AACTile* Tile : *RangeArray)
		{
			if (Tile) Tile->RequestHighlight(Requester, ETileHighlightType::None);
		}
		RangeArray->Empty();
	}
	ClearPathFor(Requester);
}

void AACGridSystem::ClearAllHighlightsFor(AActor* Requester)
{
	ClearRangeFor(Requester);
}

void AACGridSystem::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AACGridSystem::ValidateOccupancy, 1.f, true);
#endif
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

#if !UE_BUILD_SHIPPING
void AACGridSystem::ValidateOccupancy() const
{
	// 1) 지금 이 순간의 '진짜' 점유 상태를 캐릭터들에게서 직접 만들어봅니다.
	TMap<FIntPoint, APE_CharacterBase*> Expected;
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);
	for (AActor* Actor : AllChars)
	{
		APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor);
		if (!Char || (Char->GetStatComponent() && Char->GetStatComponent()->IsDead())) continue;
		if (UACGridMovementComponent* M = Char->GetGridMovementComponent())
		{
			Expected.Add(M->GetGridPosition(), Char);
			Expected.Add(M->GetTargetGridPosition(), Char);
		}
	}

	// 2) 현재 맵과 대조합니다.
	// 누락/불일치: 실제로는 캐릭터가 점유하고 있는데 OccupancyMap이 비어있거나 다른 캐릭터를 가리키고 있는 경우
	for (const auto& Pair : Expected)
		if (OccupancyMap.FindRef(Pair.Key) != Pair.Value)
			UE_LOG(LogTemp, Error, TEXT("[Occupancy] 누락/불일치 (%d,%d): 실제=%s, TMap=%s"), Pair.Key.X, Pair.Key.Y,
				*GetNameSafe(Pair.Value), *GetNameSafe(OccupancyMap.FindRef(Pair.Key)));

	// 유령 점유: 실제로는 아무도 없는 좌표에 OccupancyMap이 캐릭터를 가리키고 있는 경우
	for (const auto& Pair : OccupancyMap)
		if (!Expected.Contains(Pair.Key))
			UE_LOG(LogTemp, Error, TEXT("[Occupancy] 유령 점유 (%d,%d): %s"), Pair.Key.X, Pair.Key.Y, *GetNameSafe(Pair.Value));
}
#endif