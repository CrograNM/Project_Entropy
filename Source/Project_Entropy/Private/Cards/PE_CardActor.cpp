// Copyright CrograNM

#include "Cards/PE_CardActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"
#include "Cards/PE_CardData.h"

APE_CardActor::APE_CardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.033f; // 약 30fps로 Tick 최적화

	// 1. 메쉬 컴포넌트 (루트)
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	RootComponent = CardMesh;

	// 마우스 클릭(Raycast)을 감지해야 하므로 블록 처리
	CardMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 카메라에 파묻히지 않도록 렌더링 옵션 (CustomDepth를 활용하여 최상단 렌더링 가능)
	CardMesh->SetRenderCustomDepth(true);

	// 2. 3D 위젯 컴포넌트 (이름, AP, 설명 등 텍스트 표시용)
	CardUIWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardUIWidget"));
	CardUIWidget->SetupAttachment(RootComponent);
	CardUIWidget->SetWidgetSpace(EWidgetSpace::World);
	CardUIWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision); // UI는 클릭을 방해하지 않음

	// 3. VFX 파티클 컴포넌트 (빛 알갱이, 마법진 등)
	VFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFXComponent"));
	VFXComponent->SetupAttachment(RootComponent);
	VFXComponent->SetAutoActivate(false);
}

void APE_CardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsMovingToTarget && RootComponent)
	{
		// '부모 기준 상대 좌표'를 가져옴
		FVector CurrentLocation = RootComponent->GetRelativeLocation();
		FRotator CurrentRotation = RootComponent->GetRelativeRotation();

		FVector TargetLocation = TargetRelativeTransform.GetLocation();
		FRotator TargetRotator = TargetRelativeTransform.GetRotation().Rotator();

		// 보간 연산
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, MoveInterpSpeed);
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotator, DeltaTime, MoveInterpSpeed);

		// 계산된 상대 좌표 적용
		SetActorRelativeLocation(NewLocation);
		SetActorRelativeRotation(NewRotation);

		if (FVector::DistSquared(NewLocation, TargetLocation) < 1.0f &&
			CurrentRotation.Equals(TargetRotator, 1.0f))
		{
			SetActorRelativeLocation(TargetLocation);
			SetActorRelativeRotation(TargetRotator);
			bIsMovingToTarget = false;
		}
	}
}

void APE_CardActor::MoveToTargetTransform(const FTransform& InTargetRelativeTransform)
{
	TargetRelativeTransform = InTargetRelativeTransform;
	bIsMovingToTarget = true; // 이동 시작
}

void APE_CardActor::BeginPlay()
{
	Super::BeginPlay();

	if (CardMesh->GetMaterial(0))
	{
		DynamicMaterial = CardMesh->CreateDynamicMaterialInstance(0);
	}
}

void APE_CardActor::InitializeCard(UPE_CardData* InCardData)
{
	if (!InCardData) return;

	CardData = InCardData;

	// TODO: CardUIWidget 내부의 UserWidget(UPE_CardWidget)을 가져와서 
	// CardData->CardName, CardData->CostAP 등을 텍스트 박스에 쏴주는 로직 추가

	if (DynamicMaterial && CardData->CardArt)
	{
		// 머티리얼에 카드 일러스트 텍스처 적용
		DynamicMaterial->SetTextureParameterValue(TEXT("CardArt"), CardData->CardArt);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CardActor] %s 카드 생성 및 초기화 완료"), *CardData->CardName.ToString());
}

void APE_CardActor::SetHighlightState(bool bIsHighlighted, FLinearColor OutlineColor)
{
	if (!DynamicMaterial) return;

	// 하이라이트 On/Off (0.0 or 1.0)
	float HighlightIntensity = bIsHighlighted ? 1.0f : 0.0f;
	DynamicMaterial->SetScalarParameterValue(TEXT("HighlightIntensity"), HighlightIntensity);

	// 색상 변경 (기본 하이라이트는 흰색, 비용 감소 시 노란색/초록색 등)
	if (bIsHighlighted)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("OutlineColor"), OutlineColor);
	}
}