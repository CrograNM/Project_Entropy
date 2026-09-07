// Copyright CrograNM

#include "CardSystem/PE_SkillTrajectory.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Engine/World.h"

namespace
{
	const FIntPoint InvalidGridPos(PESkillTrajectory::InvalidCoord, PESkillTrajectory::InvalidCoord);
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

FVector FPESkillTrajectory::GetMuzzleLocation(const AActor* Caster, const FVector& AimLocation)
{
	if (!Caster) return AimLocation;

	FVector Muzzle = Caster->GetActorLocation();
	if (const UCapsuleComponent* Cap = Caster->FindComponentByClass<UCapsuleComponent>())
		Muzzle.Z += Cap->GetScaledCapsuleHalfHeight() * PESkillTrajectory::MuzzleHeightRatio;

	// 시전자의 현재 회전이 아니라 '조준 방향'으로 총구를 밀어냅니다.
	// (회전 보간이 끝나기 전에 발사되면 궤적이 몸통을 뚫고 나가는 것처럼 보이므로)
	FVector AimDir = (AimLocation - Caster->GetActorLocation()).GetSafeNormal2D();
	if (AimDir.IsNearlyZero()) AimDir = Caster->GetActorForwardVector();

	return Muzzle + AimDir * PESkillTrajectory::MuzzleForwardOffset;
}

FPESkillTrajectoryResult FPESkillTrajectory::Sweep(const UWorld* World, const AActor* Caster, const FVector& MuzzleLocation, const FPESkillAimPoint& Aim, const FPESkillHitPhase& Phase)
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

		// 관통(bDestroyOnHit == false)은 장애물을 무시하고 조준점 끝단까지 그대로 뻗습니다.
		if (Phase.bDestroyOnHit)
		{
			FHitResult HitResult;
			if (World->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
			{
				Result.bBlocked = true;
				Result.EndLocation = HitResult.Location;
				Result.HitCharacter = Cast<APE_CharacterBase>(HitResult.GetActor());
				Result.PathPoints.Add(Result.EndLocation);

				// 막힌 지점의 논리 좌표를 되짚어 둡니다 (시각화의 단일 타겟 하이라이트용).
				if (Result.HitCharacter)
				{
					if (const UACGridMovementComponent* HitMove = Result.HitCharacter->GetGridMovementComponent())
						Result.EndGridPos = HitMove->GetGridPosition();
				}
				else if (const AACTile* HitTile = Cast<AACTile>(HitResult.GetActor()))
				{
					Result.EndGridPos = HitTile->GetGridPosition();
				}

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
	return Sweep(World, Caster, GetMuzzleLocation(Caster, Aim.WorldLocation), Aim, Phase);
}
