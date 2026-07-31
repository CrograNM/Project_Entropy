// Copyright CrograNM

#include "Core/PE_GameState.h"
#include "Core/PE_TurnManagerComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACSkillComponent.h"

APE_GameState::APE_GameState()
{
	// 턴 매니저 컴포넌트 생성 및 부착 (기존 GameMode에서 이관)
	TurnManager = CreateDefaultSubobject<UPE_TurnManagerComponent>(TEXT("TurnManager"));
}

void APE_GameState::EnqueueSkillAction(const FPESkillActionPayload& Payload)
{
	// 큐잉 시스템은 오직 서버에서만 동작합니다.
	if (!HasAuthority()) return;

	ActionQueue.Enqueue(Payload);

	// 현재 쏘고 있는 스킬이 아무것도 없다면 즉시 대기열 처리 시작
	if (!bIsProcessingAction)
	{
		ProcessNextAction();
	}
}

void APE_GameState::ProcessNextAction()
{
	if (ActionQueue.IsEmpty())
	{
		bIsProcessingAction = false;
		return;
	}

	bIsProcessingAction = true;

	FPESkillActionPayload Payload;
	ActionQueue.Dequeue(Payload);

	// 꺼낸 명세서를 바탕으로 스킬 실제 실행 지시
	if (Payload.Instigator && Payload.SkillData)
	{
		UACSkillComponent* SkillComp = Payload.Instigator->FindComponentByClass<UACSkillComponent>();
		if (SkillComp)
		{
			SkillComp->ExecuteQueuedSkill(Payload.SkillData, Payload.TargetTile, Payload.TargetCharacter, Payload.CalculatedDamage);
		}
		else CompleteCurrentAction(); // 에러 발생 시 큐가 막히지 않도록 강제 패스
	}
	else CompleteCurrentAction();
}

void APE_GameState::CompleteCurrentAction()
{
	if (!HasAuthority()) return;

	// 즉시 쏘지 않고, 기획된 0.2초의 간격을 두고 다음 스킬을 발사합니다.
	GetWorld()->GetTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_GameState::ProcessNextAction, ActionInterval, false);
}