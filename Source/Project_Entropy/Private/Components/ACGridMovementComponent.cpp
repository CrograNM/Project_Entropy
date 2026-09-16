// Copyright CrograNM

#include "Components/ACGridMovementComponent.h"
#include "Characters/PE_CharacterBase.h"
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
	GridMoveSpeed = 1000.f;
	OvershootFactor = 3.f;
	RotationSpeed = 2000.f;

	bIsMovingOnGrid = false;
	bHasFiredPayload = false;
}

AACGridSystem* UACGridMovementComponent::GetCachedGridSystem()
{
	if (!CachedGridSystem)
	{
		CachedGridSystem = Cast<AACGridSystem>(
			UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
	}
	return CachedGridSystem;
}

void UACGridMovementComponent::BeginPlay() 
{ 
	Super::BeginPlay(); 

	SnapCharacterToNearestTile();
}

void UACGridMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AACGridSystem* GridSystem = GetCachedGridSystem())
	{
		GridSystem->RemoveOccupant(Cast<APE_CharacterBase>(GetOwner()));
	}

	/*
		[연쇄 안전망]
		넉백으로 밀려나던 도중 충돌 데미지로 사망하면 소유 액터가 파괴되어
		ExecuteKnockbackPayload가 영영 실행되지 않습니다. 그러면 코디네이터가 도착 보고를 받지 못해
		연쇄가 멈추고 액션 토큰도 워치독이 강제 해제할 때까지 물려 있게 됩니다.
		파괴되는 이 시점에 아직 보고하지 못한 넉백을 모두 대신 보고합니다.
	*/
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		APE_CharacterBase* OwnerChar = Cast<APE_CharacterBase>(GetOwner());

		if (CurrentPayload.bIsActive && !bHasFiredPayload)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Push] %s 가 넉백 처리 도중 파괴되어 도착을 대신 보고합니다. Chain=%d"),
				*GetNameSafe(GetOwner()), CurrentPayload.ChainID);

			bHasFiredPayload = true;
			OnKnockbackSettled.Broadcast(OwnerChar, CurrentPayload.ChainID);
		}

		// 아직 시작조차 못한 대기 명령들도 함께 보고해야 연쇄가 정상 종료됩니다.
		for (const FGridMoveCommand& Cmd : MoveCommandQueue)
		{
			if (Cmd.Payload.bIsActive)
			{
				OnKnockbackSettled.Broadcast(OwnerChar, Cmd.Payload.ChainID);
			}
		}
	}
	MoveCommandQueue.Empty();

	Super::EndPlay(EndPlayReason);
}

void UACGridMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACGridMovementComponent, GridPosition);
}

void UACGridMovementComponent::SnapCharacterToNearestTile()
{
	// 모든 그리드 이동 컴포넌트를 가진 객체 --> 레벨 배치 시 가장 가까운 타일로 스냅
	if (AACGridSystem* GridSystem = GetCachedGridSystem())
	{
		FVector Loc = GetOwner()->GetActorLocation();

		if (AACTile* ClosestTile = GridSystem->GetNearestTile(Loc))
		{
			SetGridPosition(ClosestTile->GetGridPosition());
			FVector SnapLocation = ClosestTile->GetCenterWorldLocation();
			SnapLocation.Z = Loc.Z;
			GetOwner()->SetActorLocation(SnapLocation);
		}
	}

}

void UACGridMovementComponent::NetMulticast_MoveAlongPath_Implementation(const TArray<AACTile*>& InPath, bool bRotate, FGridKnockbackPayload Payload)
{
	MoveAlongPath(InPath, bRotate, Payload);
}

