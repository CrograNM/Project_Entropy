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
}

FVector AACTile::GetCenterWorldLocation() const
{
	// 타일 메쉬의 중심 월드 좌표를 반환하되, 캐릭터가 발을 딛는 높이(Z축 부모 위치 등)를 고려하여 반환
	return GetActorLocation();
}