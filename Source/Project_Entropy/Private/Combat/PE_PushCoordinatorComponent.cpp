// Copyright CrograNM

#include "Combat/PE_PushCoordinatorComponent.h"
#include "Combat/PE_PushResolver.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Core/PE_GameState.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

UPE_PushCoordinatorComponent::UPE_PushCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

AACGridSystem* UPE_PushCoordinatorComponent::GetGrid()
{
	if (!CachedGrid)
	{
		CachedGrid = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
	}
	return CachedGrid;
}

APE_GameState* UPE_PushCoordinatorComponent::GetGameState() const
{
	return Cast<APE_GameState>(GetOwner());
}

void UPE_PushCoordinatorComponent::EnqueuePush(TArray<FPEPushRequest> Requests, const FString& Context)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	if (Requests.IsEmpty()) return;

	const int32 ChainID = ++NextChainID;

	FPEPushChain& Chain = ActiveChains.Add(ChainID);
	Chain.ChainID = ChainID;
	Chain.Pending = MoveTemp(Requests);

	// 연쇄 전체가 토큰 하나를 쥡니다. 개별 밀치기는 UI 로그만 따로 남깁니다.
	if (APE_GameState* GS = GetGameState())
	{
		Chain.ActionTokenID = GS->BeginAction(FString::Printf(TEXT("PushChain:%s"), *Context));
	}

	Drain();
}

void UPE_PushCoordinatorComponent::Drain()
{
	// 경로가 빈 밀치기는 MoveAlongPath 안에서 도착 보고가 즉시 되돌아옵니다.
	// 그때의 재진입은 여기서 흡수하고, 아래 루프가 이어서 처리합니다.
	if (bIsDraining)
	{
		bDrainRequested = true;
		return;
	}

	bIsDraining = true;

	do
	{
		bDrainRequested = false;

		// 전개 도중 연쇄가 추가/종료될 수 있으므로 키를 먼저 떠 두고 매번 다시 확인합니다.
		TArray<int32> ChainIDs;
		ActiveChains.GenerateKeyArray(ChainIDs);

		for (int32 ChainID : ChainIDs)
		{
			const FPEPushChain* Chain = ActiveChains.Find(ChainID);
			if (!Chain || Chain->Pending.IsEmpty()) continue;

			DispatchGeneration(ChainID);
			bDrainRequested = true;
		}
	} while (bDrainRequested);

	bIsDraining = false;

	CloseSettledChains();
}

void UPE_PushCoordinatorComponent::DispatchGeneration(int32 ChainID)
{
	AACGridSystem* Grid = GetGrid();
	if (!Grid) return;

	TArray<FPEPushRequest> Generation;
	{
		FPEPushChain* Chain = ActiveChains.Find(ChainID);
		if (!Chain) return;

		// 이번 세대는 지금 대기 중인 것만입니다. 실행 도중 새로 들어온 요청은 다음 세대로 넘어갑니다.
		Generation = MoveTemp(Chain->Pending);
		Chain->Pending.Reset();
	}

	/*
		가정을 얹지 않은 Field, 즉 그리드를 그대로 읽습니다.
		앞 요청이 출발하면서 목적지를 즉시 점유하므로(UACGridMovementComponent::ProcessNextCommand),
		같은 세대의 뒤 요청은 그 결과가 이미 반영된 전장을 보게 됩니다.
		매 세대 새로 읽으므로 그 사이의 사망이나 지형 변화도 자연히 반영됩니다.
	*/
	FPEPushField Field(Grid);
	FPEPushResolver::SortBackToFront(Generation, Field);

	for (const FPEPushRequest& Request : Generation)
	{
		const FPEPushStep Step = FPEPushResolver::ResolveSingle(Field, Request);
		ExecuteStep(ChainID, Request, Step);
	}
}

