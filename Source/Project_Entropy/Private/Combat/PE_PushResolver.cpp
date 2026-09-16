// Copyright CrograNM

#include "Combat/PE_PushResolver.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"

namespace
{
	/** 시체는 액터가 파괴될 때까지 점유 레지스트리에 남으므로 길을 막지 않습니다. */
	bool IsAliveBlocker(const APE_CharacterBase* Char)
	{
		if (!Char) return false;

		const UACStatComponent* Stat = Char->GetStatComponent();
		return Stat != nullptr && !Stat->IsDead();
	}
}

// --- [FPEPushField] ---

FIntPoint FPEPushField::GetPosition(const APE_CharacterBase* Char) const
{
	if (const FIntPoint* Virtual = VirtualMoves.Find(Char)) return *Virtual;

	const UACGridMovementComponent* Move = Char ? Char->GetGridMovementComponent() : nullptr;
	return Move ? Move->GetGridPosition() : PEGridMath::InvalidGridPos();
}

APE_CharacterBase* FPEPushField::GetBlockerAt(FIntPoint Pos, const APE_CharacterBase* Ignore) const
{
	// 가상으로 옮겨진 대상이 이 칸을 차지했는지 먼저 봅니다.
	// 실행 경로에서는 VirtualMoves가 비어 있어 이 순회가 아예 돌지 않습니다.
	for (const TPair<const APE_CharacterBase*, FIntPoint>& Moved : VirtualMoves)
	{
		if (Moved.Value == Pos && Moved.Key != Ignore && IsAliveBlocker(Moved.Key))
		{
			return const_cast<APE_CharacterBase*>(Moved.Key);
		}
	}

	if (!Grid) return nullptr;

	APE_CharacterBase* Occupant = Grid->GetCharacterAtPosition(Pos, const_cast<APE_CharacterBase*>(Ignore));

	// 이미 가상으로 떠났다면 이 칸은 비어 있습니다.
	if (!Occupant || VirtualMoves.Contains(Occupant)) return nullptr;

	return IsAliveBlocker(Occupant) ? Occupant : nullptr;
}

void FPEPushField::ApplyVirtual(const FPEPushStep& Step)
{
	if (Step.Target) VirtualMoves.Add(Step.Target, Step.EndPos);
}

// --- [FPEPushResolver] ---

FIntPoint FPEPushResolver::ResolveDirection(EPEPushType Type, FIntPoint InstigatorPos, FIntPoint OriginPos, FIntPoint TargetPos)
{
	if (Type == EPEPushType::Directional)
	{
		// 지향성은 시전자에서 목표 칸을 향하는 방향 하나를 모두가 공유합니다.
		return FPEGridMath::SnapToCardinal(OriginPos - InstigatorPos);
	}

	// 폭발 중심에 정확히 선 대상은 시전자 반대편으로, 나머지는 중심에서 바깥으로 밀립니다.
	const FIntPoint Origin = (TargetPos == OriginPos) ? InstigatorPos : OriginPos;
	return FPEGridMath::SnapToCardinal(TargetPos - Origin);
}

TArray<FPEPushRequest> FPEPushResolver::BuildRequests(const FPEPushField& Field, AActor* Instigator, FIntPoint OriginPos,
	const TSet<APE_CharacterBase*>& Targets, EPEPushType Type, int32 Distance, float CollisionDamageRatio)
{
	TArray<FPEPushRequest> Requests;

	APE_CharacterBase* InstigatorChar = Cast<APE_CharacterBase>(Instigator);
	if (!Field.IsValid() || !InstigatorChar || Distance <= 0 || Targets.IsEmpty()) return Requests;

	const FIntPoint InstigatorPos = Field.GetPosition(InstigatorChar);

	for (APE_CharacterBase* Target : Targets)
	{
		if (!Target || !Target->IsPushable() || Target == InstigatorChar) continue;
		if (!IsAliveBlocker(Target)) continue;

		const FIntPoint TargetPos = Field.GetPosition(Target);
		if (TargetPos == PEGridMath::InvalidGridPos()) continue;

		FIntPoint Dir = ResolveDirection(Type, InstigatorPos, OriginPos, TargetPos);

		// 제자리를 조준한 지향성 밀치기는 방향이 0이 되므로 시전자 기준 +X로 보호합니다.
		if (Dir == FIntPoint::ZeroValue && Type == EPEPushType::Directional) Dir = FIntPoint(1, 0);
		if (Dir == FIntPoint::ZeroValue) continue;

		FPEPushRequest& Request = Requests.AddDefaulted_GetRef();
		Request.Target = Target;
		Request.Instigator = Instigator;
		Request.Direction = Dir;
		Request.Distance = Distance;
		Request.BaseDistance = Distance;
		Request.CollisionDamageRatio = CollisionDamageRatio;
		Request.Cause = EPEPushCause::Skill;
	}

	return Requests;
}

