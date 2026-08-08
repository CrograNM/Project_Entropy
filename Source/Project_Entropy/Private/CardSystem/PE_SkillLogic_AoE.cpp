// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_AoE.h"
#include "CardSystem/PE_SkillData.h"
#include "Components/ACSkillComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Kismet/GameplayStatics.h"

void UPE_SkillLogic_AoE::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!Instigator || !InSkillData) return;

	// 1. 맵 위의 모든 캐릭터를 찾습니다.
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	// 2. 폭발 중심(TargetLocation)으로부터 반경 내에 있는 모든 대상에게 피해를 입힙니다.
	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (FVector::Distance(TargetLocation, Char->GetActorLocation()) <= SplashRadius)
			{
				UGameplayStatics::ApplyDamage(Char, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
			}
		}
	}

	// 3. 폭발 시각 효과(VFX/SFX)는 중심점에서 한 번만 터트립니다.
	if (UACSkillComponent* SkillComp = Instigator->FindComponentByClass<UACSkillComponent>())
	{
		SkillComp->NetMulticast_PlayHitVisuals(InSkillData, TargetLocation);
	}
}