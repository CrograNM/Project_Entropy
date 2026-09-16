// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"

/** 논리 좌표(칸)를 다루는 모든 시스템이 공유하는 상수 */
namespace PEGridMath
{
	// 그리드 밖 / 미지정을 나타내는 무효 좌표
	inline constexpr int32 InvalidCoord = -999;

	FORCEINLINE FIntPoint InvalidGridPos() { return FIntPoint(InvalidCoord, InvalidCoord); }
}

/**
 * 격자 좌표 수학 (상태 없는 순수 함수 모음).
 *
 * 스킬 궤적(FPESkillTrajectory)과 밀치기(FPEPushResolver)가 둘 다 방향 판정을 필요로 하는데,
 * 어느 한쪽에 두면 나머지가 그쪽을 참조해야 합니다. 그래서 둘 다 모르는 가장 아래 계층에 둡니다.
 */
struct PROJECT_ENTROPY_API FPEGridMath
{
	/**
	 * 격자 델타를 상하좌우 4방향 중 하나로 스냅합니다. 정확한 대각선(|dx| == |dy|)은 항상 '수평'을 택합니다.
	 *
	 * 예전에는 Atan2 + RoundToInt로 각도를 반올림했는데, 45도에서 부동소수점 한 틱에 축이 통째로 뒤집혔고
	 * 북동->북 / 남동->동 / 북서->서 / 남서->남 처럼 회전 규칙성도 없었습니다.
	 * 정수 비교만 쓰므로 조준 각도가 애매해도 결과가 흔들리지 않습니다.
	 */
	static FIntPoint SnapToCardinal(FIntPoint Delta);

	/** 스냅된 4방향을 +X(0)에서 반시계로 센 90도 회전 횟수로 바꿉니다. 0=+X, 1=+Y, 2=-X, 3=-Y */
	static int32 GetCardinalQuarterTurns(FIntPoint Delta);
};
