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
		if (MovementComponent)
		{
			MovementComponent->InitialSpeed = SkillData->ProjectileSpeed;
			MovementComponent->MaxSpeed = SkillData->ProjectileSpeed;
			MovementComponent->ProjectileGravityScale = SkillData->ProjectileGravity;

			// 속도가 있어서 이동해야 하는 스킬일 경우
			if (SkillData->ProjectileSpeed > 0.f)
			{
				// 유도 옵션을 켰고 타겟이 존재할 경우에만 호밍 적용
				if (SkillData->bIsHoming && TargetActor)
				{
					MovementComponent->bIsHomingProjectile = true;
					MovementComponent->HomingTargetComponent = TargetActor->GetRootComponent();
					MovementComponent->HomingAccelerationMagnitude = SkillData->HomingAcceleration;
				}
				else
				{
					// 유도 옵션이 없으면 목표 '좌표'를 향해 일직선으로(또는 포물선으로) 날아감
					MovementComponent->bIsHomingProjectile = false;
					FVector Direction = (InLoc - GetActorLocation()).GetSafeNormal();
					MovementComponent->Velocity = Direction * MovementComponent->InitialSpeed;
				}
			}
		}
	}
}

void APE_SkillActionActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != Caster && OtherActor != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillActionActor] 투사체 적중됨"));

		// 3단계 (Hit) 발동 지시
		if (SkillLogicInstance)
		{
			SkillLogicInstance->ApplySkillEffect(Caster, OtherActor, GetActorLocation(), SkillData, DamageToApply);
		}

		// 투사체면 바로 파괴, 장판이면 계속 유지
		if (SkillData && SkillData->bDestroyOnHit)
		{
			Destroy();
		}
	}
}