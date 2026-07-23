// Copyright CrograNM

#include "CardSystem/PE_CardActor.h"
#include "CardSystem/PE_CardInstance.h"
#include "CardSystem/PE_CardData.h"
#include "CardSystem/PE_CardWidget.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "NiagaraComponent.h"

APE_CardActor::APE_CardActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.022222f; // 약 45fps로 Tick 최적화

	// 루트 씬 생성 (레이아웃의 위치를 담당)
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// 박스 충돌체 (마우스 입력을 받으며, 루트에 고정되어 절대 튀어나오지 않음)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 카드 메쉬 (시각적 역할만 수행하며, 충돌은 끕니다)
	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	CardMesh->SetupAttachment(RootComponent);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 메쉬의 충돌 방해 금지
	CardMesh->SetRenderCustomDepth(true);

	// 라이팅 채널 분리 (월드 빛 무시, 전용 빛만 받음)
	CardMesh->LightingChannels.bChannel0 = false; // 월드 디렉셔널 라이트 무시
	CardMesh->LightingChannels.bChannel1 = true;  // 3D UI 전용 라이트 채널 활성화
	CardMesh->bCastDynamicShadow = true;          // 카드끼리 그림자 드리우기 허용

	// UI와 VFX는 CardMesh에 부착
	CardUIWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CardUIWidget"));
	CardUIWidget->SetupAttachment(CardMesh);
	CardUIWidget->SetWidgetSpace(EWidgetSpace::World);
	CardUIWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardUIWidget->SetTickWhenOffscreen(true); // 화면 밖에서도 틱 연산 허용 (UI 업데이트를 위해)

	VFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFXComponent"));
	VFXComponent->SetupAttachment(CardMesh);
	VFXComponent->SetAutoActivate(false);
}

