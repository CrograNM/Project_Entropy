// Copyright CrograNM

#include "CardSystem/PE_SkillData.h"

TSet<FIntPoint> UPE_SkillData::GetAffectedGridPositions(FIntPoint CasterPos, FIntPoint TargetPos) const
{
	TSet<FIntPoint> Result;

	if (AoEShape == EPEAoEShape::None)
	{
		Result.Add(TargetPos);
		return Result;
	}

	// 방향 벡터 계산 (시전자 -> 타겟)
	FVector2D CasterV(CasterPos.X, CasterPos.Y);
	FVector2D TargetV(TargetPos.X, TargetPos.Y);
	FVector2D Dir = (TargetV - CasterV).GetSafeNormal();

	if (Dir.IsNearlyZero())
	{
		Dir = FVector2D(1, 0); // 제자리 클릭 시 기본 방향 보호
	}

	switch (AoEShape)
	{
	case EPEAoEShape::Line:
	{
		FVector2D StartP = CasterV;
		FVector2D EndP = CasterV + Dir * BaseRange;

		int32 MinX = CasterPos.X - BaseRange - FMath::CeilToInt(LineWidth);
		int32 MaxX = CasterPos.X + BaseRange + FMath::CeilToInt(LineWidth);
		int32 MinY = CasterPos.Y - BaseRange - FMath::CeilToInt(LineWidth);
		int32 MaxY = CasterPos.Y + BaseRange + FMath::CeilToInt(LineWidth);

		for (int32 x = MinX; x <= MaxX; ++x)
		{
			for (int32 y = MinY; y <= MaxY; ++y)
			{
				FVector2D Point(x, y);

				float t = FVector2D::DotProduct(Point - StartP, Dir);
				float Dist = 0.f;

				if (t < 0.f) Dist = FVector2D::Distance(Point, StartP);
				else if (t > BaseRange) Dist = FVector2D::Distance(Point, EndP);
				else
				{
					FVector2D Projection = StartP + Dir * t;
					Dist = FVector2D::Distance(Point, Projection);
				}

				if (Dist <= LineWidth + 0.1f)
				{
					Result.Add(FIntPoint(x, y));
				}
			}
		}
		break;
	}

	case EPEAoEShape::Custom:
	{
		float Angle = FMath::Atan2(Dir.Y, Dir.X);
		int32 DirIdx = FMath::RoundToInt(Angle / (PI / 2.f)); // -2, -1, 0, 1, 2

		for (const FIntPoint& Offset : CustomAoEOffsets)
		{
			FIntPoint RotatedOffset = Offset;

			if (bRotateToTarget)
			{
				if (DirIdx == 1)      RotatedOffset = FIntPoint(-Offset.Y, Offset.X); // Up
				else if (DirIdx == 2 || DirIdx == -2) RotatedOffset = FIntPoint(-Offset.X, -Offset.Y); // Left
				else if (DirIdx == -1)RotatedOffset = FIntPoint(Offset.Y, -Offset.X); // Down
			}

			// 마우스가 위치한 타겟을 중심으로 그려지도록 TargetPos 사용
			Result.Add(TargetPos + RotatedOffset);
		}
		break;
	}

	case EPEAoEShape::Cross:
	{
		for (int32 i = -AoESize; i <= AoESize; ++i)
		{
			Result.Add(TargetPos + FIntPoint(i, 0));
			Result.Add(TargetPos + FIntPoint(0, i));
		}
		break;
	}

	case EPEAoEShape::Square:
	{
		for (int32 x = -AoESize; x <= AoESize; ++x)
		{
			for (int32 y = -AoESize; y <= AoESize; ++y)
			{
				Result.Add(TargetPos + FIntPoint(x, y));
			}
		}
		break;
	}

	case EPEAoEShape::Ring:
	{
		for (int32 x = -AoESize; x <= AoESize; ++x)
		{
			for (int32 y = -AoESize; y <= AoESize; ++y)
			{
				if (FMath::Abs(x) == AoESize || FMath::Abs(y) == AoESize)
					Result.Add(TargetPos + FIntPoint(x, y));
			}
		}
		break;
	}

	}

	return Result;
}

void UPE_SkillData::GetAoEBounds(FIntPoint CasterPos, FIntPoint TargetPos, FVector2D& OutSize, float& OutRadius) const
{
	TSet<FIntPoint> AffectedTiles = GetAffectedGridPositions(CasterPos, TargetPos);

	if (AffectedTiles.IsEmpty())
	{
		// 대상이 없거나 단일 타겟일 경우 기본 타일 1칸 크기로 반환
		OutSize = FVector2D(100.f, 100.f);
		OutRadius = 50.f;
		return;
	}

	int32 MinX = 999999, MaxX = -999999;
	int32 MinY = 999999, MaxY = -999999;

	for (const FIntPoint& Pos : AffectedTiles)
	{
		MinX = FMath::Min(MinX, Pos.X);
		MaxX = FMath::Max(MaxX, Pos.X);
		MinY = FMath::Min(MinY, Pos.Y);
		MaxY = FMath::Max(MaxY, Pos.Y);
	}

	// 타일 1개가 100x100 유닛이므로 (Max - Min + 1)을 통해 실제 월드 유닛 크기를 산출합니다.
	OutSize.X = (MaxX - MinX + 1) * 100.f;
	OutSize.Y = (MaxY - MinY + 1) * 100.f;

	// 구체 형태(Ring, Circle 등)의 이펙트를 위한 반지름(가장 긴 축의 절반) 산출
	OutRadius = FMath::Max(OutSize.X, OutSize.Y) * 0.5f;
}