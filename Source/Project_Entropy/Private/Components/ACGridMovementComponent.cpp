// Copyright CrograNM

#include "Components/ACGridMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Grid/ACTile.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Core/PE_GameState.h" 

UACGridMovementComponent::UACGridMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	GridPosition = FIntPoint(-999, -999);
	TargetGridPosition = FIntPoint(-999, -999);
	GridMoveSpeed = 1000.f;
	OvershootFactor = 3.f;
	RotationSpeed = 2000.f;

	bIsMovingOnGrid = false;
	bIsWaitingDelay = false;
	bHasFiredPayload = false;
}

void UACGridMovementComponent::BeginPlay() { Super::BeginPlay(); }

void UACGridMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACGridMovementComponent, GridPosition);
	DOREPLIFETIME(UACGridMovementComponent, TargetGridPosition);
}

void UACGridMovementComponent::NetMulticast_MoveAlongPath_Implementation(const TArray<AACTile*>& InPath, bool bRotate, float Delay, FGridKnockbackPayload Payload)
{
	MoveAlongPath(InPath, bRotate, Delay, Payload);
}

void UACGridMovementComponent::MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate, float Delay, FGridKnockbackPayload Payload)
{
	if (InPath.IsEmpty() && !Payload.bIsActive) return;

	FGridMoveCommand NewCmd;
	NewCmd.Path = InPath;
	NewCmd.bRotate = bRotate;
	NewCmd.AbsoluteStartTime = GetWorld()->GetTimeSeconds() + Delay;
	NewCmd.Payload = Payload;
	MoveCommandQueue.Add(NewCmd);

	if (!bIsMovingOnGrid && !bIsWaitingDelay)
	{
		ProcessNextCommand();
	}
}

void UACGridMovementComponent::ProcessNextCommand()
{
	if (MoveCommandQueue.Num() > 0)
	{
		FGridMoveCommand Cmd = MoveCommandQueue[0];
		MoveCommandQueue.RemoveAt(0);

		SavedPath = Cmd.Path;
		bShouldRotate = Cmd.bRotate;
		CurrentPayload = Cmd.Payload;
		bHasFiredPayload = false; // [초기화] 큐가 새로 시작될 때 폭발 장전
		CurrentPathIndex = 0;

		if (GetOwner()->HasAuthority() && SavedPath.Num() > 0)
		{
			TargetGridPosition = SavedPath.Last()->GetGridPosition();
		}

		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (Cmd.AbsoluteStartTime > CurrentTime)
		{
			bIsWaitingDelay = true;
			DelayTimer = Cmd.AbsoluteStartTime - CurrentTime;
		}
		else
		{
			bIsWaitingDelay = false;
			StartMoving();
		}
	}
	else
	{
		bIsMovingOnGrid = false;
		bIsWaitingDelay = false;

		if (GetOwner()->HasAuthority()) TargetGridPosition = FIntPoint(-999, -999);

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}

		OnMovementFinished.Broadcast();
	}
}

void UACGridMovementComponent::StartMoving()
{
	bIsMovingOnGrid = true;
	SetNextPathStep();
}

void UACGridMovementComponent::SetNextPathStep()
{
	if (CurrentPathIndex < SavedPath.Num())
	{
		TargetWorldLocation = SavedPath[CurrentPathIndex]->GetCenterWorldLocation();
		StepStartLocation = GetOwner()->GetActorLocation();

		float Distance = FVector::Distance(StepStartLocation, TargetWorldLocation);
		StepDuration = FMath::Max(0.01f, Distance / GridMoveSpeed);
		StepElapsedTime = 0.f;
	}
	else
	{
		if (SavedPath.Num() > 0)
		{
			SavedPath.Last()->SetHighlightState(ETileHighlightType::None);
		}
		SavedPath.Empty();
		CurrentPathIndex = 0;

		// 거리가 0칸이라 Tick이 아예 돌지 않았을 때를 대비한 폭발 처리
		ExecuteKnockbackPayload();

		ProcessNextCommand();
	}
}

