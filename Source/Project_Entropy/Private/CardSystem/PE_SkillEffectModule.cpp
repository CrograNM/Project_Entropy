// Copyright CrograNM

#include "CardSystem/PE_SkillEffectModule.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Combat/PE_PushCoordinatorComponent.h"
#include "Combat/PE_PushResolver.h"
#include "Components/ACGridMovementComponent.h"
#include "Core/PE_GameState.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

// --- [모듈 1: 데미지 구현부] ---
void UPE_SkillEffect_Damage::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	for (APE_CharacterBase* Target : Targets)
	{
		if (Target && Instigator)
		{
			UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
		}
	}
}

// --- [모듈 2: 넉백 어댑터] ---
TArray<FPEPushRequest> UPE_SkillEffect_Push::BuildPushRequests(const FPEPushField& Field, AActor* Instigator, FIntPoint TargetGridPos, const TSet<APE_CharacterBase*>& Targets) const
{
	return FPEPushResolver::BuildRequests(Field, Instigator, TargetGridPos, Targets, PushType, PushDistance, CollisionDamageRatio);
}

void UPE_SkillEffect_Push::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, FIntPoint TargetGridPos, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	APE_CharacterBase* InstigatorChar = Cast<APE_CharacterBase>(Instigator);
	if (!InstigatorChar) return;

	UACGridMovementComponent* InstMove = InstigatorChar->GetGridMovementComponent();
	AACGridSystem* GridSystem = InstMove ? InstMove->GetCachedGridSystem() : nullptr;
	if (!GridSystem) return;

	APE_GameState* GS = Instigator->GetWorld() ? Instigator->GetWorld()->GetGameState<APE_GameState>() : nullptr;
	UPE_PushCoordinatorComponent* Coordinator = GS ? GS->GetPushCoordinator() : nullptr;
	if (!Coordinator) return;

	/*
		규칙도 실행도 소유하지 않습니다. 요청을 만들어 넘기는 것이 전부입니다.
		연쇄 전개, 도착 타이밍, 충돌 피해, 액션 토큰은 모두 코디네이터의 책임입니다.
	*/
	const FPEPushField Field(GridSystem);

	Coordinator->EnqueuePush(
		BuildPushRequests(Field, Instigator, TargetGridPos, Targets),
		InSkillData ? InSkillData->SkillID.ToString() : FString(TEXT("Unknown")));
}
