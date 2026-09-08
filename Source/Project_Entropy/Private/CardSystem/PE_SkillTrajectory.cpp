// Copyright CrograNM

#include "CardSystem/PE_SkillTrajectory.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillActionActor.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Engine/World.h"

namespace
{
	const FIntPoint InvalidGridPos(PESkillTrajectory::InvalidCoord, PESkillTrajectory::InvalidCoord);

	/** 투사체가 막힌 지점을 논리 좌표(칸)로 되돌립니다. 되돌릴 수 없으면 InvalidGridPos를 반환합니다. */
	FIntPoint ResolveGridPosAtImpact(const AACGridSystem* Grid, const FHitResult& HitResult, const APE_CharacterBase* HitCharacter)
	{
		// 캐릭터/장애물에 막혔다면 그 대상이 서 있는 칸이 곧 착탄 칸입니다.
		if (HitCharacter)
		{
			if (const UACGridMovementComponent* HitMove = HitCharacter->GetGridMovementComponent())
				return HitMove->GetGridPosition();
		}

		if (const AACTile* HitTile = Cast<AACTile>(HitResult.GetActor()))
		{
			return HitTile->GetGridPosition();
		}

		// 그리드에 속하지 않은 레벨 지오메트리(벽 등)에 막힌 경우: 충돌 지점에서 가장 가까운 칸으로 되돌립니다.
		if (Grid)
		{
			if (const AACTile* NearestTile = Grid->GetNearestTile(HitResult.Location))
				return NearestTile->GetGridPosition();
		}

		return InvalidGridPos;
	}
}

bool FPESkillTrajectory::CanBeBlocked(const FPESkillHitPhase& Phase)
{
	// 스폰될 액터가 없으면 실제로 날아가는 물체가 없으므로 막힐 일도 없습니다.
	return Phase.SkillActorClass != nullptr && Phase.ProjectileSpeed > 0.f && Phase.bDestroyOnHit;
}

bool FPESkillTrajectory::IsPiercingProjectile(const FPESkillHitPhase& Phase)
{
	return Phase.SkillActorClass != nullptr && Phase.ProjectileSpeed > 0.f && !Phase.bDestroyOnHit;
}

