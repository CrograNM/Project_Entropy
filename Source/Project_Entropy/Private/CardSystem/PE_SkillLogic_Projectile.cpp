// Copyright CrograNM

#include "CardSystem/PE_SkillLogic_Projectile.h"
#include "CardSystem/PE_ProjectileBase.h"
#include "CardSystem/PE_SkillData.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

void UPE_SkillLogic_Projectile::ExecuteSkill_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!ProjectileClass || !Instigator) return;

	// 시전자의 가슴팍이나 지팡이 끝부분에서 스폰되도록 조절 (현재는 캐릭터 중심)
	FTransform SpawnTransform = Instigator->GetActorTransform();

	APE_ProjectileBase* Proj = GetWorld()->SpawnActor<APE_ProjectileBase>(ProjectileClass, SpawnTransform);
	if (Proj)
	{
		// 스폰된 투사체에게 정보 주입 및 출발 지시
		Proj->InitializeProjectile(this, Instigator, Target, TargetLocation, InSkillData, CalculatedDamage);
	}

	// (선택) 여기서 시전 사운드 재생 가능
}

void UPE_SkillLogic_Projectile::ApplySkillEffect_Implementation(AActor* Instigator, AActor* Target, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (!Target || !Instigator) return;

	// 적중 시 데미지 부여
	UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());

	UE_LOG(LogTemp, Warning, TEXT("투사체 적중! [%s]에게 %f의 데미지!"), *Target->GetName(), CalculatedDamage);

	// 적중 폭발 VFX/SFX 재생
	if (InSkillData && InSkillData->VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), InSkillData->VFX, Target->GetActorLocation());
	}
	if (InSkillData && InSkillData->SFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), InSkillData->SFX, Target->GetActorLocation());
	}
}