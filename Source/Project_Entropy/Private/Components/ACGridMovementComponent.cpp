// Copyright CrograNM

#include "Components/ACGridMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Grid/ACTile.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UACGridMovementComponent::UACGridMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	GridPosition = FIntPoint(-999, -999);
	TargetGridPosition = FIntPoint(-999, -999);
	GridMoveSpeed = 1000.f;
	OvershootFactor = 3.0f;
	RotationSpeed = 2000.f;

	bIsMovingOnGrid = false;
	bIsWaitingDelay = false;
}

void UACGridMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACGridMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UACGridMovementComponent, GridPosition);
	DOREPLIFETIME(UACGridMovementComponent, TargetGridPosition);
}

void UACGridMovementComponent::NetMulticast_MoveAlongPath_Implementation(const TArray<AACTile*>& InPath, bool bRotate, float Delay)
{
	MoveAlongPath(InPath, bRotate, Delay);
}

void UACGridMovementComponent::MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate, float Delay)
{
	if (InPath.Num() == 0) return;

	FGridMoveCommand NewCmd;
	NewCmd.Path = InPath;
	NewCmd.bRotate = bRotate;
	NewCmd.AbsoluteStartTime = GetWorld()->GetTimeSeconds() + Delay;
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

		if (GetOwner()->HasAuthority())
		{
			TargetGridPosition = FIntPoint(-999, -999);
		}

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}

		UE_LOG(LogTemp, Warning, TEXT("[ACGridMovementComponent] 큐 대기열 처리 및 액터 이동 완료."));
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

		ProcessNextCommand();
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

	if (!bIsMovingOnGrid) return;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	StepElapsedTime += DeltaTime;
	float t = FMath::Clamp(StepElapsedTime / StepDuration, 0.f, 1.f);

	// [수정됨: 현재 진행 중인 스텝이 경로의 마지막 칸인지 판별]
	bool bIsLastStep = (CurrentPathIndex == SavedPath.Num() - 1);
	float Alpha = t;

	if (!bShouldRotate)
	{
		// 넉백 시, 마지막 칸(최종 도착점)에만 Overshoot(Recoil) 공식 적용
		if (bIsLastStep)
		{
			float c1 = OvershootFactor;
			float c3 = c1 + 1.0f;
			float t_sub_1 = t - 1.0f;
			Alpha = 1.0f + c3 * (t_sub_1 * t_sub_1 * t_sub_1) + c1 * (t_sub_1 * t_sub_1);
		}
		// 마지막 칸이 아닌 중간 이동 구간은 Alpha = t (등속 이동) 유지
	}

	FVector AdjustTarget = TargetWorldLocation;
	AdjustTarget.Z = StepStartLocation.Z;

	FVector NewLocation = FMath::Lerp(StepStartLocation, AdjustTarget, Alpha);
	OwnerActor->SetActorLocation(NewLocation);

	// 방향 회전 처리
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