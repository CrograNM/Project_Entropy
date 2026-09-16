// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_DataTypes.h"
#include "Grid/PE_GridMath.h"

class AACGridSystem;
class AACTile;
class APE_CharacterBase;

/**
 * 타겟이 유효한지, 아니라면 왜 아닌지.
 * 실패 문구를 호출부가 제각각 지어내지 않도록 사유를 규칙이 직접 돌려줍니다.
 */
enum class EPETargetResult : uint8
{
	Valid,
	NoTargetNeeded,    // Self / All_Enemies - 지정 자체가 필요 없음
	NoTileSpecified,   // 커서가 허공이거나 그리드 밖
	OutOfRange,
	NoCharacterOnTile, // Snap_* 인데 그 칸이 비어 있음
	WrongTeam,         // 적 스킬을 아군에게 (또는 그 반대)
	TargetIsDead,
};

/** 판정 1회의 결과. 유효하다면 시전에 필요한 것이 전부 들어 있습니다. */
struct PROJECT_ENTROPY_API FPETargetCandidate
{
	AACTile* Tile = nullptr;

	// Snap_* 일 때만 채워집니다. Tile 대상 스킬은 항상 nullptr.
	APE_CharacterBase* Character = nullptr;

	EPETargetResult Result = EPETargetResult::NoTileSpecified;

	/** 시전을 진행해도 되는 상태인가 (지정이 불필요한 스킬도 포함) */
	bool IsCastable() const { return Result == EPETargetResult::Valid || Result == EPETargetResult::NoTargetNeeded; }
};

/**
 * 한 번의 조준 세션 동안 고정되는 값 + 도달 가능 칸 집합.
 *
 * 사거리 BFS를 여기서 한 번만 돌려두고 Evaluate는 집합 조회만 합니다.
 * 호버는 매 프레임 판정하지만 컨텍스트는 조준 상태가 바뀔 때만 다시 만듭니다.
 *
 * Range를 값으로 들고 있는 것이 중요합니다. SkillData->BaseRange를 내부에서 읽지 않으므로,
 * 마석/버프로 사거리가 변동되면(P1-1) 이 값만 바꿔 넣으면 모든 판정이 따라옵니다.
 */
struct PROJECT_ENTROPY_API FPETargetContext
{
	const AACGridSystem* Grid = nullptr;
	const APE_CharacterBase* Caster = nullptr;

	EPESkillTargetType TargetType = EPESkillTargetType::Tile;
	int32 Range = 0;

	FIntPoint CasterPos = PEGridMath::InvalidGridPos();
	TSet<FIntPoint> Reachable;

	bool IsValid() const { return Grid != nullptr && Caster != nullptr; }
};

/**
 * 타겟 판정의 단일 출처 (상태 없는 순수 함수 모음).
 *
 * 커서 호버 / 카드 드롭 / 서버 검증 / 턴종료 자동시전 / 적 AI 가 전부 여기를 통과합니다.
 * 월드를 바꾸지 않으므로 클라이언트에서도 안전하게 호출할 수 있습니다.
 *
 * 궤적(FPESkillTrajectory)에 의존하지 않는 것이 의도입니다.
 * "시전해도 되는가"와 "어디에 맞는가"는 별개의 질문이고, 이 구조체는 앞의 것만 답합니다.
 * 캐릭터 뒤의 타일을 노리는 것은 유효한 시전이며, 앞에서 막혀 일찍 떨어질 뿐입니다.
 */
struct PROJECT_ENTROPY_API FPETargetRules
{
	/** 지정 자체가 필요 없는 스킬인가 (Self, All_Enemies) */
	static bool RequiresTarget(EPESkillTargetType TargetType);

	/** 사거리 BFS 1회. 조준 시작 / 서버 요청 수신 / AI 판단 때 한 번씩만 부릅니다. */
	static FPETargetContext MakeContext(const AACGridSystem* Grid, const APE_CharacterBase* Caster,
		EPESkillTargetType TargetType, int32 Range);

	/** 좌표 1곳을 판정합니다. Reachable 조회라 O(1)이므로 매 프레임 불러도 됩니다. */
	static FPETargetCandidate Evaluate(const FPETargetContext& Context, FIntPoint TargetPos);

	/** 사거리 안의 유효 후보 전부. 점유 레지스트리만 훑으므로 액터 전수 스캔이 없습니다. */
	static TArray<FPETargetCandidate> CollectValidTargets(const FPETargetContext& Context);

	/** 팀 관계 + 생존 판정. Snap_Enemy / Snap_Ally / Self / All_Enemies 해석이 여기 한 곳에 있습니다. */
	static bool IsTargetableBy(const APE_CharacterBase* Candidate, const APE_CharacterBase* Caster,
		EPESkillTargetType TargetType);

	/** 실패 사유 -> 유저에게 보여줄 문구 */
	static FText GetFailureText(EPETargetResult Result);
};
