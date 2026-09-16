// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Combat/PE_PushTypes.h"

class AACGridSystem;
class APE_CharacterBase;

/**
 * 밀치기 판정이 바라보는 전장.
 *
 * 기본적으로 GridSystem 그 자체입니다. 사본을 뜨지 않습니다.
 * 시각화만 ApplyVirtual로 "A가 여기로 밀려났다고 치면"이라는 가정을 얹고,
 * 실행 경로는 가정을 얹지 않으므로 VirtualMoves가 비어 조회가 그리드 직통으로 끝납니다.
 *
 * 실제 점유는 이동이 '시작될 때' 목적지로 갱신되므로(UACGridMovementComponent::ProcessNextCommand),
 * 실행 경로는 앞 요청이 출발한 결과를 별도 처리 없이 그대로 보게 됩니다.
 */
struct PROJECT_ENTROPY_API FPEPushField
{
	explicit FPEPushField(const AACGridSystem* InGrid) : Grid(InGrid) {}

	const AACGridSystem* GetGrid() const { return Grid; }
	bool IsValid() const { return Grid != nullptr; }

	/** 대상이 지금 서 있는 칸. 가정이 얹혀 있으면 가정된 칸을 돌려줍니다. */
	FIntPoint GetPosition(const APE_CharacterBase* Char) const;

	/** 그 칸을 막고 선 캐릭터. 시체는 막지 않습니다 (액터가 파괴될 때까지 레지스트리에 남아 있으므로). */
	APE_CharacterBase* GetBlockerAt(FIntPoint Pos, const APE_CharacterBase* Ignore) const;

	/** 시각화 전용. 실행 경로는 절대 호출하지 않습니다. */
	void ApplyVirtual(const FPEPushStep& Step);

private:
	const AACGridSystem* Grid = nullptr;

	// 이번 시뮬레이션이 가상으로 옮긴 대상만 담깁니다. 보통 0~3개.
	TMap<const APE_CharacterBase*, FIntPoint> VirtualMoves;
};

/**
 * 밀치기 규칙의 단일 출처 (상태 없는 순수 함수 모음).
 *
 * 월드를 바꾸지 않으므로 클라이언트에서도 안전하게 호출할 수 있습니다.
 * 연쇄를 한꺼번에 전개하지 않고 ResolveSingle 한 번에 한 명씩만 풉니다.
 * 실행은 실제 도착 이벤트로 다음 ResolveSingle을 부르고, 시각화는 SimulateChain으로 끝까지 펼칩니다.
 */
struct PROJECT_ENTROPY_API FPEPushResolver
{
	/** 대상이 밀려날 4방향. Radial은 중심에서 바깥으로, 중심에 정확히 선 대상만 시전자 반대편으로. */
	static FIntPoint ResolveDirection(EPEPushType Type, FIntPoint InstigatorPos, FIntPoint OriginPos, FIntPoint TargetPos);

	/** 범위에 걸린 대상들을 밀치기 요청으로 바꿉니다. 스킬을 모르므로 어떤 주체든 쓸 수 있습니다. */
	static TArray<FPEPushRequest> BuildRequests(const FPEPushField& Field, AActor* Instigator, FIntPoint OriginPos,
		const TSet<APE_CharacterBase*>& Targets, EPEPushType Type, int32 Distance, float CollisionDamageRatio);

	/**
	 * 요청 1건을 지금 전장에 대고 풉니다.
	 * Field를 읽기만 하고 수정하지 않으며, 연쇄를 만들지도 않습니다 (HitCharacter로 알려주기만 합니다).
	 */
	static FPEPushStep ResolveSingle(const FPEPushField& Field, const FPEPushRequest& Request);

	/** 충돌로 파생될 연쇄 요청. 밀 수 없는 대상이거나 남은 거리가 없으면 false. */
	static bool MakeChainedRequest(const FPEPushStep& Step, const FPEPushRequest& Origin, FPEPushRequest& OutNext);

	/** 진행 방향으로 더 멀리 나가 있는 쪽을 먼저 처리해야 앞이 비워집니다. */
	static void SortBackToFront(TArray<FPEPushRequest>& Requests, const FPEPushField& Field);

	/**
	 * 시각화 전용: 연쇄를 끝까지 펼쳐 예상 결과 전체를 반환합니다.
	 *
	 * 내부가 ResolveSingle + MakeChainedRequest 반복이므로 실행과 규칙이 갈라질 수 없습니다.
	 * 다만 아무도 죽지 않는다는 가정 위의 예측이므로, 연쇄 도중 사망이 일어나면 실제와 달라질 수 있습니다.
	 *
	 * Field에 가정이 누적되므로 여러 번 나눠 호출하면 앞 호출의 결과를 이어받습니다.
	 * 관통 투사체처럼 서버가 대상을 한 명씩 순차로 미는 경우를 그대로 재현할 때 필요합니다.
	 */
	static TArray<FPEPushStep> SimulateChain(FPEPushField& Field, TArray<FPEPushRequest> Requests);
};
