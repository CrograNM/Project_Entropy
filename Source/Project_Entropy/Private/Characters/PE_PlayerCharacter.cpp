// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

APE_PlayerCharacter::APE_PlayerCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 캐릭터 회전
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f); // 회전 속도
	
	// 스프링암 컴포넌트 생성 및 설정
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1000.f; 
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); 
	CameraBoom->bDoCollisionTest = false;			// 카메라가 장애물에 가려져도 당겨지지 않도록 설정 (전술 뷰 유지)
	CameraBoom->bUsePawnControlRotation = false;	// 컨트롤러 회전에 카메라가 돌아가지 않도록 고정

	// 카메라 컴포넌트 생성 및 설정
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	
	GridMovement = CreateDefaultSubobject<UACGridMovementComponent>(TEXT("GridMovement"));
}

void APE_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// 플레이어 배치 위치 기반 최초 그리드 위치 자동 설정
	if (AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass()))
	{
		if (AACGridSystem* GridSystem = Cast<AACGridSystem>(FoundGridActor))
		{
			FVector PlayerLoc = GetActorLocation();
			AACTile* ClosestTile = nullptr;
			float MinDistance = MAX_FLT;

			// 이전에 구현해둔 GridSystem 내부의 map_data(GridTiles)를 순회하며 가장 가까운 타일을 찾습니다.
			TArray<AActor*> FoundTiles;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AACTile::StaticClass(), FoundTiles);

			for (AActor* Actor : FoundTiles)
			{
				AACTile* Tile = Cast<AACTile>(Actor);
				if (Tile)
				{
					float Dist = FVector::DistSquared(PlayerLoc, Tile->GetActorLocation());
					if (Dist < MinDistance)
					{
						MinDistance = Dist;
						ClosestTile = Tile;
					}
				}
			}

			// 가장 가까운 타일을 찾았다면, 해당 타일의 정중앙으로 캐릭터 위치를 스냅하고 그리드 좌표를 동기화합니다.
			if (ClosestTile)
			{
				GridMovement->SetGridPosition(ClosestTile->GetGridPosition());
				
				FVector SnapLocation = ClosestTile->GetCenterWorldLocation();
				// 캐릭터 모델 피벗에 맞춰 Z축 높이만 플레이어 본래 높이 유지
				SnapLocation.Z = PlayerLoc.Z; 
				
				SetActorLocation(SnapLocation);

				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerCharacter::BeginPlay] 연동 완료: (%d, %d)"), GridMovement->GetGridPosition().X, GridMovement->GetGridPosition().Y);
			}
		}
	}
}

void APE_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APE_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}