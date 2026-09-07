// Copyright CrograNM

#include "Components/ACGridMovementComponent.h"
#include "Characters/PE_CharacterBase.h"
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
	GridMoveSpeed = 1000.f;
	OvershootFactor = 3.f;
	RotationSpeed = 2000.f;

	bIsMovingOnGrid = false;
	bIsWaitingDelay = false;
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

	// [액션 큐 안전망]
	// 넉백으로 밀려나던 도중 충돌 데미지로 사망하면 소유 액터가 파괴되어
	// ExecuteKnockbackPayload가 영영 실행되지 않고 토큰이 유실됩니다. (= 큐 영구 정지)
	// 파괴되는 이 시점에 아직 터뜨리지 못한 페이로드를 모두 정산합니다.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (APE_GameState* GS = GetWorld() ? GetWorld()->GetGameState<APE_GameState>() : nullptr)
		{
			if (CurrentPayload.bIsActive && !bHasFiredPayload)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ActionQueue] %s 가 넉백 처리 도중 파괴되어 토큰을 정산합니다. Token=%d"),
					*GetNameSafe(GetOwner()), CurrentPayload.ActionTokenID);

				bHasFiredPayload = true;
				GS->EndAction(CurrentPayload.ActionTokenID, CurrentPayload.ActionLogID);
			}

			// 아직 시작조차 못한 대기 명령들의 토큰도 함께 반납합니다.
			for (const FGridMoveCommand& Cmd : MoveCommandQueue)
			{
				if (Cmd.Payload.bIsActive)
				{
					GS->EndAction(Cmd.Payload.ActionTokenID, Cmd.Payload.ActionLogID);
				}
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
		// 이동 종료 후 상태 초기화
		bIsMovingOnGrid = false;
		bIsWaitingDelay = false;

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
				GS->EndAction(CurrentPayload.ActionTokenID, CurrentPayload.ActionLogID);
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
		// SetGridPositionInternal 제거: TMap에는 이미 Occupancy가 TargetGridPosition으로 업데이트되어 있음 (한 캐릭터 당 하나의 위치만 점유 가능)
		
		OwnerActor->SetActorLocation(AdjustTarget);

		CurrentPathIndex++;
		SetNextPathStep();
	}
}