void APE_CardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 지연 매핑: Render Target이 생성될 때까지 기다림
	if (!bIsUIMapped && CardUIWidget && DynamicMaterial)
	{
		if (UTextureRenderTarget2D* UIRenderTarget = CardUIWidget->GetRenderTarget())
		{
			DynamicMaterial->SetTextureParameterValue(TEXT("CardUITexture"), UIRenderTarget);

			bIsUIMapped = true; // 매핑 완료

			UE_LOG(LogTemp, Warning, TEXT("[CardActor] UI Render Target 지연 매핑 성공!"));
		}
	}

	if (!bIsMovingToTarget || !RootComponent) return;
	
	// '부모 기준 상대 좌표'를 가져옴
	FVector CurrentRootLoc = RootComponent->GetRelativeLocation();
	FRotator CurrentRootRot = RootComponent->GetRelativeRotation();
	
	// 원래 자리를 기본 목표로 설정
	FVector TargetRootLoc = TargetRelativeTransform.GetLocation();
	FRotator TargetRootRot = TargetRelativeTransform.GetRotation().Rotator();
	
	// 보간 연산
	FVector NewRootLoc = FMath::VInterpTo(CurrentRootLoc, TargetRootLoc, DeltaTime, MoveInterpSpeed);
	FRotator NewRootRot = FMath::RInterpTo(CurrentRootRot, TargetRootRot, DeltaTime, MoveInterpSpeed);
	
	// 계산된 상대 좌표 적용
	SetActorRelativeLocation(NewRootLoc);
	SetActorRelativeRotation(NewRootRot);

	// --- [호버링 이동] CardMesh만 오프셋과 회전을 조절 ---
	FVector CurrentMeshLoc = CardMesh->GetRelativeLocation();
	FRotator CurrentMeshRot = CardMesh->GetRelativeRotation();

	// 평상시 메쉬의 목표는 자기 자리(0,0,0)를 유지하는 것
	FVector TargetMeshLoc = FVector::ZeroVector;
	FRotator TargetMeshRot = FRotator::ZeroRotator;

	if (bIsHovered)
	{
		// 마우스를 올렸을 때는 팝업 오프셋을 적용
		TargetMeshLoc = HoverOffset;

		// 부모(Root)가 부채꼴 모양으로 꺾여있으므로, 그 반대로 회전해야 결과적으로 카메라 정면.
		TargetMeshRot = TargetRelativeTransform.GetRotation().Inverse().Rotator();
	}

	FVector NewMeshLoc = FMath::VInterpTo(CurrentMeshLoc, TargetMeshLoc, DeltaTime, MoveInterpSpeed);
	FRotator NewMeshRot = FMath::RInterpTo(CurrentMeshRot, TargetMeshRot, DeltaTime, MoveInterpSpeed);

	CardMesh->SetRelativeLocation(NewMeshLoc);
	CardMesh->SetRelativeRotation(NewMeshRot);

	// --- [도착 판정] Root와 Mesh가 모두 목표에 도달했는지 확인 ---
	bool bRootArrived = FVector::DistSquared(NewRootLoc, TargetRootLoc) < TransformTolerance &&
		CurrentRootRot.Equals(TargetRootRot, TransformTolerance);

	bool bMeshArrived = FVector::DistSquared(NewMeshLoc, TargetMeshLoc) < TransformTolerance &&
		CurrentMeshRot.Equals(TargetMeshRot, TransformTolerance);

	if (bRootArrived && bMeshArrived)
	{
		// 스냅을 통한 미세 오차 보정
		SetActorRelativeLocation(TargetRootLoc);
		SetActorRelativeRotation(TargetRootRot);

		CardMesh->SetRelativeLocation(TargetMeshLoc);
		CardMesh->SetRelativeRotation(TargetMeshRot);

		// 호버링 중이 아닐 때만 틱 연산을 정지하여 성능을 최적화합니다.
		if (!bIsHovered)
		{
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


// TODO: CardUIWidget 내부의 UserWidget(UPE_CardWidget)을 가져와서 
// CardData->CardName, CardData->CostAP 등을 텍스트 박스에 쏴주는 로직 추가
void APE_CardActor::InitializeCard(UPE_CardInstance* InCardInstance)
{
	if (!InCardInstance) return;

	CardInstance = InCardInstance;

	// 인스턴스 안에 들어있는 원본 데이터를 꺼내서 시각적 세팅을 합니다.
	UPE_CardData* BaseData = CardInstance->GetBaseCardData();

	if (DynamicMaterial && BaseData && BaseData->CardArt)
	{
		DynamicMaterial->SetTextureParameterValue(TEXT("CardArt"), BaseData->CardArt);
	}

	// 위젯 컴포넌트 업데이트
	if (CardUIWidget)
	{
		UUserWidget* BaseWidget = CardUIWidget->GetUserWidgetObject();

		if (UPE_CardWidget* MyCardWidget = Cast<UPE_CardWidget>(BaseWidget))
		{
			MyCardWidget->UpdateCardUI(BaseData); 
		}

		// 위젯의 실시간 화면(Render Target)을 가져오기
		if (UTextureRenderTarget2D* UIRenderTarget = CardUIWidget->GetRenderTarget())
		{
			if (DynamicMaterial)
			{
				DynamicMaterial->SetTextureParameterValue(TEXT("CardUITexture"), UIRenderTarget);
				UE_LOG(LogTemp, Warning, TEXT("[CardActor] UI의 Render Target 가져오기"));
			}
			else UE_LOG(LogTemp, Warning, TEXT("[CardActor] DynamicMaterial 없음"));
		}
		else UE_LOG(LogTemp, Warning, TEXT("[CardActor] GetRenderTarget 실패"));
	}
	UE_LOG(LogTemp, Warning, TEXT("[CardActor] %s 카드 생성 및 UI 매핑 완료"), BaseData ? *BaseData->CardName.ToString() : TEXT("Unknown"));
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

void APE_CardActor::SetHoverOffsetEnabled(bool bEnable)
{
	// 호버링 트리거: 이동 연산을 즉시 시작하여 호버링 오프셋 적용
	bIsHovered = bEnable;
	bIsMovingToTarget = true; // 이동 연산 활성화
}