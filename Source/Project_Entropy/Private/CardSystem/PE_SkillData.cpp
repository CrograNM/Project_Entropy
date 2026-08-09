// Copyright CrograNM

#include "CardSystem/PE_SkillData.h"

TSet<FIntPoint> UPE_SkillData::GetAffectedGridPositions(FIntPoint CenterPos) const
{
	TSet<FIntPoint> AffectedPositions;
	AffectedPositions.Add(CenterPos); // 기본 중심점 포함

	if (AoEShape == EPEAoEShape::Cross)
	{
		for (int32 i = 1; i <= AoESize; ++i)
		{
			AffectedPositions.Add(CenterPos + FIntPoint(i, 0));
			AffectedPositions.Add(CenterPos + FIntPoint(-i, 0));
			AffectedPositions.Add(CenterPos + FIntPoint(0, i));
			AffectedPositions.Add(CenterPos + FIntPoint(0, -i));
		}
	}
	else if (AoEShape == EPEAoEShape::Square)
	{
		for (int32 x = -AoESize; x <= AoESize; ++x)
			for (int32 y = -AoESize; y <= AoESize; ++y)
				AffectedPositions.Add(CenterPos + FIntPoint(x, y));
	}
	else if (AoEShape == EPEAoEShape::Ring)
	{
		for (int32 x = -AoESize; x <= AoESize; ++x)
			for (int32 y = -AoESize; y <= AoESize; ++y)
				if (FMath::Abs(x) == AoESize || FMath::Abs(y) == AoESize)
					AffectedPositions.Add(CenterPos + FIntPoint(x, y));
	}
	else if (AoEShape == EPEAoEShape::Custom)
	{
		for (const FIntPoint& Offset : CustomAoEOffsets)
			AffectedPositions.Add(CenterPos + Offset);
	}

	return AffectedPositions;
}