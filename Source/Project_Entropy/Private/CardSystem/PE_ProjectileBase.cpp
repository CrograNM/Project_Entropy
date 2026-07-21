// Copyright CrograNM

#include "CardSystem/PE_ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "CardSystem/PE_SkillData.h"

APE_ProjectileBase::APE_ProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 충돌체 셋업 (모든 것과 겹침 허용)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(15.0f);
	CollisionComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APE_ProjectileBase::OnOverlapBegin);
	RootComponent = CollisionComp;

	// 투사체 이동 컴포넌트 셋업
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 800.f;
	ProjectileMovement->MaxSpeed = 800.f;
	ProjectileMovement->bRotationFollowsVelocity = true; // 날아가는 방향으로 회전
	ProjectileMovement->ProjectileGravityScale = 0.f;    // 중력 무시 (직선 비행)
}

void APE_ProjectileBase::InitializeProjectile(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage)
{
	SkillLogicInstance = InLogic;
	Caster = InInstigator;
	TargetActor = InTarget;
	SkillData = InData;
	DamageToApply = InDamage;

	// 대상이 명확한 경우 유도탄(Homing) 기능 활성화
	if (TargetActor)
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = 2000.f;
	}
	else
	{
		// 타일(좌표) 공격일 경우 해당 좌표를 향해 날아감
		FVector Direction = (InLoc - GetActorLocation()).GetSafeNormal();
		ProjectileMovement->Velocity = Direction * ProjectileMovement->InitialSpeed;
	}
}

void APE_ProjectileBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 자기 자신이나 시전자(Caster)가 아닌 적(OtherActor)을 맞췄다면
	if (OtherActor && OtherActor != Caster && OtherActor != this)
	{
		// 🌟 도착했으니 로직의 '적중(Apply)' 이벤트를 호출하여 데미지를 입힙니다.
		if (SkillLogicInstance)
		{
			SkillLogicInstance->ApplySkillEffect(Caster, OtherActor, GetActorLocation(), SkillData, DamageToApply);
		}

		// 충돌했으므로 투사체는 파괴됩니다.
		Destroy();
	}
}