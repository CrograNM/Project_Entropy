// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_SpawnActor.h"
#include "CardSystem/PE_SkillActionActor.h"
#include "CardSystem/PE_SkillData.h"
#include "Components/ACSkillComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

void UPE_SkillLogic_SpawnActor::ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!InSkillData || !InSkillData->SkillActorClass || !Instigator) return;

	// 1. 로컬에서 재생하던 이펙트를, 시전자의 컴포넌트를 찾아 멀티캐스트로 모두에게 뿌리도록 위임합니다.
	if (UACSkillComponent* SkillComp = Instigator->FindComponentByClass<UACSkillComponent>())
	{
		SkillComp->NetMulticast_PlayCastVisuals(InSkillData);
	}

	// 2. 실체 액터 스폰 (투사체면 내 위치, 장판이면 타겟 위치에 스폰할지 분기 가능)
	// 기본적으로는 내 위치에서 (정면에서 약간 떨어지게) 출발하도록 세팅합니다.
	FTransform SpawnTransform = Instigator->GetActorTransform();
	SpawnTransform.SetLocation(SpawnTransform.GetLocation() + SpawnTransform.GetRotation().Vector() * 70.0f); // 정면으로 조금 더 나아가도록 설정
	APE_SkillActionActor* ActionActor = GetWorld()->SpawnActor<APE_SkillActionActor>(InSkillData->SkillActorClass, SpawnTransform);
	if (ActionActor)
	{
		ActionActor->InitializeActionActor(this, Instigator, Target, TargetLocation, InSkillData, CalculatedDamage);
	}
}

void UPE_SkillLogic_SpawnActor::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!Instigator || !InSkillData) return;

	// 3. 적중 (Hit) 처리 (타겟 액터가 존재할 때만 데미지 적용)
	if (Target)
	{
		UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
	}

	// 폭발 이펙트는 Target 액터 여부와 상관없이 무조건 TargetLocation 좌표 기반으로 재생 명령을 내립니다.
	if (UACSkillComponent* SkillComp = Instigator->FindComponentByClass<UACSkillComponent>())
	{
		SkillComp->NetMulticast_PlayHitVisuals(InSkillData, TargetLocation);
	}
}