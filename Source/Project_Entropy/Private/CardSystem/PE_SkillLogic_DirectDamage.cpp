// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_DirectDamage.h"
#include "CardSystem/PE_SkillData.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

void UPE_SkillLogic_DirectDamage::ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	// 1. 데이터 및 타겟 유효성 검사
	if (!InSkillData || !Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillLogic] 데이터가 없거나 타겟이 지정되지 않았습니다."));
		return;
	}

	// 2. 데미지 적용 (언리얼 기본 데미지 시스템 활용 또는 커스텀 스탯 컴포넌트 호출)
	UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());

	UE_LOG(LogTemp, Warning, TEXT("[%s]가 [%s]에게 %f의 데미지를 입혔습니다! (스킬: %s)"),
		*Instigator->GetName(), *Target->GetName(), CalculatedDamage, *InSkillData->SkillID.ToString());

	// 3. 시각 효과 (VFX) 재생
	if (InSkillData->VFX)
	{
		// 타겟의 위치에 파티클 생성
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), InSkillData->VFX, Target->GetActorLocation());
	}

	// 4. 사운드 (SFX) 재생
	if (InSkillData->SFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), InSkillData->SFX, Target->GetActorLocation());
	}
}