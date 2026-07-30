// Copyright CrograNM

#include "Components/ACTargetingVisualizerComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UACTargetingVisualizerComponent::UACTargetingVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 캐릭터에 부착되어 모두에게 복제됨
}

void UACTargetingVisualizerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepTargetingMode);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepRange);
	DOREPLIFETIME(UACTargetingVisualizerComponent, RepHoveredTile);
}

void UACTargetingVisualizerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UACTargetingVisualizerComponent::SetTargetingMode(ETargetingMode NewMode, int32 InRange)
{
	RepTargetingMode = NewMode;
	RepRange = InRange;
	RefreshVisuals(); // 즉각적인 로컬 반응 (Local Prediction)

	if (!GetOwner()->HasAuthority())
	{
		Server_SetTargetingState(NewMode, InRange);
	}
}

void UACTargetingVisualizerComponent::UpdateHoveredTile(FIntPoint NewPos)
{
	if (RepHoveredTile != NewPos) // 변화가 있을 때만 연산 (최적화)
	{
		RepHoveredTile = NewPos;
		RefreshVisuals();

		if (!GetOwner()->HasAuthority())
		{
			Server_UpdateHoveredTile(NewPos);
		}
	}
}

void UACTargetingVisualizerComponent::ClearTargeting()
{
	SetTargetingMode(ETargetingMode::None, 0);
	UpdateHoveredTile(FIntPoint(-999, -999));
}

bool UACTargetingVisualizerComponent::IsTileInRange(AACTile* TargetTile) const
{
	return TargetTile && CurrentValidTiles.Contains(TargetTile);
}

void UACTargetingVisualizerComponent::Server_SetTargetingState_Implementation(ETargetingMode NewMode, int32 InRange)
{
	RepTargetingMode = NewMode;
	RepRange = InRange;
	RefreshVisuals(); // 서버가 직접 시각화 갱신
}

void UACTargetingVisualizerComponent::Server_UpdateHoveredTile_Implementation(FIntPoint NewPos)
{
	RepHoveredTile = NewPos;
	RefreshVisuals();
}

void UACTargetingVisualizerComponent::OnRep_TargetingState() { RefreshVisuals(); }
void UACTargetingVisualizerComponent::OnRep_HoveredTile() { RefreshVisuals(); }

void UACTargetingVisualizerComponent::RefreshVisuals()
{
	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
	UACGridMovementComponent* MoveComp = GetOwner()->FindComponentByClass<UACGridMovementComponent>();
	if (!GridSystem || !MoveComp) return;

	AActor* OwnerActor = GetOwner();
	FIntPoint CenterPos = MoveComp->GetGridPosition();

	// 1. 기존의 내 잔상들을 모두 지웁니다.
	GridSystem->ClearAllHighlightsFor(OwnerActor);
	CurrentValidTiles.Empty();

	// 2. 모드에 따라 범위를 그립니다.
	if (RepTargetingMode != ETargetingMode::None && RepRange > 0)
	{
		CurrentValidTiles = GridSystem->HighlightArea(OwnerActor, CenterPos, RepRange);
	}

	// 3. 목표 지점에 마우스를 올렸다면, 디테일한 조준선/경로를 그립니다.
	if (RepHoveredTile != FIntPoint(-999, -999) && RepTargetingMode != ETargetingMode::None)
	{
		if (RepTargetingMode == ETargetingMode::Movement)
		{
			GridSystem->HighlightPath(OwnerActor, CenterPos, RepHoveredTile, CurrentValidTiles);
		}
		else if (RepTargetingMode == ETargetingMode::Skill)
		{
			GridSystem->HighlightTarget(OwnerActor, RepHoveredTile);
		}
	}
}