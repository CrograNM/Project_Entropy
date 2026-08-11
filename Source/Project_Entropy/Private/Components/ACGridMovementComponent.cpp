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
	GridMoveSpeed = 600.f;       
	AcceptanceRadius = 10.f;      
	RotationSpeed = 2000.f;
	bIsMovingOnGrid = false;
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

	SavedPath = InPath;
	CurrentPathIndex = 0;
	bShouldRotate = bRotate;

	// 이동 시작 시 서버 권한으로 최종 목적지 좌표 예약 갱신
	if (GetOwner()->HasAuthority())
	{
		TargetGridPosition = SavedPath.Last()->GetGridPosition();
	}

	if (Delay > 0.f)
	{
		FTimerHandle DelayHandle;
		GetWorld()->GetTimerManager().SetTimer(DelayHandle, this, &UACGridMovementComponent::StartMoving, Delay, false);
	}
	else
	{
		StartMoving();
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
		
		bIsMovingOnGrid = false;
		SavedPath.Empty();
		CurrentPathIndex = 0;

		if (GetOwner()->HasAuthority())
		{
			TargetGridPosition = FIntPoint(-999, -999); // 이동 종료 시 예약 해제
		}

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

		// 2. 메쉬 회전 수동 적용
		if (bShouldRotate)
		{
			FRotator TargetRot = Direction.Rotation();
			FRotator NewRot = FMath::RInterpConstantTo(OwnerActor->GetActorRotation(), TargetRot, DeltaTime, RotationSpeed);
			OwnerActor->SetActorRotation(NewRot);
		}

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