FIntPoint FPESkillTrajectory::SnapToCardinalDirection(FIntPoint Delta)
{
	if (Delta == FIntPoint::ZeroValue) return FIntPoint::ZeroValue;

	// 정수 비교만으로 판정하므로 조준 각도가 애매해도 결과가 흔들리지 않습니다.
	return (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
		? FIntPoint(Delta.X > 0 ? 1 : -1, 0)
		: FIntPoint(0, Delta.Y > 0 ? 1 : -1);
}

int32 FPESkillTrajectory::GetCardinalQuarterTurns(FIntPoint Delta)
{
	const FIntPoint Dir = SnapToCardinalDirection(Delta);

	if (Dir.Y > 0) return 1;
	if (Dir.X < 0) return 2;
	if (Dir.Y < 0) return 3;
	return 0; // +X 또는 제자리
}

FIntPoint FPESkillTrajectory::ClampLineTarget(const AACGridSystem* Grid, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase)
{
	if (!Grid || Phase.AoEShape != EPEAoEShape::Line || TargetPos == InvalidGridPos) return TargetPos;

	FVector2D Dir = (FVector2D(TargetPos.X, TargetPos.Y) - FVector2D(CasterPos.X, CasterPos.Y)).GetSafeNormal();
	if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

	// 시전자에서 한 칸씩 뻗어나가며 그리드가 끊기기 직전 칸을 최종 목표로 삼습니다.
	FIntPoint LastValidPos = CasterPos;
	for (int32 Step = 1; Step <= BaseRange; ++Step)
	{
		const FIntPoint TestPos = CasterPos + FIntPoint(FMath::RoundToInt(Dir.X * Step), FMath::RoundToInt(Dir.Y * Step));
		if (Grid->GetTileAtPosition(TestPos)) LastValidPos = TestPos;
		else break;
	}
	return LastValidPos;
}

FPESkillAimPoint FPESkillTrajectory::ResolveAim(const AACGridSystem* Grid, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase)
{
	FPESkillAimPoint Aim;
	Aim.GridPos = ClampLineTarget(Grid, CasterPos, TargetPos, BaseRange, Phase);

	if (!Grid || Aim.GridPos == InvalidGridPos) return Aim;

	// 캐릭터가 서 있으면 발밑이 아니라 몸통을 겨냥해야 스윕이 실제 충돌과 일치합니다.
	if (APE_CharacterBase* TargetChar = Grid->GetCharacterAtPosition(Aim.GridPos))
	{
		Aim.WorldLocation = TargetChar->GetActorLocation();
		if (const UCapsuleComponent* Cap = TargetChar->FindComponentByClass<UCapsuleComponent>())
			Aim.WorldLocation.Z += Cap->GetScaledCapsuleHalfHeight() * PESkillTrajectory::AimHeightRatio;
	}
	else if (const AACTile* Tile = Grid->GetTileAtPosition(Aim.GridPos))
	{
		Aim.WorldLocation = Tile->GetActorLocation();
		Aim.WorldLocation.Z += PESkillTrajectory::TileAimHeightOffset;
	}

	return Aim;
}

FVector FPESkillTrajectory::GetMuzzleLocationForDirection(const AActor* Caster, const FVector& Direction)
{
	if (!Caster) return FVector::ZeroVector;

	FVector Muzzle = Caster->GetActorLocation();
	if (const UCapsuleComponent* Cap = Caster->FindComponentByClass<UCapsuleComponent>())
		Muzzle.Z += Cap->GetScaledCapsuleHalfHeight() * PESkillTrajectory::MuzzleHeightRatio;

	FVector FlatDir = Direction.GetSafeNormal2D();
	if (FlatDir.IsNearlyZero()) FlatDir = Caster->GetActorForwardVector();

	return Muzzle + FlatDir * PESkillTrajectory::MuzzleForwardOffset;
}

FVector FPESkillTrajectory::GetMuzzleLocation(const AActor* Caster, const FVector& AimLocation)
{
	if (!Caster) return AimLocation;

	// 시전자의 현재 회전이 아니라 '조준 방향'으로 총구를 밀어냅니다.
	// (회전 보간이 끝나기 전에 발사되면 궤적이 몸통을 뚫고 나가는 것처럼 보이므로)
	return GetMuzzleLocationForDirection(Caster, AimLocation - Caster->GetActorLocation());
}

FPESkillTrajectoryResult FPESkillTrajectory::Sweep(const UWorld* World, const AACGridSystem* Grid, const AActor* Caster, const FVector& MuzzleLocation, const FPESkillAimPoint& Aim, const FPESkillHitPhase& Phase)
{
	FPESkillTrajectoryResult Result;
	Result.StartLocation = MuzzleLocation;
	Result.AimLocation = Aim.WorldLocation;
	Result.EndLocation = Aim.WorldLocation;
	Result.EndGridPos = Aim.GridPos;
	Result.PathPoints.Add(MuzzleLocation);

	if (!World) return Result;

	// 투사체가 아니면(즉발/근접) 총구에서 조준점까지 직선 한 구간으로 끝냅니다.
	if (Phase.ProjectileSpeed <= 0.f)
	{
		Result.PathPoints.Add(Aim.WorldLocation);
		return Result;
	}

	// 관통이거나 실제 투사체가 없는 페이즈는 장애물을 무시하고 조준점 끝단까지 그대로 뻗습니다.
	const bool bCanBeBlocked = CanBeBlocked(Phase);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Caster);
	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(PESkillTrajectory::SweepRadius);

	FVector LastPos = MuzzleLocation;
	for (int32 Step = 1; Step <= PESkillTrajectory::SweepSegments; ++Step)
	{
		const float Alpha = (float)Step / (float)PESkillTrajectory::SweepSegments;
		FVector NextPos = FMath::Lerp(MuzzleLocation, Aim.WorldLocation, Alpha);

		// Alpha가 0.5(중간)일 때 Sin(0.5 * PI) = 1 이 되어 최고점(ProjectileGravity)에 도달합니다.
		if (Phase.ProjectileGravity > 0.f)
			NextPos.Z += FMath::Sin(Alpha * PI) * Phase.ProjectileGravity;

		if (bCanBeBlocked)
		{
			FHitResult HitResult;
			if (World->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
			{
				Result.bBlocked = true;
				Result.EndLocation = HitResult.Location;
				Result.HitCharacter = Cast<APE_CharacterBase>(HitResult.GetActor());
				Result.PathPoints.Add(Result.EndLocation);

				// 되돌릴 수 없는 지점이면 원래 조준 칸을 그대로 둡니다 (범위가 통째로 사라지는 것보다 안전).
				const FIntPoint ImpactGridPos = ResolveGridPosAtImpact(Grid, HitResult, Result.HitCharacter);
				if (ImpactGridPos != InvalidGridPos) Result.EndGridPos = ImpactGridPos;

				return Result;
			}
		}

		Result.PathPoints.Add(NextPos);
		LastPos = NextPos;
	}

	return Result;
}

FPESkillTrajectoryResult FPESkillTrajectory::Solve(const UWorld* World, const AACGridSystem* Grid, const AActor* Caster, FIntPoint CasterPos, FIntPoint TargetPos, int32 BaseRange, const FPESkillHitPhase& Phase)
{
	const FPESkillAimPoint Aim = ResolveAim(Grid, CasterPos, TargetPos, BaseRange, Phase);
	return Sweep(World, Grid, Caster, GetMuzzleLocation(Caster, Aim.WorldLocation), Aim, Phase);
}
