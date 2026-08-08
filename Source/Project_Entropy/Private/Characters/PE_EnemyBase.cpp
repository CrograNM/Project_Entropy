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
#include "Core/PE_GameState.h"

APE_EnemyBase::APE_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // 적 캐릭터 복제 활성화

	// 몬스터는 기본적으로 1팀(적대 세력)으로 간주
	TeamID = 1;
}

void APE_EnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && GridMovement)
	{
		GridMovement->OnMovementFinished.AddDynamic(this, &APE_EnemyBase::OnMovementCompleted);
	}
}

void APE_EnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(ActionDelayTimerHandle);
		GetWorldTimerManager().ClearTimer(QueueWaitTimerHandle);
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

	// 밀치기 등으로 강제 이동이 끝났을 때 턴이 무시되는 현상 방어
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		if (UPE_TurnManagerComponent* TM = GS->GetTurnManager())
		{
			// 현재 전투 페이즈가 '팀 턴'이 아니거나, '내 팀(1팀)'의 차례가 아니라면 무조건 차단!
			if (TM->GetCurrentPhase() != EPEBattlePhase::TeamTurn || TM->GetCurrentTeamTurn() != GetTeamID())
			{
				return;
			}
		}
	}

	// 1. AP가 없거나 죽었다면 즉시 턴 종료
	if (StatComponent->GetCurrentAP() <= 0 || StatComponent->IsDead())
	{
		FinishTurn();
		return;
	}

	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
	if (!GridSystem)
	{
		FinishTurn();
		return;
	}

	// 맵 위의 모든 플레이어를 탐색하여 가장 가까운 대상을 찾습니다.
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_PlayerCharacter::StaticClass(), FoundPlayers);

	APE_PlayerCharacter* TargetPlayer = nullptr;
	int32 MinDistance = MAX_int32;
	FIntPoint MyPos = GridMovement->GetGridPosition();

	for (AActor* Actor : FoundPlayers)
	{
		APE_PlayerCharacter* PC = Cast<APE_PlayerCharacter>(Actor);
		if (PC && PC->GetStatComponent() && !PC->GetStatComponent()->IsDead())
		{
			FIntPoint PlayerPos = PC->GetGridMovementComponent()->GetGridPosition();
			int32 DistanceToPlayer = FMath::Abs(MyPos.X - PlayerPos.X) + FMath::Abs(MyPos.Y - PlayerPos.Y);

			if (DistanceToPlayer < MinDistance)
			{
				MinDistance = DistanceToPlayer;
				TargetPlayer = PC;
			}
		}
	}

	// 타겟 플레이어가 없다면 턴 종료
	if (!TargetPlayer)
	{
		FinishTurn();
		return;
	}

	FIntPoint PlayerPos = TargetPlayer->GetGridMovementComponent()->GetGridPosition();

	// 2. 스킬 사용 판단 로직
	if (SkillComponent->GetActiveSkills().Num() > 0)
	{
		UPE_SkillData* MainSkill = SkillComponent->GetActiveSkills()[0];
		AACTile* PlayerTile = GridSystem->GetTileAtPosition(PlayerPos);

		if (MinDistance <= MainSkill->BaseRange && StatComponent->GetCurrentAP() >= MainSkill->BaseAPCost)
		{
			NetMulticast_ShowSkillIntent(PlayerTile);
			PendingSkillIndex = 0;
			PendingSkillTargetTile = PlayerTile;
			PendingSkillTargetCharacter = TargetPlayer;

			GetWorldTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_EnemyBase::ExecutePendingSkill, ActionDelay, false);
			return;
		}
	}

	// 3. 이동 판단 로직
	TArray<AACTile*> FullPath = GridSystem->CalculatePath(MyPos, PlayerPos);
	TArray<AACTile*> MovePath;
	int32 MoveRange = StatComponent->GetMoveRange();

	for (int32 i = 0; i < FullPath.Num(); ++i)
	{
		if (FullPath[i]->GetGridPosition() == PlayerPos) break;
		MovePath.Add(FullPath[i]);
		if (MovePath.Num() >= MoveRange) break;
	}

	if (MovePath.Num() > 0)
	{
		if (StatComponent->ConsumeAP(1))
		{
			NetMulticast_ShowMoveIntent(MyPos, MoveRange, MovePath.Last());
			PendingMovePath = MovePath;
			GetWorldTimerManager().SetTimer(ActionDelayTimerHandle, this, &APE_EnemyBase::ExecutePendingMovement, ActionDelay, false);
			return;
		}
	}

	FinishTurn();
}


void APE_EnemyBase::ExecutePendingSkill()
{
	NetMulticast_ClearIntent();

	if (PendingSkillIndex >= 0 && PendingSkillTargetTile)
	{
		SkillComponent->TryExecuteSkill(PendingSkillIndex, PendingSkillTargetTile, PendingSkillTargetCharacter);
	}

	// 0.1초마다 GameState의 큐가 비워졌는지(투사체가 폭발했는지) 감시하는 타이머를 돌립니다.
	GetWorldTimerManager().SetTimer(QueueWaitTimerHandle, this, &APE_EnemyBase::CheckQueueAndEvaluate, 0.1f, true);
}

void APE_EnemyBase::CheckQueueAndEvaluate()
{
	if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
	{
		// 투사체가 목표에 도달하여 터지면 IsActionQueueActive가 false가 됩니다.
		if (!GS->IsActionQueueActive())
		{
			// 큐가 비워졌음을 확인했으니, 감시 타이머를 끄고 다음 행동(AP 확인 후 스킬/이동/턴종료)을 고민합니다.
			GetWorldTimerManager().ClearTimer(QueueWaitTimerHandle);
			EvaluateAndTakeAction();
		}
	}
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
		if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
		{
			GridSystem->HighlightTarget(this, TargetTile->GetGridPosition());
		}
	}
}

void APE_EnemyBase::NetMulticast_ShowMoveIntent_Implementation(FIntPoint StartPos, int32 MoveRange, AACTile* DestinationTile)
{
	if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
	{
		TArray<AACTile*> RangeTiles = GridSystem->HighlightArea(this,StartPos, MoveRange);
		GridSystem->HighlightPath(this, StartPos, DestinationTile->GetGridPosition(), RangeTiles);
	}
}

void APE_EnemyBase::NetMulticast_ClearIntent_Implementation()
{
	if (AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass())))
	{
		GridSystem->ClearAllHighlightsFor(this);
	}
}