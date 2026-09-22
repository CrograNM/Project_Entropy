// Copyright CrograNM

#include "Components/PE_CardCastComponent.h"
#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Combat/PE_TargetRules.h"
#include "Components/ACCardInteractionComponent.h"
#include "Components/ACDeckManagerComponent.h"
#include "Components/ACSkillComponent.h"
#include "Components/ACStatComponent.h"
#include "Components/ACTargetingVisualizerComponent.h"
#include "Core/PE_GameState.h"
#include "Core/PE_PlayerController.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"

UPE_CardCastComponent::UPE_CardCastComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPE_CardCastComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPC = Cast<APE_PlayerController>(GetOwner());
}

// ----- [시전 개시] -----

void UPE_CardCastComponent::TryExecuteCardDrop(APE_CardActor* DroppedCard)
{
	if (!OwnerPC || !DroppedCard || !DroppedCard->GetSkillData()) return;

	UACCardInteractionComponent* Interaction = OwnerPC->GetCardInteraction();
	if (!Interaction) return;

	UPE_SkillData* SkillData = DroppedCard->GetSkillData();
	UACDeckManagerComponent* DeckManager = OwnerPC->GetDeckManager();
	APE_PlayerCharacter* Caster = OwnerPC->GetCachedPlayerCharacter();

	if (!Caster || !Caster->GetTargetingVisualizer() || !Caster->GetStatComponent())
	{
		Interaction->CancelCasting();
		return;
	}

	if (Caster->GetStatComponent()->GetCurrentAP() < SkillData->BaseAPCost)
	{
		OwnerPC->ShowToastMessage(FText::FromString(TEXT("AP가 부족합니다.")));
		Interaction->CancelCasting();
		return;
	}

	// 지정이 필요 없는 스킬(Self / All_Enemies)은 타겟 판정 없이 그대로 보냅니다.
	if (!FPETargetRules::RequiresTarget(SkillData->TargetType))
	{
		if (DeckManager) DeckManager->QueueCard(DroppedCard);
		SendCardCastRequest(SkillData, nullptr, nullptr, DroppedCard);

		Caster->GetTargetingVisualizer()->ClearTargeting();
		Interaction->CompleteCasting();
		return;
	}

	AACTile* TargetTile = OwnerPC->GetTileUnderCursor();
	if (!TargetTile)
	{
		OwnerPC->ShowToastMessage(FPETargetRules::GetFailureText(EPETargetResult::NoTileSpecified));
		Interaction->CancelCasting();
		return;
	}

	// 조준 중 커서가 통과한 것과 같은 판정입니다. 여기서 갈라질 수 없습니다.
	const FPETargetCandidate Candidate = FPETargetRules::Evaluate(
		Caster->GetTargetingVisualizer()->GetTargetContext(), TargetTile->GetGridPosition());

	if (Candidate.Result != EPETargetResult::Valid)
	{
		OwnerPC->ShowToastMessage(FPETargetRules::GetFailureText(Candidate.Result));
		Interaction->CancelCasting();
		return;
	}

	if (DeckManager) DeckManager->QueueCard(DroppedCard);

	// Tile 대상 스킬은 Character가 비어 옵니다. 서버도 Tile 분기에서 TargetCharacter를 읽지 않습니다.
	SendCardCastRequest(SkillData, Candidate.Tile, Candidate.Character, DroppedCard);

	Caster->GetTargetingVisualizer()->ClearTargeting();
	Interaction->CompleteCasting();
}

void UPE_CardCastComponent::SendCardCastRequest(UPE_SkillData* SkillData, AACTile* TargetTile,
	APE_CharacterBase* TargetCharacter, APE_CardActor* SourceCard, bool bIsFreeCast)
{
	// 고유 번호 발급 및 카드 매핑 저장
	const int32 RequestID = ++NextCastRequestID;

	if (SourceCard)
	{
		FPECardCastRequest& Entry = PendingRequests.Add(RequestID);
		Entry.Card = SourceCard;
		Entry.bIsFreeCast = bIsFreeCast;
	}

	// 네트워크를 넘을 수 있는 ID만 서버로 전송
	Server_RequestCardCast(SkillData, TargetTile, TargetCharacter, RequestID, bIsFreeCast);
}

