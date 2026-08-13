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
	SetIsReplicatedByDefault(true); // 멀티플레이어 동기화 활성화

	GridPosition = FIntPoint(-999, -999);
	TargetGridPosition = FIntPoint(-999, -999);
	GridMoveSpeed = 600.f;
	AcceptanceRadius = 10.f;
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
	// 기존 경로를 덮어쓰지 않고 큐에 명령을 적재
	FGridMoveCommand NewCmd;
	NewCmd.Path = InPath;
	NewCmd.bRotate = bRotate;
	NewCmd.AbsoluteStartTime = GetWorld()->GetTimeSeconds() + Delay;
	MoveCommandQueue.Add(NewCmd);

	// 대기 중인 작업이 없다면 즉시 처리 시작
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

		// 남은 대기 시간이 있다면 타이머 작동, 아니면 즉시 출발
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
		// 모든 큐가 비워졌을 때 완전히 정지
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
	}
	else
	{
		// 최종 목적지 타일에 완벽히 도달함
		if (SavedPath.Num() > 0)
		{
			SavedPath.Last()->SetHighlightState(ETileHighlightType::None);
		}
		
		if (SavedPath.Num() > 0)
		{
			SavedPath.Last()->SetHighlightState(ETileHighlightType::None);
		}

		SavedPath.Empty();
		CurrentPathIndex = 0;

		// 하나의 이동 덩어리가 끝났으므로 큐에 남은 다음 명령이 있는지 검사
		ProcessNextCommand();
	}
}

void UACGridMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 지연 대기열 틱 처리
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

	FVector CurrentLocation = OwnerActor->GetActorLocation();
	FVector AdjustTarget = TargetWorldLocation;
	AdjustTarget.Z = CurrentLocation.Z;

	FVector Direction = AdjustTarget - CurrentLocation;
	float DistanceToTarget2D = Direction.Size2D();

	if (DistanceToTarget2D > AcceptanceRadius)
	{
		Direction.Normalize();

		FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, AdjustTarget, DeltaTime, GridMoveSpeed);
		OwnerActor->SetActorLocation(NewLocation);

		if (bShouldRotate)
		{
			FRotator TargetRot = Direction.Rotation();
			FRotator NewRot = FMath::RInterpConstantTo(OwnerActor->GetActorRotation(), TargetRot, DeltaTime, RotationSpeed);
			OwnerActor->SetActorRotation(NewRot);
		}

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
		{
			OwnerCharacter->GetCharacterMovement()->Velocity = Direction * GridMoveSpeed;
		}
	}
	else
	{
		GridPosition = SavedPath[CurrentPathIndex]->GetGridPosition();
		OwnerActor->SetActorLocation(AdjustTarget);

		CurrentPathIndex++;
		SetNextPathStep();
	}
}