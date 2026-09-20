// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "CardSystem/PE_DataTypes.h"
#include "Grid/PE_GridMath.h"

class AACGridSystem;
class AACTile;
class APE_CharacterBase;

/*
	EPETargetResult - 타겟 판정 결과: 타겟이 유효한지, 아니라면 왜 아닌지

	FPETargetCandidate.Result에 들어감
	실패 문구를 규칙이 직접 돌려줌
 */
enum class EPETargetResult : uint8
{
	Valid,				// 유효: 시전 가능
	NoTargetNeeded,		// 지정 불필요: Self, All_Enemies 등 지정 자체가 필요 없음
	NoTileSpecified,	// 타일 특정 불가: 커서가 허공이거나 그리드 밖
	OutOfRange,			// 사거리 초과
	NoCharacterOnTile,	// 캐릭터 없음: Snap_* 인데 그 칸이 비어 있음
	WrongTeam,			// 팀 불일치: 적 스킬을 아군에게 (또는 그 반대)
	TargetIsDead,		// 시체: 캐릭터가 죽어 있음 (즉, 액터는 살아 있지만 StatComponent가 죽음 상태)
};

/*
	FPETargetCandidate - 타겟 후보 (판정 1회 결과)
*/
struct PROJECT_ENTROPY_API FPETargetCandidate
{
	AACTile* Tile = nullptr;

	// Snap_* 일 때만 채워집니다. Tile 대상 스킬은 항상 nullptr.
	APE_CharacterBase* Character = nullptr;

	EPETargetResult Result = EPETargetResult::NoTileSpecified;

	/** 시전을 진행해도 되는 상태인가 (지정이 불필요한 스킬도 포함) */
	bool IsCastable() const { return Result == EPETargetResult::Valid || Result == EPETargetResult::NoTargetNeeded; }
};

/*
	FPETargetContext - 한 번의 조준 세션 동안 고정되는 값 + 도달 가능 칸 집합.

	Evaluate는 집합 조회만, 사거리 BFS를 여기서 한 번만 돌려둠
	호버는 매 프레임 판정하지만, 컨텍스트는 조준 상태가 바뀔 때만 다시 만듬
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

/*
	FPETargetRules - 타겟 판정의 단일 출처 (상태 없는 순수 함수 모음).

	궤적(FPESkillTrajectory)에 의존하지 않는다
	월드를 바꾸지 않으므로 클라이언트에서도 안전하게 호출 가능
	커서 호버 / 카드 드롭 / 서버 검증 / 턴 종료 자동시전 / 적 AI 가 전부 여기를 통과

	"시전해도 되는가"와 "어디에 맞는가"는 별개의 질문이고, 전자에만 답을 줌
*/
struct PROJECT_ENTROPY_API FPETargetRules
{
	// 지정 자체가 필요한가? - Self, All_Enemies 등
	static bool RequiresTarget(EPESkillTargetType TargetType);

	// FPETargetContext 생성 및 반환 - 사거리 BFS 1회. (조준 시작 / 서버 요청 수신 / AI 판단 때 한 번씩만 호출)
	static FPETargetContext MakeContext(const AACGridSystem* Grid, const APE_CharacterBase* Caster, EPESkillTargetType TargetType, int32 Range);

	// FPETargetCandidate 생성 - FPETargetContext의 Reachable 조회 O(1) 및 판정 - 팀 관계, 생존 여부, Snap_* 해석
	static FPETargetCandidate Evaluate(const FPETargetContext& Context, FIntPoint TargetPos);

	// 사거리 안의 유효 후보 전부.
	static TArray<FPETargetCandidate> CollectValidTargets(const FPETargetContext& Context);

	// 지정이 가능한가? - 팀 관계 + 생존 판정. Snap_Enemy / Snap_Ally / Self / All_Enemies 해석
	static bool IsTargetableBy(const APE_CharacterBase* Candidate, const APE_CharacterBase* Caster, EPESkillTargetType TargetType);

	// 실패 사유 텍스트 - 유저에게 보여줄 문구
	static FText GetFailureText(EPETargetResult Result);
};