void UPE_CardCastComponent::ForceTriggerCardLocally(APE_CardActor* TriggeredCard)
{
	if (!OwnerPC || !TriggeredCard || !TriggeredCard->GetSkillData()) return;

	UACDeckManagerComponent* DeckManager = OwnerPC->GetDeckManager();
	if (!DeckManager) return;

	UPE_CardInstance* CardInst = TriggeredCard->GetCardInstance();
	UPE_CardData* BaseData = CardInst ? CardInst->GetBaseCardData() : nullptr;
	UPE_SkillData* SkillData = TriggeredCard->GetSkillData();

	if (!OwnerPC->GetCachedPlayerCharacter() || !BaseData) return;

	AACTile* TargetTile = nullptr;
	APE_CharacterBase* TargetCharacter = nullptr;
	const bool bHasTarget = PickRandomValidTarget(SkillData, TargetTile, TargetCharacter);

	// 시전 대기 연출과 연산이 모두 끝났으므로 캐스팅 카드 등록을 해제합니다.
	// (이후 QueueCard 또는 DiscardCard 내부에서 안전하게 HandCards 배열에서 카드를 완전히 제거합니다)
	DeckManager->SetCastingCard(nullptr);

	if (bHasTarget)
	{
		DeckManager->QueueCard(TriggeredCard);
		SendCardCastRequest(SkillData, TargetTile, TargetCharacter, TriggeredCard, true);

		OwnerPC->ShowToastMessage(FText::FromString(FString::Printf(TEXT("%s 자동 발동!"), *BaseData->CardName.ToString())));
	}
	else
	{
		OwnerPC->ShowToastMessage(FText::FromString(FString::Printf(TEXT("%s 발동 실패: 대상 없음"), *BaseData->CardName.ToString())));
		DeckManager->DiscardCard(TriggeredCard);

		// 실패한 카드를 버릴 때 다음 탐색을 무작정 시작하지 않고 대기하기 위한 캐싱 작업
		FailedTurnEndCard = TriggeredCard;
		TriggeredCard->PlayDiscardAnimation();
	}
}

bool UPE_CardCastComponent::PickRandomValidTarget(UPE_SkillData* SkillData, AACTile*& OutTile, APE_CharacterBase*& OutChar)
{
	OutTile = nullptr;
	OutChar = nullptr;

	if (!OwnerPC || !SkillData) return false;

	APE_PlayerCharacter* Caster = OwnerPC->GetCachedPlayerCharacter();
	const AACGridSystem* Grid = OwnerPC->GetGridSystem();
	if (!Caster || !Grid) return false;

	// 지정이 필요 없는 스킬(Self / All_Enemies)은 대상 없이 그대로 발동합니다.
	if (!FPETargetRules::RequiresTarget(SkillData->TargetType)) return true;

	// BaseRange: 마석/버프로 사거리가 변동되면(P1-1) 이 인자만 최종 사거리로 바꾸면 됩니다.
	const FPETargetContext Context = FPETargetRules::MakeContext(Grid, Caster, SkillData->TargetType, SkillData->BaseRange);

	const TArray<FPETargetCandidate> Candidates = FPETargetRules::CollectValidTargets(Context);
	if (Candidates.IsEmpty()) return false;

	/*
		여기서 런 시드(UPE_RunManagerSubsystem)를 쓰면 안 됩니다.
		이 함수는 클라이언트에서 돌고 런 시드 스트림은 클라마다 별개이므로,
		여기서 스트림을 전진시키면 클라마다 시드 위치가 어긋나 시드런 재현성이 깨집니다.
	*/
	const FPETargetCandidate& Picked = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

	OutTile = Picked.Tile;
	OutChar = Picked.Character;
	return true;
}

// ----- [카드 연출 종료 통보] -----

void UPE_CardCastComponent::NotifyDiscardAnimFinished(APE_CardActor* Card)
{
	// 대상 지정에 실패한 턴 종료 카드의 산화 애니메이션이 끝난 시점을 감지합니다.
	if (FailedTurnEndCard == Card)
	{
		FailedTurnEndCard = nullptr;

		// 겹침 현상 없이 부드럽게 폐기가 완료된 직후, 그제서야 다음 카드를 찾도록 지시
		FTimerHandle DelayTimer;
		GetWorld()->GetTimerManager().SetTimer(DelayTimer, this, &UPE_CardCastComponent::Client_TriggerTurnEndCards, 0.2f, false);
		return;
	}

	for (const TPair<int32, FPECardCastRequest>& Entry : PendingRequests)
	{
		if (Entry.Value.Card == Card)
		{
			Server_NotifyCardCastAnimFinished(Entry.Key);
			return;
		}
	}
}

void UPE_CardCastComponent::NotifyTurnEndReadyAnimFinished(APE_CardActor* Card)
{
	// 브로드캐스트 도중 델리게이트를 지우려다 엔진이 꺼지는 증상(Ensure Failed)을 해결하기 위한 명시적 함수 연결
	if (PendingTurnEndCard == Card)
	{
		ForceTriggerCardLocally(PendingTurnEndCard);
		PendingTurnEndCard = nullptr;
	}
}

// ----- [서버 -> 소유 클라이언트] -----

