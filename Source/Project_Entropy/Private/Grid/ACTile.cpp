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
	
	// 장애물 메쉬 초기화 (기본적으로 숨김 처리)
	ObstacleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObstacleMesh"));
	ObstacleMesh->SetupAttachment(RootComponent);
	ObstacleMesh->SetVisibility(false);
	ObstacleMesh->SetCollisionProfileName(TEXT("NoCollision"));
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
	case ETileHighlightType::SkillTarget:
		TargetColor = SkillTargetColor;
		break;
	case ETileHighlightType::None:
		TargetColor = DefaultColor;
		break;
	default:
		TargetColor = DefaultColor;
		break;
	}

	DynamicMaterial->SetVectorParameterValue(EmissiveParamName, TargetColor);
}

void AACTile::SetObstacle(bool bInObstacle)
{
	bIsObstacle = bInObstacle;
	
	if (ObstacleMesh)
	{
		ObstacleMesh->SetVisibility(bIsObstacle);
	}
	
	// 장애물이 생기면 기본 하이라이트로 초기화
	if (bIsObstacle)
	{
		SetHighlightState(ETileHighlightType::None);
	}
}

// 에디터 프로퍼티 변경 감지 로직
#if WITH_EDITOR
void AACTile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 어떤 변수가 변경되었는지 이름(FName)을 가져옵니다.
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// 변경된 변수가 'bIsObstacle'인지 확인합니다.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AACTile, bIsObstacle))
	{
		// 시각적 메쉬 갱신 및 하이라이트 초기화 함수 호출
		SetObstacle(bIsObstacle);
	}
}
#endif