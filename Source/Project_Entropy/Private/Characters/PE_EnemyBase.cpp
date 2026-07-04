// Copyright CrograNM

#include "Characters/PE_EnemyBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Kismet/GameplayStatics.h"

APE_EnemyBase::APE_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	// 적 식별용 메시나 머티리얼 세팅 등...
}

void APE_EnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 이동 완료 델리게이트 바인딩 (이동 컴포넌트가 목표 도달 시 OnMovementCompleted 호출)
	if (GridMovement)
	{
		GridMovement->OnMovementFinished.AddDynamic(this, &APE_EnemyBase::OnMovementCompleted);
	}
}

void APE_EnemyBase::StartTurn()
{
	if (!StatComponent || StatComponent->IsDead())
	{
		FinishTurn();
		return;
	}

	StatComponent->ResetAP(); // 턴 시작 시 AP 회복
	UE_LOG(LogTemp, Warning, TEXT("[EnemyBase] %s 의 턴이 시작되었습니다."), *GetName());

	ProcessAI();
}

void APE_EnemyBase::ProcessAI()
{
	APE_PlayerCharacter* Player = Cast<APE_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));

	if (!Player || !GridSystem || !StatComponent->ConsumeAP(1)) // 이동용 AP 1 소모
	{
		FinishTurn(); // 플레이어나 그리드가 없거나 AP가 없으면 즉시 턴 종료
		return;
	}

	FIntPoint MyPos = GridMovement->GetGridPosition();
	FIntPoint PlayerPos = Player->GetGridMovementComponent()->GetGridPosition();

	// 플레이어까지의 전체 경로 계산
	TArray<AACTile*> FullPath = GridSystem->CalculatePath(MyPos, PlayerPos);
	
	TArray<AACTile*> MovePath;
	int32 MoveRange = StatComponent->GetMoveRange();

	// 플레이어 위치 바로 앞까지만(혹은 내 이동력 한계까지만) 경로를 자릅니다.
	for (int32 i = 0; i < FullPath.Num(); ++i)
	{
		// 플레이어가 서 있는 타일은 밟을 수 없으므로 직전에 멈춤
		if (FullPath[i]->GetGridPosition() == PlayerPos) break;
		
		MovePath.Add(FullPath[i]);
		if (MovePath.Num() >= MoveRange) break; // 이동력 한계 도달
	}

	if (MovePath.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] %s 이(가) 플레이어를 향해 %d칸 이동합니다."), *GetName(), MovePath.Num());
		GridMovement->MoveAlongPath(MovePath); // 이동 지시 
		// (이후 이동이 완료되면 OnMovementCompleted 가 자동으로 호출됨)
	}
	else
	{
		// 움직일 필요가 없거나 막혀있다면 바로 공격 페이즈로
		EvaluateAttackAndEndTurn();
	}
}

void APE_EnemyBase::OnMovementCompleted()
{
	// 이동 컴포넌트의 콜백: 이동이 끝났으므로 공격 가능 여부 판단
	EvaluateAttackAndEndTurn();
}

void APE_EnemyBase::EvaluateAttackAndEndTurn()
{
	APE_PlayerCharacter* Player = Cast<APE_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (Player && StatComponent && StatComponent->GetCurrentAP() >= 1)
	{
		FIntPoint MyPos = GridMovement->GetGridPosition();
		FIntPoint PlayerPos = Player->GetGridMovementComponent()->GetGridPosition();

		// 맨해튼 거리로 공격 사거리 확인
		int32 DistanceToPlayer = FMath::Abs(MyPos.X - PlayerPos.X) + FMath::Abs(MyPos.Y - PlayerPos.Y);

		if (DistanceToPlayer <= AttackRange)
		{
			StatComponent->ConsumeAP(1); // 공격용 AP 소모
			
			// TODO: 플레이어의 StatComponent->ApplyDamage() 호출 및 애니메이션 재생
			UE_LOG(LogTemp, Error, TEXT("[EnemyAI] %s 이(가) 플레이어에게 쾅! (남은AP: %d)"), *GetName(), StatComponent->GetCurrentAP());
		}
	}

	FinishTurn();
}

void APE_EnemyBase::FinishTurn()
{
	UE_LOG(LogTemp, Log, TEXT("[EnemyBase] %s 의 턴 종료."), *GetName());
	OnTurnFinished.Broadcast();
}