void UPE_CardCastComponent::Client_PlayCardCastAnim_Implementation(int32 CastRequestID)
{
	bool bAnimStarted = false;

	if (const FPECardCastRequest* Request = PendingRequests.Find(CastRequestID))
	{
		APE_CardActor* Card = Request->Card;

		if (Card && Card->GetSkillData())
		{
			const EPESkillTargetType TargetType = Card->GetSkillData()->TargetType;

			// 조준 없이 나가는 시전은 손에서 바로 타오르고, 조준했던 카드는 산화 연출을 탑니다.
			if (Request->bIsFreeCast || !FPETargetRules::RequiresTarget(TargetType))
			{
				Card->PlayInstantCastingAnimation();
			}
			else
			{
				Card->PlayDiscardAnimation();
			}

			bAnimStarted = true;
		}
	}

	// 카드를 못 찾았거나 데이터가 비어있어 애니메이션 재생을 시작하지 못했다면 즉시 완료 처리 (무한 대기 방지)
	if (!bAnimStarted)
	{
		Server_NotifyCardCastAnimFinished(CastRequestID);
	}
}

void UPE_CardCastComponent::Client_ConfirmCardCast_Implementation(int32 CastRequestID)
{
	if (const FPECardCastRequest* Request = PendingRequests.Find(CastRequestID))
	{
		if (UACDeckManagerComponent* DeckManager = OwnerPC ? OwnerPC->GetDeckManager() : nullptr)
		{
			DeckManager->ConfirmQueuedCard(Request->Card);
		}
		PendingRequests.Remove(CastRequestID);
	}
}

void UPE_CardCastComponent::Client_CancelCardCast_Implementation(int32 CastRequestID)
{
	if (const FPECardCastRequest* Request = PendingRequests.Find(CastRequestID))
	{
		if (UACDeckManagerComponent* DeckManager = OwnerPC ? OwnerPC->GetDeckManager() : nullptr)
		{
			DeckManager->RevertQueuedCard(Request->Card);
			DeckManager->UpdateHandLayout();
		}

		if (OwnerPC) OwnerPC->ShowToastMessage(FText::FromString(TEXT("시전 취소: 검증 실패")));

		PendingRequests.Remove(CastRequestID);
	}
}

void UPE_CardCastComponent::Client_TriggerTurnEndCards_Implementation()
{
	UACDeckManagerComponent* DeckManager = OwnerPC ? OwnerPC->GetDeckManager() : nullptr;

	if (DeckManager)
	{
		// 순회 도중 손패가 바뀔 수 있으므로 사본을 뜹니다.
		TArray<APE_CardActor*> HandCardsCopy;
		for (const TObjectPtr<APE_CardActor>& CardObj : DeckManager->GetHandCards()) HandCardsCopy.Add(CardObj);

		for (APE_CardActor* Card : HandCardsCopy)
		{
			UPE_CardInstance* CardInst = Card ? Card->GetCardInstance() : nullptr;
			UPE_CardData* BaseData = CardInst ? CardInst->GetBaseCardData() : nullptr;

			if (BaseData && BaseData->TriggerType == EPECardTriggerType::OnTurnEnd)
			{
				PendingTurnEndCard = Card;

				/*
					강제 발동 연출 중 손패 정렬(UpdateHandLayout)의 간섭을 막기 위해
					일시적으로 시전 카드(CastingCard)로 등록하고 C++ 기반의 물리 이동을 즉시 중단시킵니다.
				*/
				DeckManager->SetCastingCard(Card);
				Card->CancelMoveToTarget();

				Card->PlayInstantCastingReadyAnimation();
				return;
			}
		}
	}

	Server_TurnEndCardsFinished();
}

// ----- [클라이언트 -> 서버] -----

bool UPE_CardCastComponent::Server_RequestCardCast_Validate(UPE_SkillData* SkillData, AACTile* TargetTile,
	APE_CharacterBase* TargetCharacter, int32 CastRequestID, bool bIsFreeCast)
{
	return SkillData != nullptr;
}

void UPE_CardCastComponent::Server_RequestCardCast_Implementation(UPE_SkillData* SkillData, AACTile* TargetTile,
	APE_CharacterBase* TargetCharacter, int32 CastRequestID, bool bIsFreeCast)
{
	APE_PlayerCharacter* Caster = OwnerPC ? OwnerPC->GetCachedPlayerCharacter() : nullptr;
	if (!Caster) return;

	UACSkillComponent* SkillComp = Caster->FindComponentByClass<UACSkillComponent>();
	if (!SkillComp) return;

	// 타겟 검증은 UACSkillComponent가 FPETargetRules로 다시 수행합니다. 클라의 판정을 믿지 않습니다.
	if (!SkillComp->TryExecuteSkillByData(SkillData, TargetTile, TargetCharacter, SkillData->BaseDamage, CastRequestID, bIsFreeCast))
	{
		Client_CancelCardCast(CastRequestID);
	}
}

void UPE_CardCastComponent::Server_NotifyCardCastAnimFinished_Implementation(int32 CastRequestID)
{
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		GS->CommitCurrentAction();
	}
}

void UPE_CardCastComponent::Server_TurnEndCardsFinished_Implementation()
{
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		GS->ReportTurnEndCardsFinished(OwnerPC);
	}
}
