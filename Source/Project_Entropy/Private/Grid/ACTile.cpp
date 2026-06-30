// Copyright CrograNM

#include "Grid/ACTile.h"

AACTile::AACTile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트로 static mesh 생성
	TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
	RootComponent = TileMesh;

	// 마우스 클릭 레이캐스트가 감지될 수 있도록 콜리전 프로파일 설정
	TileMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AACTile::BeginPlay()
{
	Super::BeginPlay();
	
	if (TileMesh->GetMaterial(0))
	{
		DynamicMaterial = TileMesh->CreateDynamicMaterialInstance(0);
	}
}

FVector AACTile::GetCenterWorldLocation() const
{
	FVector Loc = GetActorLocation();
	// Loc.Z += 50.f; 
	return Loc;
}

void AACTile::SetHighlightState(ETileHighlightType NewState)
{
	if (!DynamicMaterial) return;

	FLinearColor TargetColor; 

	switch (NewState)
	{
	case ETileHighlightType::InRange:
		TargetColor = InRangeColor;
		break;
	case ETileHighlightType::Hovered:
		TargetColor = HoveredColor;
		break;
	case ETileHighlightType::Path:
		TargetColor = PathColor;
		break;
	case ETileHighlightType::None:
	default:
		TargetColor = DefaultColor;
		break;
	}

	DynamicMaterial->SetVectorParameterValue(EmissiveParamName, TargetColor);
}