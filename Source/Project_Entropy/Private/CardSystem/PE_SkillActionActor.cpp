// Copyright CrograNM

#include "CardSystem/PE_SkillActionActor.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CardSystem/PE_SkillData.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Core/PE_GameState.h"
#include "CardSystem/PE_SkillEffectModule.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACSkillComponent.h"
#include "Grid/ACTile.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

APE_SkillActionActor::APE_SkillActionActor()
{
	// Tick을 활성화하여 궤적 보간을 수행합니다.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// 물리 충돌 컴포넌트(USphereComponent, UProjectileMovementComponent)를 완전히 삭제하고 SceneRoot 기반으로 변경
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	ActionVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ActionVFX"));
	ActionVFXComponent->SetupAttachment(RootComponent);
	ActionVFXComponent->SetAutoActivate(false);

	ActionSFXComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ActionSFX"));
	ActionSFXComponent->SetupAttachment(RootComponent);
	ActionSFXComponent->SetAutoActivate(false);
}

void APE_SkillActionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APE_SkillActionActor, RepSkillData);
	DOREPLIFETIME(APE_SkillActionActor, RepTargetActor);
	DOREPLIFETIME(APE_SkillActionActor, RepTargetLocation);
}

void APE_SkillActionActor::InitializeActionActor(AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage, int32 InActionLogID, const TSet<APE_CharacterBase*>& InTargets)
{
	Caster = InInstigator;
	DamageToApply = InDamage;
	ActionLogID = InActionLogID;

	RepTargetActor = InTarget;
	RepSkillData = InData;
	RepTargetLocation = InLoc; // 목표 타일 좌표 동기화
	StartLocation = GetActorLocation(); // 출발지 확정

	PendingTargets = InTargets;

	// 서버 측 시각 효과 및 비행 변수 초기화를 위해 직접 호출
	OnRep_SkillData();
}

void APE_SkillActionActor::OnRep_SkillData()
{
	// 클라이언트 측 시작 위치 보정 (서버에서 스폰된 위치가 클라이언트로 넘어온 직후)
	StartLocation = GetActorLocation();

	if (RepSkillData)
	{
		// 이펙트는 딜레이와 상관없이 즉시 재생
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

		// 이동이 필요한 투사체인지 검사
		if (RepSkillData->ProjectileSpeed > 0.f)
		{
			float Distance = FVector::Distance(StartLocation, RepTargetLocation);
			FlightDuration = Distance / RepSkillData->ProjectileSpeed; // 거리 / 속도 = 걸리는 시간 보장

			// ProjectileGravity 변수를 포물선의 최대 높이 값(ArcHeight)으로 활용합니다.
			ArcHeight = RepSkillData->ProjectileGravity;

			bIsFlying = true;
		}
		else
		{
			bIsFlying = false;

			// 투사체가 아닐 경우, 또는 추가 연출 등 딜레이가 있다면 타이머 후 폭발 처리
			if (RepSkillData->ExplosionDelay > 0.f)
			{
				FTimerHandle DelayTimer;
				GetWorld()->GetTimerManager().SetTimer(DelayTimer, this, &APE_SkillActionActor::Explode, RepSkillData->ExplosionDelay, false);
			}
			else
			{
				Explode();
			}
		}
	}
}

void APE_SkillActionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsFlying) return;

	CurrentFlightTime += DeltaTime;
	float Alpha = FMath::Clamp(CurrentFlightTime / FlightDuration, 0.f, 1.f);

	// 1. X, Y 축 직선 보간
	FVector LerpXY = FMath::Lerp(StartLocation, RepTargetLocation, Alpha);

	// 2. Z 축 포물선(Sine Curve) 연산 추가
	// Alpha가 0.5(중간)일 때 Sin(0.5 * PI) = 1 이 되어 최고점(ArcHeight)에 도달합니다.
	float ZOffset = FMath::Sin(Alpha * PI) * ArcHeight;
	LerpXY.Z += ZOffset;

	// 3. 이동 방향으로 회전 적용
	FVector MoveDirection = (LerpXY - GetActorLocation()).GetSafeNormal();
	if (!MoveDirection.IsNearlyZero())
	{
		SetActorRotation(MoveDirection.Rotation());
	}

	// 4. 위치 갱신
	SetActorLocation(LerpXY);

	// 비행 도중 관통 타격 처리 로직
	if (HasAuthority() && RepSkillData && !RepSkillData->bDestroyOnHit && RepSkillData->ProjectileSpeed > 0.f)
	{
		TArray<APE_CharacterBase*> HitThisFrame;

		for (APE_CharacterBase* Target : PendingTargets)
		{
			if (Target)
			{
				float Dist = FVector::Distance(GetActorLocation(), Target->GetActorLocation());

				// 투사체가 타겟과 150 유닛 내로 겹칠 때 타격 인정 (투사체 속도에 따른 오차 방지)
				if (Dist < 150.f)
				{
					HitThisFrame.Add(Target);
				}
			}
		}

		// 이번 프레임에 부딪힌 적들에게 모듈 효과(데미지/넉백 등) 적용 및 VFX 발동
		for (APE_CharacterBase* HitTarget : HitThisFrame)
		{
			TSet<APE_CharacterBase*> SingleTarget;
			SingleTarget.Add(HitTarget);

			for (UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
			{
				Module->ApplyEffects(Caster, SingleTarget, HitTarget->GetActorLocation(), RepSkillData, DamageToApply);
			}

			if (UACSkillComponent* SkillComp = Caster->FindComponentByClass<UACSkillComponent>())
			{
				SkillComp->NetMulticast_PlayHitVisuals(RepSkillData, HitTarget->GetActorLocation());
			}

			// 두 번 맞지 않도록 대기 명단에서 제거
			PendingTargets.Remove(HitTarget);
		}
	}

	// 5. 정확히 목표 타일에 도달 시
	if (Alpha >= 1.0f)
	{
		bIsFlying = false;

		// 투사체가 아닐 경우, 또는 추가 연출 등 딜레이가 있다면 타이머 후 폭발 처리
		if (RepSkillData->ExplosionDelay > 0.f)
		{
			FTimerHandle DelayTimer;
			GetWorld()->GetTimerManager().SetTimer(DelayTimer, this, &APE_SkillActionActor::Explode, RepSkillData->ExplosionDelay, false);
		}
		else
		{
			Explode();
		}
	}
}

void APE_SkillActionActor::Explode()
{
	if (HasAuthority())
	{
		// 파괴형 투사체이거나, 범위에 도달했는데 아직 타격받지 않은 잔여 타겟이 남아있을 때 일괄 처리
		if (RepSkillData && (RepSkillData->bDestroyOnHit || PendingTargets.Num() > 0))
		{
			for (UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
			{
				if (Module)
				{
					Module->ApplyEffects(Caster, PendingTargets, RepTargetLocation, RepSkillData, DamageToApply);
				}
			}

			// 폭발 시각 효과
			if (Caster)
			{
				if (UACSkillComponent* SkillComp = Caster->FindComponentByClass<UACSkillComponent>())
				{
					SkillComp->NetMulticast_PlayHitVisuals(RepSkillData, RepTargetLocation);
				}
			}
		}

		// 액션 종료 통보
		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
			GS->ReportActionEnded(ActionLogID);
		}
	}

	// 잔여물 정리
	if (ActionVFXComponent) ActionVFXComponent->Deactivate();
	if (ActionSFXComponent) ActionSFXComponent->FadeOut(0.2f, 0.f);
	SetLifeSpan(0.2f);
}