FPEPushStep FPEPushResolver::ResolveSingle(const FPEPushField& Field, const FPEPushRequest& Request)
{
	FPEPushStep Step;
	Step.Target = Request.Target;
	Step.PushDir = Request.Direction;
	Step.RemainingDist = Request.Distance;

	const AACGridSystem* Grid = Field.GetGrid();
	if (!Grid || !Request.Target || Request.Direction == FIntPoint::ZeroValue) return Step;

	const FIntPoint StartPos = Field.GetPosition(Request.Target);
	Step.StartPos = StartPos;
	Step.EndPos = StartPos;

	FIntPoint CurrentPos = StartPos;

	for (int32 Tile = 1; Tile <= Request.Distance; ++Tile)
	{
		const FIntPoint NextPos = CurrentPos + Request.Direction;
		AACTile* NextTile = Grid->GetTileAtPosition(NextPos);

		// 맵 밖이거나 고정 장애물 타일이면 그 자리에서 멈춥니다.
		if (!NextTile || NextTile->IsObstacle())
		{
			Step.bBlocked = true;
			break;
		}

		if (APE_CharacterBase* Blocker = Field.GetBlockerAt(NextPos, Request.Target))
		{
			Step.bBlocked = true;
			Step.HitCharacter = Blocker;
			break;
		}

		CurrentPos = NextPos;
		Step.Path.Add(NextTile);
	}

	Step.EndPos = CurrentPos;
	return Step;
}

bool FPEPushResolver::MakeChainedRequest(const FPEPushStep& Step, const FPEPushRequest& Origin, FPEPushRequest& OutNext)
{
	if (!Step.bBlocked || !Step.HitCharacter || !Step.HitCharacter->IsPushable()) return false;

	// 부딪힌 칸까지 세면 Path.Num() + 1칸을 소모한 셈입니다.
	const int32 Remaining = Origin.Distance - (Step.Path.Num() + 1);
	if (Remaining <= 0) return false;

	// 방향 / 분모 / 피해 비율 / 시전자는 그대로 물려받습니다.
	OutNext = Origin;
	OutNext.Target = Step.HitCharacter;
	OutNext.Distance = Remaining;
	OutNext.Cause = EPEPushCause::Collision;
	return true;
}

void FPEPushResolver::SortBackToFront(TArray<FPEPushRequest>& Requests, const FPEPushField& Field)
{
	Requests.Sort([&Field](const FPEPushRequest& A, const FPEPushRequest& B)
		{
			const FIntPoint PosA = Field.GetPosition(A.Target);
			const FIntPoint PosB = Field.GetPosition(B.Target);

			return (PosA.X * A.Direction.X + PosA.Y * A.Direction.Y)
				 > (PosB.X * B.Direction.X + PosB.Y * B.Direction.Y);
		});
}

TArray<FPEPushStep> FPEPushResolver::SimulateChain(FPEPushField& Field, TArray<FPEPushRequest> Requests)
{
	TArray<FPEPushStep> Steps;
	if (!Field.IsValid()) return Steps;

	// 남은 거리가 매 회 최소 1씩 줄어 반드시 끝나지만, 방어적으로 상한을 둡니다.
	constexpr int32 MaxIterations = 64;

	for (int32 Iteration = 0; Iteration < MaxIterations && Requests.Num() > 0; ++Iteration)
	{
		SortBackToFront(Requests, Field);

		const FPEPushRequest Request = Requests[0];
		Requests.RemoveAt(0);

		const FPEPushStep Step = ResolveSingle(Field, Request);
		Field.ApplyVirtual(Step);
		Steps.Add(Step);

		FPEPushRequest Next;
		if (MakeChainedRequest(Step, Request, Next)) Requests.Add(Next);
	}

	return Steps;
}
