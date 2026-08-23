// Copyright CrograNM

#include "CardSystem/PE_SkillEffectModule.h"
#include "CardSystem/PE_SkillData.h"
#include "Characters/PE_CharacterBase.h"
#include "Components/ACGridMovementComponent.h"
#include "Components/ACStatComponent.h"
#include "Grid/ACGridSystem.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Queue.h"
#include "Core/PE_GameState.h" 

// --- [모듈 1: 데미지 구현부] ---
void UPE_SkillEffect_Damage::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	for (APE_CharacterBase* Target : Targets)
	{
		if (Target && Instigator)
		{
			UGameplayStatics::ApplyDamage(Target, CalculatedDamage, Instigator->GetInstigatorController(), Instigator, UDamageType::StaticClass());
		}
	}
}

// --- [모듈 2: 넉백 구현부 (실제 적용 로직)] ---
void UPE_SkillEffect_Push::ApplyEffects(AActor* Instigator, const TSet<APE_CharacterBase*>& Targets, const FVector& TargetLocation, const UPE_SkillData* InSkillData, float CalculatedDamage)
{
	if (Targets.IsEmpty() || !Instigator || PushDistance <= 0) return;

	AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(Instigator, AACGridSystem::StaticClass()));
	UACGridMovementComponent* InstMove = Cast<APE_CharacterBase>(Instigator)->GetGridMovementComponent();
	if (!GridSystem || !InstMove) return;

	FIntPoint InstPos = InstMove->GetGridPosition();

	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GridSystem->GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;
			if (UACGridMovementComponent* CharMove = Char->GetGridMovementComponent())
			{
				FIntPoint Pos = (CharMove->GetTargetGridPosition() != FIntPoint(-999, -999))
					? CharMove->GetTargetGridPosition() : CharMove->GetGridPosition();
				CurrentPosMap.Add(Char, Pos);
			}
		}
	}

	// TargetLocation 벡터를 기반으로 스킬이 떨어진 중심(그리드 좌표)을 유추합니다.
	FIntPoint SkillTargetGridPos(-999, -999);
	TArray<AActor*> TilesArr;
	UGameplayStatics::GetAllActorsOfClass(GridSystem->GetWorld(), AACTile::StaticClass(), TilesArr);
	for (AActor* Actor : TilesArr)
	{
		if (FVector::DistXY(Actor->GetActorLocation(), TargetLocation) < 50.f)
		{
			SkillTargetGridPos = Cast<AACTile>(Actor)->GetGridPosition();
			break;
		}
	}
	if (SkillTargetGridPos == FIntPoint(-999, -999)) SkillTargetGridPos = InstPos; // 예비 보정

	FVector Dir3D = (TargetLocation - Instigator->GetActorLocation()).GetSafeNormal2D();
	if (Dir3D.IsNearlyZero()) Dir3D = Instigator->GetActorForwardVector();
	FVector2D DirV(Dir3D.X, Dir3D.Y);
	DirV.Normalize();

	float Angle = FMath::Atan2(DirV.Y, DirV.X);
	int32 DirIdx = FMath::RoundToInt(Angle / (PI / 2.f));

	FIntPoint DirectionalDir(0, 0);
	if (DirIdx == 1) DirectionalDir = FIntPoint(0, 1);
	else if (DirIdx == 2 || DirIdx == -2) DirectionalDir = FIntPoint(-1, 0);
	else if (DirIdx == -1) DirectionalDir = FIntPoint(0, -1);
	else DirectionalDir = FIntPoint(1, 0);

	struct FSimulatedPush {
		APE_CharacterBase* Actor;
		int32 RemainingDist;
		FIntPoint PushDir;
		float Delay;
	};
	TArray<FSimulatedPush> PendingPushes;

	for (APE_CharacterBase* Target : Targets)
	{
		if (!Target->IsPushable()) continue;
		if (!CurrentPosMap.Contains(Target)) continue;

		FIntPoint TargetGridPos = CurrentPosMap[Target];
		FIntPoint FinalPushDir(0, 0);

		if (PushType == EPEPushType::Directional)
		{
			FinalPushDir = DirectionalDir;
		}
		else // 방사형(Radial) 로직 고도화
		{
			if (TargetGridPos == SkillTargetGridPos)
			{
				// 정확히 중심에 있는 녀석은 시전자가 바라본 방향으로 넉백
				FinalPushDir = FIntPoint(
					FMath::Clamp(TargetGridPos.X - InstPos.X, -1, 1),
					FMath::Clamp(TargetGridPos.Y - InstPos.Y, -1, 1)
				);
			}
			else
			{
				// 그 외 범위 안의 적들은 폭발 중심점(Target) 기준으로 방사형으로 밀려남
				FinalPushDir = FIntPoint(
					FMath::Clamp(TargetGridPos.X - SkillTargetGridPos.X, -1, 1),
					FMath::Clamp(TargetGridPos.Y - SkillTargetGridPos.Y, -1, 1)
				);
			}

			if (FMath::Abs(FinalPushDir.X) == 1 && FMath::Abs(FinalPushDir.Y) == 1) FinalPushDir.Y = 0;
		}

		if (FinalPushDir.X != 0 || FinalPushDir.Y != 0)
		{
			PendingPushes.Add({ Target, PushDistance, FinalPushDir, 0.f });
		}
	}

	while (PendingPushes.Num() > 0)
	{
		PendingPushes.Sort([&CurrentPosMap](const FSimulatedPush& A, const FSimulatedPush& B) {
			FIntPoint PosA = CurrentPosMap[A.Actor];
			FIntPoint PosB = CurrentPosMap[B.Actor];
			int32 DotA = PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y;
			int32 DotB = PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y;
			return DotA > DotB;
			});

		FSimulatedPush Task = PendingPushes[0];
		PendingPushes.RemoveAt(0);

		if (!CurrentPosMap.Contains(Task.Actor)) continue;

		FIntPoint CurrentPos = CurrentPosMap[Task.Actor];
		TArray<AACTile*> KnockbackPath;

		bool bHitSomething = false;
		APE_CharacterBase* HitCharacter = nullptr;
		float TimePerTile = 100.f / Task.Actor->GetGridMovementComponent()->GetGridMoveSpeed();

		for (int32 step = 1; step <= Task.RemainingDist; ++step)
		{
			FIntPoint NextPos = CurrentPos + Task.PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			if (!NextTile || NextTile->IsObstacle())
			{
				bHitSomething = true;
				break;
			}

			APE_CharacterBase* CollidedChar = nullptr;
			for (const auto& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == NextPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				bHitSomething = true;
				HitCharacter = CollidedChar;

				if (CollidedChar->IsPushable())
				{
					float HitDelay = Task.Delay + (step * TimePerTile);
					PendingPushes.Add({ CollidedChar, Task.RemainingDist - step, Task.PushDir, HitDelay });
				}
				break;
			}

			CurrentPos = NextPos;
			KnockbackPath.Add(NextTile);
		}

		FGridKnockbackPayload Payload;
		Payload.bIsActive = true;
		Payload.Instigator = Instigator;
		Payload.SkillData = InSkillData;

		if (bHitSomething)
		{
			float ScaledRatio = (float)Task.RemainingDist / (float)PushDistance;
			float FinalDamageRatio = CollisionDamageRatio * ScaledRatio;

			if (HitCharacter)
			{
				if (UACStatComponent* HitStat = HitCharacter->GetStatComponent())
				{
					Payload.TargetDamage = HitStat->GetMaxHP() * FinalDamageRatio;
					Payload.OtherDamage = Payload.TargetDamage;
					Payload.HitCharacter = HitCharacter;
				}
			}
			else
			{
				if (UACStatComponent* MyStat = Task.Actor->GetStatComponent())
				{
					Payload.TargetDamage = MyStat->GetMaxHP() * FinalDamageRatio;
				}
			}
		}

		// 개별 밀치기가 계산될 때마다 UI에 등록
		if (APE_GameState* GS = Instigator->GetWorld()->GetGameState<APE_GameState>())
		{
			FString LogText = FString::Printf(TEXT("%s - %d칸 밀림"), *Task.Actor->GetName(), Task.RemainingDist);
			Payload.ActionLogID = GS->AddActionLog(Task.Actor->GetTeamID(), LogText);
			GS->ReportActionStarted();
		}

		if (UACGridMovementComponent* MoveComp = Task.Actor->GetGridMovementComponent())
		{
			MoveComp->NetMulticast_MoveAlongPath(KnockbackPath, false, Task.Delay, Payload);

			if (KnockbackPath.Num() > 0)
			{
				CurrentPosMap[Task.Actor] = CurrentPos;
			}
		}
	}
}

