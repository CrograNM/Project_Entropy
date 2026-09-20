// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Grid/PE_GridMath.h"
#include "PE_PushTypes.generated.h"

class APE_CharacterBase;
class AACTile;

/*
	EPEPushType - 방사형 / 지향성
*/	
UENUM(BlueprintType)
enum class EPEPushType : uint8
{
	Radial		UMETA(DisplayName = "방사형 (Radial - 폭발)"),
	Directional UMETA(DisplayName = "지향성 (Directional - 파도/바람)")
};

/*
	EPEPushCause - 밀치기가 발생한 '원인'
	
	스킬 외의 주체가 늘어나도 로그와 피해 규칙을 구분 가능하게
*/
UENUM()
enum class EPEPushCause : uint8
{
	Skill,
	Collision,	// 밀려난 캐릭터가 다른 캐릭터를 들이받아 파생된 연쇄
	Environment
};

/*
	FPEPushRequest - 밀치기 1건의 '요청'

	[ 누가 / 누구에게 / 어느 방향으로 / 몇 칸 ]의 요청을 담고 결과는 모름
	스킬을 전혀 참조하지 않음 --> 어떤 주체든 같은 요청을 만들어 넣을 수 있음
*/
USTRUCT()
struct PROJECT_ENTROPY_API FPEPushRequest
{
	GENERATED_BODY()

	UPROPERTY() APE_CharacterBase* Target = nullptr;
	UPROPERTY() AActor* Instigator = nullptr;

	// 4방향 단위 벡터 (FPEGridMath::SnapToCardinal을 통과한 값)
	UPROPERTY() FIntPoint Direction = FIntPoint::ZeroValue;

	// 이번 요청으로 밀려날 칸 수
	UPROPERTY() int32 Distance = 0;

	// 충돌 피해 비율의 분모 (연쇄로 이어져도 '최초' 거리를 그대로 물려받음)
	UPROPERTY() int32 BaseDistance = 0;

	// 충돌 피해량 (대상 또는 본인의 최대 체력 대비 입을 피해량, 0.2 = 20%)
	UPROPERTY() float CollisionDamageRatio = 0.f;

	// 밀치기 발생 원인
	UPROPERTY() EPEPushCause Cause = EPEPushCause::Skill;
};

/*
	FPEPushStep - 요청 1건을 '지금 전장'에 대고 푼 결과

	실행		(UPE_PushCoordinatorComponent)
	시각화	(UACTargetingVisualizerComponent)
	
	실행, 시각화 모두 이것만 소비하므로 "보이는 밀림"과 "실제 밀림"이 갈라질 수 없음
*/
USTRUCT()
struct PROJECT_ENTROPY_API FPEPushStep
{
	GENERATED_BODY()

	UPROPERTY() APE_CharacterBase* Target = nullptr;

	UPROPERTY() FIntPoint StartPos = FIntPoint(PEGridMath::InvalidCoord, PEGridMath::InvalidCoord);
	UPROPERTY() FIntPoint EndPos = FIntPoint(PEGridMath::InvalidCoord, PEGridMath::InvalidCoord);
	UPROPERTY() FIntPoint PushDir = FIntPoint::ZeroValue;

	// 실제로 지나갈 타일 목록 (이동 연출용). 즉시 막혔다면 비어 있습니다.
	UPROPERTY() TArray<AACTile*> Path;

	// 이 밀치기가 시작될 때 남아있던 밀림 거리. 충돌 피해 비율 산출에 씁니다.
	UPROPERTY() int32 RemainingDist = 0;

	// 다른 캐릭터에 부딪혀 멈췄다면 그 대상 (벽/장애물에 막혔거나 안 막혔으면 null)
	UPROPERTY() APE_CharacterBase* HitCharacter = nullptr;

	// 목표 거리를 다 못 가고 무언가에 막혀 멈췄는지 여부
	UPROPERTY() bool bBlocked = false;
};
