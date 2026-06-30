// Copyright CrograNM


#include "Characters/PE_PlayerCharacter.h"

#include "Camera/CameraComponent.h"
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
				GridPosition = ClosestTile->GetGridPosition();
				
				FVector SnapLocation = ClosestTile->GetCenterWorldLocation();
				// 캐릭터 모델 피벗에 맞춰 Z축 높이만 플레이어 본래 높이 유지
				SnapLocation.Z = PlayerLoc.Z; 
				
				SetActorLocation(SnapLocation);

				UE_LOG(LogTemp, Warning, TEXT("[APE_PlayerCharacter::BeginPlay] 플레이어 초기 그리드 좌표 자동 등록 완료: (%d, %d)"), GridPosition.X, GridPosition.Y);
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

void APE_PlayerCharacter::MoveAlongPath(const TArray<class AACTile*>& InPath)
{
	if (InPath.Num() == 0) return;

	SavedPath = InPath;
	CurrentPathIndex = 0;
	bIsMovingOnGrid = true;

	ProcessNextPathStep();
}

void APE_PlayerCharacter::ProcessNextPathStep()
{
	if (CurrentPathIndex < SavedPath.Num())
	{
		TargetWorldLocation = SavedPath[CurrentPathIndex]->GetCenterWorldLocation();
		
		// 실제 상용 개발 단계에서는 은은한 언리얼 툴 내 "AActor::SetActorLocation"을 
		// 매 틱마다 Lerp/VInterpTo 하거나, AI MoveTo를 사용하는 래퍼(Wrapper) 틱 구동이 필요합니다.
		// 임시 프로토타입용 강제 텔레포트/이동 로직 구현 공간:
		SetActorLocation(TargetWorldLocation); 

		// 해당 칸 도달 완료 처리 후 좌표 갱신
		GridPosition = SavedPath[CurrentPathIndex]->GetGridPosition();

		CurrentPathIndex++;
		
		// 약간의 딜레이 후 다음 칸 처리를 유도하거나 틱(Tick) 내부에서 정밀 간격 연산 수행
		// 여기서는 프로토타입이므로 즉시 다음 단계를 호출하지만, 실제로는 부드러운 이동 시간(Interp)을 줍니다.
		ProcessNextPathStep();
	}
	else
	{
		// 최종 목적지에 도착 완료 시점! -> 도착 타일의 불빛을 원래대로 원상복구합니다.
		if (SavedPath.Num() > 0)
		{
			SavedPath.Last()->SetHighlightState(ETileHighlightType::None);
		}
		bIsMovingOnGrid = false;
		SavedPath.Empty();
	}
}

