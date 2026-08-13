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

void APE_SkillActionActor::InitializeActionActor(AActor* InInstigator, AActor* InTarget, const FVector& InLoc, const UPE_SkillData* InData, float InDamage)
{
	Caster = InInstigator;
	DamageToApply = InDamage;

	RepTargetActor = InTarget;
	RepSkillData = InData;
	RepTargetLocation = InLoc; // 목표 타일 좌표 동기화
	StartLocation = GetActorLocation(); // 출발지 확정

	// 서버 측 시각 효과 및 비행 변수 초기화를 위해 직접 호출
	OnRep_SkillData();
}

void APE_SkillActionActor::OnRep_SkillData()
{
	// 클라이언트 측 시작 위치 보정 (서버에서 스폰된 위치가 클라이언트로 넘어온 직후)
	StartLocation = GetActorLocation();

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
			// 제자리 생성 장판/즉발일 경우 즉시 폭발
			Explode();
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

	// 5. 정확히 목표 타일에 도달 시
	if (Alpha >= 1.0f)
	{
		bIsFlying = false;
		Explode();
	}
}

void APE_SkillActionActor::Explode()
{
	if (HasAuthority())
	{
		// 1. [Delivery]: 타겟 긁어모으기
		TSet<APE_CharacterBase*> AffectedTargets;

		if (RepSkillData->AoEShape == EPEAoEShape::None)
		{
			// 단일 타겟
			if (APE_CharacterBase* TC = Cast<APE_CharacterBase>(RepTargetActor))
			{
				AffectedTargets.Add(TC);
			}
		}
		else
		{
			// 광역(AoE) 타겟 연산 (기존 AoE Logic에 있던 수학 연산 흡수)
			FIntPoint CenterPos(-999, -999);
			if (RepTargetActor)
			{
				if (UACGridMovementComponent* MoveComp = RepTargetActor->FindComponentByClass<UACGridMovementComponent>())
					CenterPos = MoveComp->GetGridPosition();
			}
			else
			{
				TArray<AActor*> Tiles;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AACTile::StaticClass(), Tiles);
				for (AActor* Actor : Tiles)
				{
					if (Actor->GetActorLocation().Equals(RepTargetLocation, 50.f))
					{
						CenterPos = Cast<AACTile>(Actor)->GetGridPosition();
						break;
					}
				}
			}

			if (CenterPos != FIntPoint(-999, -999))
			{
				// PE_SkillData의 일원화된 헬퍼 함수 호출
				TSet<FIntPoint> AffectedPositions = RepSkillData->GetAffectedGridPositions(CenterPos);

				// 범위 내 캐릭터 추출
				TArray<AActor*> AllChars;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);
				for (AActor* Actor : AllChars)
				{
					if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
					{
						if (UACGridMovementComponent* MoveComp = Char->GetGridMovementComponent())
						{
							if (AffectedPositions.Contains(MoveComp->GetGridPosition()))
							{
								AffectedTargets.Add(Char);
							}
						}
					}
				}
			}
		}

		// 2. 모듈에 그룹 타겟 전체(AffectedTargets)를 전달
		for (UPE_SkillEffectModule* Module : RepSkillData->EffectModules)
		{
			if (Module)
			{
				Module->ApplyEffects(Caster, AffectedTargets, RepTargetLocation, RepSkillData, DamageToApply);
			}
		}

		// 3. 중앙 폭발 시각 효과
		if (Caster)
		{
			if (UACSkillComponent* SkillComp = Caster->FindComponentByClass<UACSkillComponent>())
			{
				SkillComp->NetMulticast_PlayHitVisuals(RepSkillData, RepTargetLocation);
			}
		}

		// 4. 액션 종료 통보
		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
			GS->ReportActionEnded(); // 스킬 본체(1 카운트) 소멸 보고
		}
	}

	// 잔여물 정리
	if (RepSkillData && RepSkillData->bDestroyOnHit)
	{
		if (ActionVFXComponent) ActionVFXComponent->Deactivate();
		if (ActionSFXComponent) ActionSFXComponent->FadeOut(0.2f, 0.f);
		SetLifeSpan(0.2f);
	}
}