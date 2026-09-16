// Copyright CrograNM

#include "Grid/PE_GridMath.h"

FIntPoint FPEGridMath::SnapToCardinal(FIntPoint Delta)
{
	if (Delta == FIntPoint::ZeroValue) return FIntPoint::ZeroValue;

	// 정수 비교만으로 판정하므로 조준 각도가 애매해도 결과가 흔들리지 않습니다.
	return (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
		? FIntPoint(Delta.X > 0 ? 1 : -1, 0)
		: FIntPoint(0, Delta.Y > 0 ? 1 : -1);
}

int32 FPEGridMath::GetCardinalQuarterTurns(FIntPoint Delta)
{
	const FIntPoint Dir = SnapToCardinal(Delta);

	if (Dir.Y > 0) return 1;
	if (Dir.X < 0) return 2;
	if (Dir.Y < 0) return 3;
	return 0; // +X 또는 제자리
}