void UACGridMovementComponent::ExecuteKnockbackPayload()
{
	// [데미지 적용 및 캐릭터 본연의 이펙트 발생을 전담]
	if (CurrentPayload.bIsActive && !bHasFiredPayload)
	{
		bHasFiredPayload = true;

		if (GetOwner()->HasAuthority())
		{
			if (CurrentPayload.TargetDamage > 0.f && GetOwner())
			{
				UGameplayStatics::ApplyDamage(GetOwner(), CurrentPayload.TargetDamage, CurrentPayload.Instigator ? CurrentPayload.Instigator->GetInstigatorController() : nullptr, CurrentPayload.Instigator, UDamageType::StaticClass());
			}
			if (CurrentPayload.OtherDamage > 0.f && CurrentPayload.HitCharacter)
			{
				UGameplayStatics::ApplyDamage(CurrentPayload.HitCharacter, CurrentPayload.OtherDamage, CurrentPayload.Instigator ? CurrentPayload.Instigator->GetInstigatorController() : nullptr, CurrentPayload.Instigator, UDamageType::StaticClass());
			}

			if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
			{
				GS->ReportActionEnded(CurrentPayload.ActionLogID);
			}
		}

		// [스킬 데이터의 하드코딩된 VFX를 버리고, 이벤트 브로드캐스트로 위임]
		if (CurrentPayload.TargetDamage > 0.f || CurrentPayload.OtherDamage > 0.f)
		{
			OnKnockbackImpact.Broadcast();
		}

		CurrentPayload = FGridKnockbackPayload();
	}
}

void UACGridMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsWaitingDelay)
	{
		DelayTimer -= DeltaTime;
		if (DelayTimer <= 0.f)
		{
			bIsWaitingDelay = false;
			StartMoving();
		}
		return;
	}

	if (!bIsMovingOnGrid || SavedPath.IsEmpty()) return;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	StepElapsedTime += DeltaTime;
	float t = FMath::Clamp(StepElapsedTime / StepDuration, 0.f, 1.f);

	bool bIsLastStep = (CurrentPathIndex == SavedPath.Num() - 1);
	float Alpha = t;

	if (!bShouldRotate)
	{
		if (bIsLastStep)
		{
			float c1 = OvershootFactor;
			float c3 = c1 + 1.0f;
			float t_sub_1 = t - 1.0f;
			Alpha = 1.0f + c3 * (t_sub_1 * t_sub_1 * t_sub_1) + c1 * (t_sub_1 * t_sub_1);

			// [핵심: Alpha가 1.0을 돌파하는 정확한 수학적 타이밍에 벽 충돌 데미지 발생!]
			float ImpactTimeThreshold = 1.0f - (c1 / c3);
			if (t >= ImpactTimeThreshold)
			{
				ExecuteKnockbackPayload();
			}
		}
	}

	FVector AdjustTarget = TargetWorldLocation;
	AdjustTarget.Z = StepStartLocation.Z;

	FVector NewLocation = FMath::Lerp(StepStartLocation, AdjustTarget, Alpha);
	OwnerActor->SetActorLocation(NewLocation);

	if (bShouldRotate)
	{
		FVector Direction = (AdjustTarget - StepStartLocation).GetSafeNormal();
		if (!Direction.IsNearlyZero())
		{
			FRotator TargetRot = Direction.Rotation();
			FRotator NewRot = FMath::RInterpConstantTo(OwnerActor->GetActorRotation(), TargetRot, DeltaTime, RotationSpeed);
			OwnerActor->SetActorRotation(NewRot);
		}
	}

	if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		OwnerCharacter->GetCharacterMovement()->Velocity = (AdjustTarget - StepStartLocation).GetSafeNormal() * GridMoveSpeed;
	}

	if (t >= 1.0f)
	{
		GridPosition = SavedPath[CurrentPathIndex]->GetGridPosition();
		OwnerActor->SetActorLocation(AdjustTarget);

		CurrentPathIndex++;
		SetNextPathStep();
	}
}