// --- [모듈 2: 시각화를 위한 넉백 시뮬레이션부] ---
TArray<FPushSimulationResult> UPE_SkillEffect_Push::SimulatePush(AACGridSystem* GridSystem, FIntPoint InstigatorPos, FIntPoint TargetPos, const TSet<FIntPoint>& AffectedGridPositions) const
{
	TArray<FPushSimulationResult> Results;
	if (!GridSystem || PushDistance <= 0) return Results;

	TMap<APE_CharacterBase*, FIntPoint> InitialPosMap;
	TMap<APE_CharacterBase*, FIntPoint> CurrentPosMap;
	TMap<APE_CharacterBase*, FIntPoint> PushDirMap;

	TArray<AActor*> AllChars;
	UGameplayStatics::GetAllActorsOfClass(GridSystem->GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

	for (AActor* Actor : AllChars)
	{
		if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
		{
			if (Char->GetStatComponent() && Char->GetStatComponent()->IsDead()) continue;
			if (UACGridMovementComponent* CharMove = Char->GetGridMovementComponent())
			{
				FIntPoint Pos = (CharMove->GetTargetGridPosition() != FIntPoint(-999, -999))
					? CharMove->GetTargetGridPosition() : CharMove->GetGridPosition();

				InitialPosMap.Add(Char, Pos);
				CurrentPosMap.Add(Char, Pos);
			}
		}
	}

	FVector2D DirV(TargetPos.X - InstigatorPos.X, TargetPos.Y - InstigatorPos.Y);
	if (DirV.IsNearlyZero()) DirV = FVector2D(1, 0);
	DirV.Normalize();

	float Angle = FMath::Atan2(DirV.Y, DirV.X);
	int32 DirIdx = FMath::RoundToInt(Angle / (PI / 2.f));

	FIntPoint DirectionalDir(0, 0);
	if (DirIdx == 1) DirectionalDir = FIntPoint(0, 1);
	else if (DirIdx == 2 || DirIdx == -2) DirectionalDir = FIntPoint(-1, 0);
	else if (DirIdx == -1) DirectionalDir = FIntPoint(0, -1);
	else DirectionalDir = FIntPoint(1, 0);

	struct FSimulatedPush {
		APE_CharacterBase* Actor;
		int32 RemainingDist;
		FIntPoint PushDir;
	};
	TArray<FSimulatedPush> PendingPushes;

	for (const FIntPoint& CurrentTargetPos : AffectedGridPositions)
	{
		APE_CharacterBase* HitChar = nullptr;
		for (const auto& Pair : CurrentPosMap)
		{
			if (Pair.Value == CurrentTargetPos)
			{
				HitChar = Pair.Key;
				break;
			}
		}

		if (HitChar && HitChar->IsPushable())
		{
			FIntPoint PushDir(0, 0);

			if (PushType == EPEPushType::Directional)
			{
				PushDir = DirectionalDir;
			}
			else // 시각화에서도 방사형 연산을 TargetPos(스킬 타겟 타일) 기준으로 수행
			{
				if (CurrentTargetPos == TargetPos)
				{
					PushDir = FIntPoint(
						FMath::Clamp(CurrentTargetPos.X - InstigatorPos.X, -1, 1),
						FMath::Clamp(CurrentTargetPos.Y - InstigatorPos.Y, -1, 1)
					);
				}
				else
				{
					PushDir = FIntPoint(
						FMath::Clamp(CurrentTargetPos.X - TargetPos.X, -1, 1),
						FMath::Clamp(CurrentTargetPos.Y - TargetPos.Y, -1, 1)
					);
				}

				if (FMath::Abs(PushDir.X) == 1 && FMath::Abs(PushDir.Y) == 1) PushDir.Y = 0;
			}

			if (PushDir.X != 0 || PushDir.Y != 0)
			{
				PendingPushes.Add({ HitChar, PushDistance, PushDir });
				PushDirMap.Add(HitChar, PushDir);
			}
		}
	}

	while (PendingPushes.Num() > 0)
	{
		// 시각화에서도 실제 처리와 동일하게 Back-to-Front 정렬
		PendingPushes.Sort([&CurrentPosMap](const FSimulatedPush& A, const FSimulatedPush& B) {
			FIntPoint PosA = CurrentPosMap[A.Actor];
			FIntPoint PosB = CurrentPosMap[B.Actor];
			int32 DotA = PosA.X * A.PushDir.X + PosA.Y * A.PushDir.Y;
			int32 DotB = PosB.X * B.PushDir.X + PosB.Y * B.PushDir.Y;
			return DotA > DotB;
			});

		FSimulatedPush Task = PendingPushes[0];
		PendingPushes.RemoveAt(0);

		if (!CurrentPosMap.Contains(Task.Actor)) continue;

		FIntPoint CurrentPos = CurrentPosMap[Task.Actor];

		for (int32 step = 1; step <= Task.RemainingDist; ++step)
		{
			FIntPoint NextPos = CurrentPos + Task.PushDir;
			AACTile* NextTile = GridSystem->GetTileAtPosition(NextPos);

			if (!NextTile || NextTile->IsObstacle()) break;

			APE_CharacterBase* CollidedChar = nullptr;
			for (const auto& Pair : CurrentPosMap)
			{
				if (Pair.Key != Task.Actor && Pair.Value == NextPos)
				{
					CollidedChar = Pair.Key;
					break;
				}
			}

			if (CollidedChar)
			{
				if (CollidedChar->IsPushable())
				{
					PendingPushes.Add({ CollidedChar, Task.RemainingDist - step, Task.PushDir });
					PushDirMap.Add(CollidedChar, Task.PushDir);
				}
				break;
			}

			CurrentPos = NextPos;
		}

		CurrentPosMap[Task.Actor] = CurrentPos;
	}

	for (const auto& Pair : PushDirMap)
	{
		APE_CharacterBase* Char = Pair.Key;
		FPushSimulationResult Result;
		Result.TargetActor = Char;
		Result.StartPos = InitialPosMap[Char];
		Result.EndPos = CurrentPosMap[Char];
		Result.PushDir = Pair.Value;
		Results.Add(Result);
	}

	return Results;
}