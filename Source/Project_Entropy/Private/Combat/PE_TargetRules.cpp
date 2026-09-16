// Copyright CrograNM

#include "Combat/PE_TargetRules.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"

bool FPETargetRules::RequiresTarget(EPESkillTargetType TargetType)
{
	return TargetType != EPESkillTargetType::Self
		&& TargetType != EPESkillTargetType::All_Enemies;
}

FPETargetContext FPETargetRules::MakeContext(const AACGridSystem* Grid, const APE_CharacterBase* Caster,
	EPESkillTargetType TargetType, int32 Range)
{
	FPETargetContext Context;
	Context.Grid = Grid;
	Context.Caster = Caster;
	Context.TargetType = TargetType;
	Context.Range = Range;

	const UACGridMovementComponent* Move = Caster ? Caster->GetGridMovementComponent() : nullptr;
	if (!Grid || !Move) return Context;

	Context.CasterPos = Move->GetGridPosition();

	// 스킬 사거리는 유닛을 관통해 뻗습니다. 앞이 막히는 것은 궤적이 따로 판단합니다.
	Context.Reachable = Grid->ComputeReachablePositions(Context.CasterPos, Range, /*bIsMovement=*/false, Caster);

	return Context;
}

bool FPETargetRules::IsTargetableBy(const APE_CharacterBase* Candidate, const APE_CharacterBase* Caster,
	EPESkillTargetType TargetType)
{
	if (!Candidate) return false;

	const UACStatComponent* Stat = Candidate->GetStatComponent();
	if (!Stat || Stat->IsDead()) return false;

	const int32 CasterTeam = Caster ? Caster->GetTeamID() : INDEX_NONE;

	switch (TargetType)
	{
	case EPESkillTargetType::Snap_Enemy:
	case EPESkillTargetType::All_Enemies:
		return Candidate->GetTeamID() != CasterTeam;

	case EPESkillTargetType::Snap_Ally:
		return Candidate->GetTeamID() == CasterTeam;

	case EPESkillTargetType::Self:
		return Candidate == Caster;

	case EPESkillTargetType::Tile:
	default:
		// 타일 대상은 그 칸에 누가 서 있든 상관하지 않습니다.
		return true;
	}
}

FPETargetCandidate FPETargetRules::Evaluate(const FPETargetContext& Context, FIntPoint TargetPos)
{
	FPETargetCandidate Out;

	if (!RequiresTarget(Context.TargetType))
	{
		Out.Result = EPETargetResult::NoTargetNeeded;
		return Out;
	}

	if (!Context.IsValid() || TargetPos == PEGridMath::InvalidGridPos())
	{
		Out.Result = EPETargetResult::NoTileSpecified;
		return Out;
	}

	Out.Tile = Context.Grid->GetTileAtPosition(TargetPos);
	if (!Out.Tile)
	{
		Out.Result = EPETargetResult::NoTileSpecified;
		return Out;
	}

	if (!Context.Reachable.Contains(TargetPos))
	{
		Out.Result = EPETargetResult::OutOfRange;
		return Out;
	}

	if (Context.TargetType == EPESkillTargetType::Tile)
	{
		Out.Result = EPETargetResult::Valid;
		return Out;
	}

	// --- Snap_* : 그 칸에 선 캐릭터가 조건을 만족해야 합니다 ---
	APE_CharacterBase* Occupant = Context.Grid->GetCharacterAtPosition(TargetPos);
	if (!Occupant)
	{
		Out.Result = EPETargetResult::NoCharacterOnTile;
		return Out;
	}

	const UACStatComponent* Stat = Occupant->GetStatComponent();
	if (!Stat || Stat->IsDead())
	{
		// 시체는 액터가 파괴될 때까지 레지스트리에 남습니다. 대상으로는 잡히지 않습니다.
		Out.Result = EPETargetResult::TargetIsDead;
		return Out;
	}

	if (!IsTargetableBy(Occupant, Context.Caster, Context.TargetType))
	{
		Out.Result = EPETargetResult::WrongTeam;
		return Out;
	}

	Out.Character = Occupant;
	Out.Result = EPETargetResult::Valid;
	return Out;
}

TArray<FPETargetCandidate> FPETargetRules::CollectValidTargets(const FPETargetContext& Context)
{
	TArray<FPETargetCandidate> Out;
	if (!Context.IsValid() || !RequiresTarget(Context.TargetType)) return Out;

	if (Context.TargetType == EPESkillTargetType::Tile)
	{
		// 사거리 안의 모든 타일이 후보입니다.
		Out.Reserve(Context.Reachable.Num());
		for (const FIntPoint& Pos : Context.Reachable)
		{
			FPETargetCandidate Candidate = Evaluate(Context, Pos);
			if (Candidate.Result == EPETargetResult::Valid) Out.Add(MoveTemp(Candidate));
		}
		return Out;
	}

	/*
		Snap_* 는 점유 레지스트리만 훑습니다.
		맵 전체 액터를 뒤지던 GetAllActorsOfClass 대비 후보 수가 '실제로 서 있는 캐릭터 수'로 줄고,
		사거리 판정이 Evaluate와 동일해지므로 손으로 찍을 수 없는 칸을 AI가 고르는 일이 없어집니다.
	*/
	for (const TPair<FIntPoint, APE_CharacterBase*>& Entry : Context.Grid->GetOccupancyMap())
	{
		if (!Context.Reachable.Contains(Entry.Key)) continue;

		FPETargetCandidate Candidate = Evaluate(Context, Entry.Key);
		if (Candidate.Result == EPETargetResult::Valid) Out.Add(MoveTemp(Candidate));
	}

	return Out;
}

FText FPETargetRules::GetFailureText(EPETargetResult Result)
{
	switch (Result)
	{
	case EPETargetResult::NoTileSpecified:
		return FText::FromString(TEXT("시전 취소: 타겟을 지정하지 않았습니다."));
	case EPETargetResult::OutOfRange:
		return FText::FromString(TEXT("시전 취소: 사거리 밖입니다."));
	case EPETargetResult::NoCharacterOnTile:
		return FText::FromString(TEXT("시전 취소: 그 칸에 대상이 없습니다."));
	case EPETargetResult::WrongTeam:
		return FText::FromString(TEXT("시전 취소: 대상이 될 수 없는 진영입니다."));
	case EPETargetResult::TargetIsDead:
		return FText::FromString(TEXT("시전 취소: 이미 쓰러진 대상입니다."));
	default:
		return FText::FromString(TEXT("시전 취소: 유효하지 않은 타겟입니다."));
	}
}
