// Copyright CrograNM

#include "Grid/ACTile.h"
#include "GameFramework/PlayerController.h"
#include "Characters/PE_EnemyBase.h"

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
	// nullptr을 전달하여 '시스템 전역 요청'으로 처리합니다.
	RequestHighlight(nullptr, NewState);
}

void AACTile::RequestHighlight(AActor* Requester, ETileHighlightType Type)
{
	if (Type == ETileHighlightType::None)
	{
		HighlightRequests.Remove(Requester);
	}
	else
	{
		HighlightRequests.Add(Requester, Type);
	}

	UpdateVisuals();
}

void AACTile::UpdateVisuals()
{
	if (!DynamicMaterial) return;

	if (HighlightRequests.IsEmpty())
	{
		DynamicMaterial->SetVectorParameterValue(EmissiveParamName, DefaultColor);
		DynamicMaterial->SetScalarParameterValue(InsideOpacityParamName, 0.0f);
		return;
	}

	FLinearColor FinalColor = FLinearColor::Black;
	float FinalOpacity = 0.0f;

	// 화면을 보고 있는 나 자신(로컬 플레이어 폰)을 구합니다.
	AActor* LocalPawn = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;

	for (const auto& Pair : HighlightRequests)
	{
		AActor* RequesterActor = Pair.Key;
		ETileHighlightType Type = Pair.Value;

		// nullptr인 경우(시스템/맵툴 호출)는 로컬 취급(기본 색상)하여 렌더링합니다.
		bool bIsLocal = (RequesterActor == nullptr) || (RequesterActor == LocalPawn);
		bool bIsEnemy = RequesterActor && RequesterActor->IsA(APE_EnemyBase::StaticClass()); 

		FLinearColor TypeColor = DefaultColor;

		switch (Type)
		{
		case ETileHighlightType::InRange:
			if (bIsEnemy) TypeColor = EnemyInRangeColor;
			else TypeColor = bIsLocal ? InRangeColor : OtherInRangeColor;
			break;
		case ETileHighlightType::Hovered:
			if (bIsEnemy) TypeColor = EnemySkillTargetColor;
			else TypeColor = bIsLocal ? HoveredColor : OtherHoveredColor;
			break;
		case ETileHighlightType::Path:
			if (bIsEnemy) TypeColor = EnemyPathColor;
			else TypeColor = bIsLocal ? PathColor : OtherPathColor;
			break;
		case ETileHighlightType::SkillTarget:
			if (bIsEnemy) TypeColor = EnemySkillTargetColor;
			else TypeColor = bIsLocal ? SkillTargetColor : OtherSkillTargetColor;
			break;
		}

		FinalColor += TypeColor;
		FinalOpacity = FMath::Max(FinalOpacity, InsideOpacityValue);
	}

	// 색상이 하얗게 타버리는 것을 방지하기 위해 Clamp
	FinalColor.R = FMath::Min(FinalColor.R, 1.0f);
	FinalColor.G = FMath::Min(FinalColor.G, 1.0f);
	FinalColor.B = FMath::Min(FinalColor.B, 1.0f);
	FinalColor.A = 1.0f;

	DynamicMaterial->SetVectorParameterValue(EmissiveParamName, FinalColor);
	DynamicMaterial->SetScalarParameterValue(InsideOpacityParamName, FinalOpacity);
}

void AACTile::SetObstacle(bool bInObstacle)
{
	bIsObstacle = bInObstacle;
	if (ObstacleMesh) { ObstacleMesh->SetVisibility(bIsObstacle); }

	if (bIsObstacle)
	{
		HighlightRequests.Empty();
		UpdateVisuals();
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