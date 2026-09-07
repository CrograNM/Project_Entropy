// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "PE_SkillTrajectory.generated.h"

struct FPESkillHitPhase;
class AACGridSystem;
class APE_CharacterBase;

/**
 * 궤적 계산에 쓰이던 하드코딩 상수들의 단일 출처.
 * 서버 판정(UACSkillComponent)과 클라 예측(UACTargetingVisualizerComponent)이 반드시 같은 값을 봅니다.
 */
namespace PESkillTrajectory
{
	// 스윕을 나눌 구간 수
	inline constexpr int32 SweepSegments = 20;

	// 스윕에 사용할 스피어 반경
	inline constexpr float SweepRadius = 5.f;

	// 시전자 전방으로 총구를 밀어내는 거리
	inline constexpr float MuzzleForwardOffset = 70.f;

	// 시전자 캡슐 높이 대비 총구 높이 비율
	inline constexpr float MuzzleHeightRatio = 0.7f;

	// 대상 캡슐 높이 대비 조준점 높이 비율
	inline constexpr float AimHeightRatio = 0.8f;

	// 빈 타일을 조준할 때 타일 바닥에서 띄울 높이
	inline constexpr float TileAimHeightOffset = 20.f;

	// 그리드 밖 / 미지정을 나타내는 무효 좌표
	inline constexpr int32 InvalidCoord = -999;
}

/** 논리 좌표(그리드)와 그에 대응하는 조준용 월드 좌표 한 쌍 */
USTRUCT()
struct FPESkillAimPoint
{
	GENERATED_BODY()

	// Line 사거리 보정까지 끝난 최종 논리 좌표
	UPROPERTY()
	FIntPoint GridPos = FIntPoint(PESkillTrajectory::InvalidCoord, PESkillTrajectory::InvalidCoord);

	// 그 좌표를 조준할 때 겨냥할 월드 위치 (캐릭터가 있으면 가슴 높이, 없으면 타일 위)
	UPROPERTY()
	FVector WorldLocation = FVector::ZeroVector;
};

/** 총구 -> 조준점 스윕 결과. 시각화는 PathPoints를, 판정은 HitCharacter를 쓰면 어긋날 수 없습니다. */
USTRUCT()
struct FPESkillTrajectoryResult
{
	GENERATED_BODY()

	// 궤적 시작점(총구)
	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	// 아무것에도 막히지 않았을 때 도달했을 조준점
	UPROPERTY()
	FVector AimLocation = FVector::ZeroVector;

	// 실제 착탄 지점 (막혔으면 충돌 지점, 아니면 AimLocation과 동일)
	UPROPERTY()
	FVector EndLocation = FVector::ZeroVector;

	/**
	 * 착탄 지점의 논리 좌표.
	 * 막히지 않았으면 보정된 조준 좌표, 막혔으면 충돌한 캐릭터/타일의 좌표입니다.
	 * [주의] 서버 판정은 AoE 범위를 스윕 '이전'의 조준 좌표로 잡으므로 이 값을 쓰지 않습니다.
	 *        시각화(착탄 마커/단일 타겟 하이라이트) 전용입니다.
	 */
	UPROPERTY()
	FIntPoint EndGridPos = FIntPoint(PESkillTrajectory::InvalidCoord, PESkillTrajectory::InvalidCoord);

	// 스윕 도중 맞은 캐릭터 (지형/타일에 막혔거나 안 막혔으면 null)
	UPROPERTY()
	TObjectPtr<APE_CharacterBase> HitCharacter = nullptr;

	// 조준점에 닿기 전에 무언가에 막혔는지 여부
	UPROPERTY()
	bool bBlocked = false;

	// 시각화용 경로점. StartLocation을 첫 원소로 포함합니다.
	TArray<FVector> PathPoints;
};

/**
 * 스킬 궤적 해석기 (상태 없는 순수 함수 모음).
 *
 * 서버 판정과 클라 예측이 같은 코드를 통과하도록 만드는 것이 목적입니다.
 * 서버는 AoE 범위를 스윕 이전에 확정해야 하므로 ResolveAim / Sweep 을 나눠 호출하고,
 * 클라는 한 번에 끝내면 되므로 Solve 를 호출합니다.
 */
struct PROJECT_ENTROPY_API FPESkillTrajectory
{
	/** Line(직선/관통) 형태일 때 목표를 사거리 내 '가장 마지막 유효 타일'로 당깁니다. 그 외 형태는 그대로 반환합니다. */
	static FIntPoint ClampLineTarget(const AACGridSystem* Grid, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase);

	/** 좌표 보정(ClampLineTarget) + 그 좌표의 조준 월드 위치 산출. 캐릭터가 서 있으면 타일보다 캐릭터를 우선합니다. */
	static FPESkillAimPoint ResolveAim(const AACGridSystem* Grid, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase);

	/** 시전자 캡슐 높이와 조준 방향을 기준으로 투사체가 출발할 총구 위치를 구합니다. */
	static FVector GetMuzzleLocation(const AActor* Caster, const FVector& AimLocation);

	/** 총구에서 조준점까지 구간을 나눠 스윕합니다. 포물선(ProjectileGravity)도 여기서 함께 적용됩니다. */
	static FPESkillTrajectoryResult Sweep(const UWorld* World, const AActor* Caster, const FVector& MuzzleLocation, const FPESkillAimPoint& Aim, const FPESkillHitPhase& Phase);

	/** ResolveAim -> GetMuzzleLocation -> Sweep 을 한 번에 수행하는 편의 함수 (클라 예측용). */
	static FPESkillTrajectoryResult Solve(const UWorld* World, const AACGridSystem* Grid, const AActor* Caster, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase);
};
