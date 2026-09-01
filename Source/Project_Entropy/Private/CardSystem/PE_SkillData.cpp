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

void UPE_SkillData::GetAoEBoundsAndRotation(FIntPoint CasterPos, FIntPoint TargetPos, FVector2D& OutSize, float& OutRadius, FRotator& OutRotation) const
{
	FVector2D Dir(TargetPos.X - CasterPos.X, TargetPos.Y - CasterPos.Y);
	if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0); // 제자리 클릭 보호
	Dir.Normalize();

	// 기본 각도 도출 (마우스 방향)
	float Angle = FMath::Atan2(Dir.Y, Dir.X);
	OutRotation = FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f);

	switch (AoEShape)
	{
	case EPEAoEShape::Line:
	{
		// 선형 스킬의 로컬 크기 (X축이 사거리 길이, Y축이 레이저 폭)
		OutSize.X = BaseRange * 100.f;
		OutSize.Y = ((LineWidth * 2.f) + 1.f) * 100.f;
		OutRadius = OutSize.X * 0.5f;

		// 선형 스킬은 마우스를 향한 연속적인 각도를 그대로 사용
		break;
	}
	case EPEAoEShape::Custom:
	{
		// 오프셋 자체의 미니멈/맥시멈을 구해 '회전하기 전의 순수 로컬 크기'를 구함
		int32 MinX = 999999, MaxX = -999999;
		int32 MinY = 999999, MaxY = -999999;

		for (const FIntPoint& Offset : CustomAoEOffsets)
		{
			MinX = FMath::Min(MinX, Offset.X);
			MaxX = FMath::Max(MaxX, Offset.X);
			MinY = FMath::Min(MinY, Offset.Y);
			MaxY = FMath::Max(MaxY, Offset.Y);
		}

		OutSize.X = (MaxX - MinX + 1) * 100.f;
		OutSize.Y = (MaxY - MinY + 1) * 100.f;
		OutRadius = FMath::Max(OutSize.X, OutSize.Y) * 0.5f;

		// 커스텀 스킬은 타일 그리드에 맞게 90도(4방향)로 스냅(Snap)된 회전값을 반환
		if (bRotateToTarget)
		{
			int32 DirIdx = FMath::RoundToInt(Angle / (PI / 2.f));
			OutRotation = FRotator(0.f, DirIdx * 90.f, 0.f);
		}
		else
		{
			OutRotation = FRotator::ZeroRotator;
		}
		break;
	}
	default:
	{
		// 그 외 (Square, Cross, Ring, None)
		float Side = (AoESize * 2.f + 1.f) * 100.f;
		if (AoEShape == EPEAoEShape::None) Side = 100.f;

		OutSize = FVector2D(Side, Side);
		OutRadius = Side * 0.5f;
		OutRotation = FRotator::ZeroRotator; // 대칭형이므로 회전 불필요
		break;
	}
	}
}