void UPE_PushCoordinatorComponent::ExecuteStep(int32 ChainID, const FPEPushRequest& Request, const FPEPushStep& Step)
{
	APE_CharacterBase* Target = Step.Target;
	UACGridMovementComponent* MoveComp = Target ? Target->GetGridMovementComponent() : nullptr;

	// 실행할 수 없는 요청은 여기서 버립니다. InFlight에 잡히지 않으므로 연쇄를 멈추지 않습니다.
	if (!MoveComp) return;

	APE_GameState* GS = GetGameState();

	FGridKnockbackPayload Payload;
	Payload.bIsActive = true;
	Payload.Instigator = Request.Instigator;
	Payload.ChainID = ChainID;

	// 충돌 피해는 '남아있던 밀림 거리' 비율만큼만 들어갑니다 (덜 밀렸으면 덜 아픔).
	if (Step.bBlocked && Request.BaseDistance > 0)
	{
		const float DamageRatio = Request.CollisionDamageRatio * ((float)Request.Distance / (float)Request.BaseDistance);

		if (Step.HitCharacter)
		{
			if (UACStatComponent* HitStat = Step.HitCharacter->GetStatComponent())
			{
				Payload.TargetDamage = HitStat->GetMaxHP() * DamageRatio;
				Payload.OtherDamage = Payload.TargetDamage;
				Payload.HitCharacter = Step.HitCharacter;
			}
		}
		else if (UACStatComponent* MyStat = Target->GetStatComponent())
		{
			// 벽에 부딪힌 경우는 본인만 피해를 입습니다.
			Payload.TargetDamage = MyStat->GetMaxHP() * DamageRatio;
		}
	}

	FPEPushInFlight Entry;
	Entry.Target = Target;

	if (GS)
	{
		Entry.ActionLogID = GS->AddActionLog(Target->GetTeamID(),
			FString::Printf(TEXT("%s - %d칸 밀림"), *Target->GetName(), Step.Path.Num()));
	}

	// 연쇄는 지금 만들어 두기만 하고, 이 대상이 실제로 멈출 때 투입합니다.
	Entry.bHasChained = FPEPushResolver::MakeChainedRequest(Step, Request, Entry.Chained);

	MoveComp->OnKnockbackSettled.AddUniqueDynamic(this, &UPE_PushCoordinatorComponent::HandleKnockbackSettled);

	{
		// 경로가 비면 아래 MoveAlongPath 안에서 도착 보고가 즉시 되돌아오므로 반드시 먼저 등록해야 합니다.
		FPEPushChain* Chain = ActiveChains.Find(ChainID);
		if (!Chain) return;

		Chain->InFlight.Add(Entry);
	}

	MoveComp->NetMulticast_MoveAlongPath(Step.Path, false, Payload);
}

void UPE_PushCoordinatorComponent::HandleKnockbackSettled(APE_CharacterBase* Mover, int32 ChainID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	FPEPushChain* Chain = ActiveChains.Find(ChainID);
	if (!Chain) return;

	const int32 Index = Chain->InFlight.IndexOfByPredicate(
		[Mover](const FPEPushInFlight& Entry) { return Entry.Target == Mover; });
	if (Index == INDEX_NONE) return;

	const FPEPushInFlight Entry = Chain->InFlight[Index];
	Chain->InFlight.RemoveAt(Index);

	if (APE_GameState* GS = GetGameState())
	{
		GS->RemoveActionLog(Entry.ActionLogID);
	}

	// 앞 캐릭터가 '실제로 닿은' 지금에서야 연쇄를 투입합니다. 예측 지연을 쓰지 않는 이유입니다.
	if (Entry.bHasChained)
	{
		Chain->Pending.Add(Entry.Chained);
	}

	Drain();
}

void UPE_PushCoordinatorComponent::CloseSettledChains()
{
	TArray<int32> ChainIDs;
	ActiveChains.GenerateKeyArray(ChainIDs);

	for (int32 ChainID : ChainIDs)
	{
		const FPEPushChain* Chain = ActiveChains.Find(ChainID);
		if (!Chain) continue;
		if (!Chain->Pending.IsEmpty() || !Chain->InFlight.IsEmpty()) continue;

		const int32 TokenID = Chain->ActionTokenID;

		// 토큰 반납이 다음 스킬을 당겨올 수 있으므로 연쇄를 먼저 지운 뒤 호출합니다.
		ActiveChains.Remove(ChainID);

		if (APE_GameState* GS = GetGameState())
		{
			GS->EndAction(TokenID);
		}
	}
}
