// Copyright CrograNM

#include "CardSystem/PE_SkillActionActor.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "CardSystem/PE_SkillData.h"
#include "NiagaraComponent.h"

APE_SkillActionActor::APE_SkillActionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 기본 충돌체는 구체(Sphere)로 설정하되, BP에서 모양 변경 가능
	USphereComponent* SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereCollision->InitSphereRadius(20.0f);
	SphereCollision->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &APE_SkillActionActor::OnOverlapBegin);

	CollisionComp = SphereCollision;
	RootComponent = CollisionComp;

	ActionVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ActionVFX"));
	ActionVFXComponent->SetupAttachment(RootComponent);
	ActionVFXComponent->SetAutoActivate(false);

	ActionSFXComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ActionSFX"));
	ActionSFXComponent->SetupAttachment(RootComponent);
	ActionSFXComponent->SetAutoActivate(false);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComp"));
	MovementComponent->InitialSpeed = 800.f;
	MovementComponent->MaxSpeed = 800.f;
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->ProjectileGravityScale = 0.f;
}

void APE_SkillActionActor::InitializeActionActor(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage)
{
	SkillLogicInstance = InLogic;
	Caster = InInstigator;
	TargetActor = InTarget;
	SkillData = InData;
	DamageToApply = InDamage;

	// 2단계 (Action): 진행 중 이펙트 및 사운드 활성화
	if (SkillData)
	{
		if (SkillData->ActionVFX)
		{
			ActionVFXComponent->SetAsset(SkillData->ActionVFX);
			ActionVFXComponent->Activate();
		}
		if (SkillData->ActionSFX)
		{
			ActionSFXComponent->SetSound(SkillData->ActionSFX);
			ActionSFXComponent->Play();
		}
	}

	// Movement 세팅 (속도가 0인 장판류는 이동하지 않음)
	if (MovementComponent->InitialSpeed > 0.f)
	{
		if (TargetActor)
		{
			MovementComponent->bIsHomingProjectile = true;
			MovementComponent->HomingTargetComponent = TargetActor->GetRootComponent();
			MovementComponent->HomingAccelerationMagnitude = 2000.f;
		}
		else
		{
			FVector Direction = (InLoc - GetActorLocation()).GetSafeNormal();
			MovementComponent->Velocity = Direction * MovementComponent->InitialSpeed;
		}
	}
}

void APE_SkillActionActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != Caster && OtherActor != this)
	{
		// 3단계 (Hit) 발동 지시
		if (SkillLogicInstance)
		{
			SkillLogicInstance->ApplySkillEffect(Caster, OtherActor, GetActorLocation(), SkillData, DamageToApply);
		}

		// 투사체면 바로 파괴, 장판이면 계속 유지
		if (bDestroyOnHit)
		{
			Destroy();
		}
	}
}