// Copyright CrograNM

#include "Characters/PE_EnemyBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Components/ACSkillComponent.h"
#include "Core/PE_SkillBase.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

APE_EnemyBase::APE_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SkillComponent = CreateDefaultSubobject<UACSkillComponent>(TEXT("SkillComponent"));
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

	// 사고 루프 가동
	EvaluateAndTakeAction();
}

void APE_EnemyBase::EvaluateAndTakeAction()
{
	// 1. AP가 없거나 죽었다면 즉시 턴 종료
	if (StatComponent->GetCurrentAP() <= 0 || StatComponent->IsDead())
	{
		FinishTurn();
		return;
	}
	
	APE_PlayerCharacter* Player = Cast<APE_PlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));

	if (!Player || !GridSystem) 
	{
		FinishTurn();
		return;
	}
	
	// 내 위치와 플레이어 위치 계산
	FIntPoint MyPos = GridMovement->GetGridPosition();
	FIntPoint PlayerPos = Player->GetGridMovementComponent()->GetGridPosition();
	int32 DistanceToPlayer = FMath::Abs(MyPos.X - PlayerPos.X) + FMath::Abs(MyPos.Y - PlayerPos.Y);
	
	// 2. 첫 번째 스킬(무기)이 있는지 확인합니다.
	if (SkillComponent->GetActiveSkills().Num() > 0)
	{
		UPE_SkillBase* MainSkill = SkillComponent->GetActiveSkills()[0];
		AACTile* PlayerTile = GridSystem->GetTileAtPosition(PlayerPos);

		// [판단 1]: 거리가 닿고 AP가 충분하다면 공격!
		if (DistanceToPlayer <= MainSkill->Range && StatComponent->GetCurrentAP() >= MainSkill->APCost)
		{
			// 타겟에게 스킬 발동 (내부에서 알아서 AP 차감됨)
			if (SkillComponent->TryExecuteSkill(0, PlayerTile, Player))
			{
				// 스킬을 썼으므로, 남은 AP로 힐을 하거나 더 때릴 수 있는지 다시 스스로 재평가
				EvaluateAndTakeAction(); 
				return;
			}
		}
	}
	
	// 3. [판단 2]: 때릴 수 없다면, 플레이어를 향해 이동
	TArray<AACTile*> FullPath = GridSystem->CalculatePath(MyPos, PlayerPos);
	TArray<AACTile*> MovePath;
	
	// 1 AP로 갈 수 있는 최대 거리는 온전히 내 MoveRange 스탯
	int32 MoveRange = StatComponent->GetMoveRange();
	
	// 갈 수 있는 만큼 경로 자르기
	for (int32 i = 0; i < FullPath.Num(); ++i)
	{
		if (FullPath[i]->GetGridPosition() == PlayerPos) break; // 플레이어 밟기 방지
		
		MovePath.Add(FullPath[i]);
		if (MovePath.Num() >= MoveRange) break; // 1AP 이동력 한계까지만 자르기
	}
	
	if (MovePath.Num() > 0)
	{
		// 거리에 상관없이(1칸이든 끝까지 가든) 이동이라는 '행동' 자체에 딱 1 AP만 소모합니다.
		if (StatComponent->ConsumeAP(1))
		{
			UE_LOG(LogTemp, Log, TEXT("[EnemyAI] %d칸 이동을 결심. (비용: 1 AP, 남은 AP: %d)"), MovePath.Num(), StatComponent->GetCurrentAP());
			GridMovement->MoveAlongPath(MovePath);
			// 걷기 시작! (완료되면 OnMovementCompleted 발동 후 다시 루프 진입)
			return;
		}
	}

	// 4. 할 수 있는 게 아무것도 없으면 턴 종료
	FinishTurn();
}

void APE_EnemyBase::OnMovementCompleted()
{
	// 이동이 끝났으니, 이제 때릴 수 있는지 다시 루프를 굴립니다.
	EvaluateAndTakeAction();
}

void APE_EnemyBase::FinishTurn()
{
	UE_LOG(LogTemp, Log, TEXT("[EnemyBase] %s 의 행동 완료 및 턴 종료."), *GetName());
	OnTurnFinished.Broadcast();
}