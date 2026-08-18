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