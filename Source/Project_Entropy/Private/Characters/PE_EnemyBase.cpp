// Copyright CrograNM

#include "Characters/PE_EnemyBase.h"
#include "Characters/PE_PlayerCharacter.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Components/ACSkillComponent.h"
#include "CardSystem/PE_SkillData.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"

APE_EnemyBase::APE_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 적 캐릭터 복제 활성화

	SkillComponent = CreateDefaultSubobject<UACSkillComponent>(TEXT("SkillComponent"));
}

void APE_EnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && GridMovement)
	{
		GridMovement->OnMovementFinished.AddDynamic(this, &APE_EnemyBase::OnMovementCompleted);
	}
}

void APE_EnemyBase::StartTurn()
{
	// AI 루프는 서버에서만 돌아야 합니다.
	if (!HasAuthority()) return;

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
	if (!HasAuthority()) return;

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
		UPE_SkillData* MainSkill = SkillComponent->GetActiveSkills()[0];
		AACTile* PlayerTile = GridSystem->GetTileAtPosition(PlayerPos);

		// [판단 1]: 거리가 닿고 AP가 충분하다면 공격!
		if (DistanceToPlayer <= MainSkill->BaseRange && StatComponent->GetCurrentAP() >= MainSkill->BaseAPCost)
		{
			NetMulticast_ShowSkillIntent(PlayerTile);

			// 타이머 설정
			PendingSkillIndex = 0;
			PendingSkillTargetTile = PlayerTile;
			PendingSkillTargetCharacter = Player;

			GetWorldTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_EnemyBase::ExecutePendingSkill, ActionDelay, false);

			return;
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
			// 멀티캐스트를 통해 클라이언트들에게 적의 예상 이동 경로 표시
			NetMulticast_ShowMoveIntent(MyPos, MoveRange, MovePath.Last());

			// N초 후 이동하도록 시각화 처리
			// 1. 반경 및 경로 시각화
			TArray<AACTile*> RangeTiles = GridSystem->ShowMovementRange(MyPos, MoveRange);
			GridSystem->HighlightPath(MyPos, MovePath.Last()->GetGridPosition(), RangeTiles);

			// 2. 타이머로 실제 이동 지연
			PendingMovePath = MovePath;
			GetWorldTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_EnemyBase::ExecutePendingMovement, ActionDelay, false);

			return;
		}
	}

	// 4. 할 수 있는 게 아무것도 없으면 턴 종료
	FinishTurn();
}


void APE_EnemyBase::ExecutePendingSkill()
{
	NetMulticast_ClearIntent();

	if (PendingSkillIndex >= 0 && PendingSkillTargetTile)
	{
		SkillComponent->TryExecuteSkill(PendingSkillIndex, PendingSkillTargetTile, PendingSkillTargetCharacter);
	}

	EvaluateAndTakeAction();
}

void APE_EnemyBase::ExecutePendingMovement()
{
	NetMulticast_ClearIntent();

	if (PendingMovePath.Num() > 0)
	{
		GridMovement->NetMulticast_MoveAlongPath(PendingMovePath);
		// 걷기 시작! (완료되면 OnMovementCompleted 발동 후 다시 루프 진입)
	}
	else
	{
		EvaluateAndTakeAction(); // 만약 경로가 비어있다면 루프 재진입 (예외 처리)
	}
}

void APE_EnemyBase::OnMovementCompleted()
{
	if (HasAuthority())
	{
		EvaluateAndTakeAction();
	}
}

void APE_EnemyBase::FinishTurn()
{
	UE_LOG(LogTemp, Log, TEXT("[EnemyBase] %s 의 행동 완료 및 턴 종료."), *GetName());
	OnTurnFinished.Broadcast();
}

/* --- 멀티캐스트 시각화 구현부 --- */
void APE_EnemyBase::NetMulticast_ShowSkillIntent_Implementation(AACTile* TargetTile)
{
	if (TargetTile)
	{
		TargetTile->SetHighlightState(ETileHighlightType::SkillTarget);
	}
}

void APE_EnemyBase::NetMulticast_ShowMoveIntent_Implementation(FIntPoint StartPos, int32 MoveRange, AACTile* DestinationTile)
{
	if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
	{
		TArray<AACTile*> RangeTiles = GridSystem->ShowMovementRange(StartPos, MoveRange);
		GridSystem->HighlightPath(StartPos, DestinationTile->GetGridPosition(), RangeTiles);
	}
}

void APE_EnemyBase::NetMulticast_ClearIntent_Implementation()
{
	if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
	{
		GridSystem->ClearAllHighlights();
	}
}