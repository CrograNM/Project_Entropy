// Copyright CrograNM

#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h" // 캡슐 컴포넌트 제어를 위해 추가

APE_CharacterBase::APE_CharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 이동 및 회전 기본 설정
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f); 

	if (GetCapsuleComponent())
	{
		// 다른 캐릭터(Pawn 채널)와의 물리적 충돌(Block)을 무시(Ignore)로 변경
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// 핵심 컴포넌트 생성
	GridMovement = CreateDefaultSubobject<UACGridMovementComponent>(TEXT("GridMovement"));
	StatComponent = CreateDefaultSubobject<UACStatComponent>(TEXT("StatComponent"));
}

void APE_CharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// 사망 이벤트 바인딩
	if (StatComponent)
	{
		StatComponent->OnDeath.AddDynamic(this, &APE_CharacterBase::HandleDeath);
	}

	SnapCharacterToNearestTile();
}

void APE_CharacterBase::SnapCharacterToNearestTile()
{
	// 모든 캐릭터(플레이어/적) 레벨 배치 시 가장 가까운 타일로 스냅
	if (AActor* FoundGridActor = UGameplayStatics::GetActorOfClass(GetWorld(), AACGridSystem::StaticClass()))
	{
		if (AACGridSystem* GridSystem = Cast<AACGridSystem>(FoundGridActor))
		{
			FVector Loc = GetActorLocation();
			AACTile* ClosestTile = nullptr;
			float MinDistance = MAX_FLT;

			TArray<AActor*> FoundTiles;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AACTile::StaticClass(), FoundTiles);

			for (AActor* Actor : FoundTiles)
			{
				if (AACTile* Tile = Cast<AACTile>(Actor))
				{
					float Dist = FVector::DistSquared(Loc, Tile->GetActorLocation());
					if (Dist < MinDistance)
					{
						MinDistance = Dist;
						ClosestTile = Tile;
					}
				}
			}

			if (ClosestTile && GridMovement)
			{
				GridMovement->SetGridPosition(ClosestTile->GetGridPosition());
				FVector SnapLocation = ClosestTile->GetCenterWorldLocation();
				SnapLocation.Z = Loc.Z; 
				SetActorLocation(SnapLocation);
			}
		}
	}
}

float APE_CharacterBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	// 1. 부모 클래스의 기본 데미지 처리 (반드시 호출)
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.f && StatComponent)
	{
		// 2. StatComponent에게 실제 데미지를 전달하여 체력을 깎음
		// (UACStatComponent 내부에 TakeDamage 함수가 구현되어 있다고 가정합니다)
		StatComponent->TakeDamage(ActualDamage);

		UE_LOG(LogTemp, Warning, TEXT("[%s]가 %f 의 데미지를 받았습니다!"), *GetName(), ActualDamage);

		// 3. 만약 체력이 0 이하가 되었다면 사망 처리 로직 호출
		if (StatComponent->IsDead())
		{
			// Die(); 
		}
	}

	return ActualDamage;
}

void APE_CharacterBase::HandleDeath()
{
	UE_LOG(LogTemp, Warning, TEXT("[%s] 사망 처리되었습니다."), *GetName());
	// 충돌체 끄기, 랙돌 전환 또는 파괴 로직 등의 공통 처리를 이곳에서 진행합니다.
}
