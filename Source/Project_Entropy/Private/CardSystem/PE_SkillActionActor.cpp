// Copyright CrograNM

#include "CardSystem/PE_SkillActionActor.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "CardSystem/PE_SkillData.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"

APE_SkillActionActor::APE_SkillActionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

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

void APE_SkillActionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APE_SkillActionActor, RepSkillData);
	DOREPLIFETIME(APE_SkillActionActor, RepTargetActor);
}

void APE_SkillActionActor::InitializeActionActor(UPE_SkillLogicBase* InLogic, AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage)
{
	// 이 함수는 서버에서만 호출됩니다.
	SkillLogicInstance = InLogic;
	Caster = InInstigator;
	DamageToApply = InDamage;

	// 복제용 변수에 담아 클라이언트에게 전송 유도
	RepTargetActor = InTarget;
	RepSkillData = InData;

	// 서버도 시각적 효과를 봐야 하므로 OnRep 함수를 수동 호출해 줍니다.
	OnRep_SkillData();

	// 투사체 이동 로직은 복제되지만, 초기 속도 부여는 서버가 주도합니다.
	if (RepSkillData && MovementComponent && RepSkillData->ProjectileSpeed > 0.f)
	{
		if (!RepSkillData->bIsHoming)
		{
			FVector Direction = (InLoc - GetActorLocation()).GetSafeNormal();
			MovementComponent->Velocity = Direction * MovementComponent->InitialSpeed;
		}
	}
}

void APE_SkillActionActor::OnRep_SkillData()
{
	if (RepSkillData)
	{
		if (RepSkillData->ActionVFX)
		{
			ActionVFXComponent->SetAsset(RepSkillData->ActionVFX);
			ActionVFXComponent->Activate();
		}
		if (RepSkillData->ActionSFX)
		{
			ActionSFXComponent->SetSound(RepSkillData->ActionSFX);
			ActionSFXComponent->Play();
		}

		if (MovementComponent)
		{
			MovementComponent->InitialSpeed = RepSkillData->ProjectileSpeed;
			MovementComponent->MaxSpeed = RepSkillData->ProjectileSpeed;
			MovementComponent->ProjectileGravityScale = RepSkillData->ProjectileGravity;

			if (RepSkillData->ProjectileSpeed > 0.f && RepSkillData->bIsHoming && RepTargetActor)
			{
				MovementComponent->bIsHomingProjectile = true;
				MovementComponent->HomingTargetComponent = RepTargetActor->GetRootComponent();
				MovementComponent->HomingAccelerationMagnitude = RepSkillData->HomingAcceleration;
			}
		}
	}
}

void APE_SkillActionActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 충돌 판정은 오직 서버에서만 처리합니다. (다중 히트 방지)
	if (!HasAuthority()) return;

	if (OtherActor && OtherActor != Caster && OtherActor != this)
	{
		if (SkillLogicInstance)
		{
			SkillLogicInstance->ApplySkillEffect(Caster, OtherActor, GetActorLocation(), RepSkillData, DamageToApply);
		}

		if (RepSkillData && RepSkillData->bDestroyOnHit)
		{
			if (CollisionComp) CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (MovementComponent)
			{
				MovementComponent->MaxSpeed = 0;
				MovementComponent->HomingAccelerationMagnitude = 0;
				MovementComponent->StopMovementImmediately();
			}
			if (ActionSFXComponent) ActionSFXComponent->FadeOut(0.5f, 0.f);
			SetLifeSpan(2.0f); // 2초 뒤 서버에서 완전히 소멸 (클라이언트도 동기화되어 소멸)
		}
	}
}