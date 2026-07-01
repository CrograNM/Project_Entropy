// Copyright CrograNM

#include "Components/ACGridMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Grid/ACTile.h"

UACGridMovementComponent::UACGridMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	GridPosition = FIntPoint(-999, -999);
	GridMoveSpeed = 600.f;       
	AcceptanceRadius = 10.f;      
	RotationSpeed = 2000.f;
	bIsMovingOnGrid = false;
}

void UACGridMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACGridMovementComponent::MoveAlongPath(const TArray<AACTile*>& InPath)
{
	if (InPath.Num() == 0) return;

	SavedPath = InPath;
	CurrentPathIndex = 0;
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
		
		bIsMovingOnGrid = false;
		SavedPath.Empty();
		CurrentPathIndex = 0;
		
		// 애니메이션 정지를 위해 속도 리셋
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			OwnerCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		
		UE_LOG(LogTemp, Warning, TEXT("[ACGridMovementComponent] 액터 이동 완료."));
		OnMovementFinished.Broadcast();
	}
}

void UACGridMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsMovingOnGrid) return;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	FVector CurrentLocation = OwnerActor->GetActorLocation();
	FVector AdjustTarget = TargetWorldLocation;
	AdjustTarget.Z = CurrentLocation.Z; // Z축 고정으로 오차 방지

	FVector Direction = AdjustTarget - CurrentLocation;
	float DistanceToTarget2D = Direction.Size2D();

	if (DistanceToTarget2D > AcceptanceRadius)
	{
		Direction.Normalize();

		// 1. 위치 이동 보간
		FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, AdjustTarget, DeltaTime, GridMoveSpeed);
		OwnerActor->SetActorLocation(NewLocation);
		
		// 2. 메쉬 회전 수동 강제 적용 (컴포넌트 독립성 확보)
		FRotator TargetRot = Direction.Rotation();
		FRotator NewRot = FMath::RInterpConstantTo(OwnerActor->GetActorRotation(), TargetRot, DeltaTime, RotationSpeed);
		OwnerActor->SetActorRotation(NewRot);

		// 3. 애니메이션 연동을 위한 가짜 속도 주입
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
		{
			OwnerCharacter->GetCharacterMovement()->Velocity = Direction * GridMoveSpeed;
		}
	}
	else
	{
		// 목표 칸에 근접하면, 미세 오차를 무시하고 타일 정중앙으로 완전히 스냅시킵니다.
		GridPosition = SavedPath[CurrentPathIndex]->GetGridPosition();
		OwnerActor->SetActorLocation(AdjustTarget); 
		
		CurrentPathIndex++;
		SetNextPathStep();
	}
}