void UACGridMovementComponent::MoveAlongPath(const TArray<AACTile*>& InPath, bool bRotate, FGridKnockbackPayload Payload)
{
	if (InPath.IsEmpty() && !Payload.bIsActive) return;

	FGridMoveCommand NewCmd;
	NewCmd.Path = InPath;
	NewCmd.bRotate = bRotate;
	NewCmd.Payload = Payload;
	MoveCommandQueue.Add(NewCmd);

	/*
		경로가 비어 있으면 아래 호출 안에서 ExecuteKnockbackPayload까지 곧바로 진행되어
		도착 보고가 이 함수가 끝나기도 전에 호출자에게 되돌아갑니다.
		코디네이터가 재진입을 견디도록 만들어져 있는 이유입니다.
	*/
	if (!bIsMovingOnGrid)
	{
		ProcessNextCommand();
	}
}

void UACGridMovementComponent::SetGridPosition(FIntPoint NewPos)
{
	if (GridPosition == NewPos) return;

	AACGridSystem* GridSystem = GetCachedGridSystem();
	if (GridSystem)
	{
		GridSystem->UpdateOccupancy(Cast<APE_CharacterBase>(GetOwner()), GridPosition, NewPos);
	}

	GridPosition = NewPos;
}

void UACGridMovementComponent::OnRep_GridPosition(FIntPoint OldPos)
{
	// 복제로 값이 도착했을 때
	if (AACGridSystem* Grid = GetCachedGridSystem())
	{
		Grid->UpdateOccupancy(Cast<APE_CharacterBase>(GetOwner()), OldPos, GridPosition);
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

		if (SavedPath.Num() > 0)
		{
			SetGridPosition(SavedPath.Last()->GetGridPosition());
		}

		StartMoving();
	}
	else
	{
		// 이동 종료 후 상태 초기화
		bIsMovingOnGrid = false;

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
	if (!CurrentPayload.bIsActive || bHasFiredPayload) return;

	bHasFiredPayload = true;

	/*
		피해가 소유 액터를 즉사시켜 파괴로 이어질 수 있으므로, 필요한 값을 먼저 떼어내고
		CurrentPayload를 비웁니다. 그래야 그 사이 EndPlay가 끼어들어도 이중 보고가 나지 않습니다.
	*/
	APE_CharacterBase* OwnerChar = Cast<APE_CharacterBase>(GetOwner());
	const FGridKnockbackPayload Payload = CurrentPayload;
	CurrentPayload = FGridKnockbackPayload();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		AController* InstigatorController = Payload.Instigator ? Payload.Instigator->GetInstigatorController() : nullptr;

		if (Payload.TargetDamage > 0.f)
		{
			UGameplayStatics::ApplyDamage(GetOwner(), Payload.TargetDamage, InstigatorController, Payload.Instigator, UDamageType::StaticClass());
		}
		if (Payload.OtherDamage > 0.f && Payload.HitCharacter)
		{
			UGameplayStatics::ApplyDamage(Payload.HitCharacter, Payload.OtherDamage, InstigatorController, Payload.Instigator, UDamageType::StaticClass());
		}
	}

	// [스킬 데이터의 하드코딩된 VFX를 버리고, 이벤트 브로드캐스트로 위임]
	if (Payload.TargetDamage > 0.f || Payload.OtherDamage > 0.f)
	{
		OnKnockbackImpact.Broadcast();
	}

	/*
		[도착 보고] 연쇄 밀치기는 오직 여기서만 출발합니다.
		마지막 칸의 오버슈트가 1.0을 돌파하는 순간에 호출되므로,
		'앞 캐릭터가 눈으로 보기에 닿는 그 프레임'과 연쇄 출발이 일치합니다.
	*/
	OnKnockbackSettled.Broadcast(OwnerChar, Payload.ChainID);
}

void UACGridMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

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
		// SetGridPositionInternal 제거: TMap에는 이미 Occupancy가 TargetGridPosition으로 업데이트되어 있음 (한 캐릭터 당 하나의 위치만 점유 가능)
		
		OwnerActor->SetActorLocation(AdjustTarget);

		CurrentPathIndex++;
		SetNextPathStep();
	}
}