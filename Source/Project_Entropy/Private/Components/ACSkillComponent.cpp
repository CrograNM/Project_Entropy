// Copyright CrograNM

#include "Components/ACSkillComponent.h"
#include "Core/PE_PlayerController.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillActionActor.h"
#include "CardSystem/PE_DataTypes.h"
#include "CardSystem/PE_SkillEffectModule.h"
#include "Components/ACStatComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Core/PE_GameState.h"
#include "Components/CapsuleComponent.h"

UACSkillComponent::UACSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UACSkillComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APE_CharacterBase* OwnerChar = Cast<APE_CharacterBase>(GetOwner()))
	{
		OwnerStatComponent = OwnerChar->GetStatComponent();
	}

	// 에디터에 세팅된 데이터 에셋(UPE_SkillData)을 활성 목록에 등록합니다.
	for (UPE_SkillData* SkillData : DefaultSkills)
	{
		if (SkillData)
		{
			ActiveSkills.Add(SkillData);
		}
	}
}

bool UACSkillComponent::TryExecuteSkill(int32 SkillIndex, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	if (!ActiveSkills.IsValidIndex(SkillIndex) || !OwnerStatComponent) return false;

	UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] 스킬 시도: %d"), SkillIndex);
	UPE_SkillData* SkillData = ActiveSkills[SkillIndex];

	// 몬스터 등이 기본 스킬을 쓸 때는 마석 연산이 없으므로, 데이터 에셋의 BaseDamage를 그대로 넘깁니다.
	return TryExecuteSkillByData(SkillData, TargetTile, TargetCharacter, SkillData->BaseDamage);
}

bool UACSkillComponent::TryExecuteSkillByData(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage, int32 ClientRequestID, bool bIsFreeCast)
{
	if (!GetOwner()->HasAuthority() || !SkillData || !OwnerStatComponent) return false;

	APE_CharacterBase* Caster = Cast<APE_CharacterBase>(GetOwner());

	// 타겟 조건이 맞는지 1차 검증 (TargetType 기반)
	if (SkillData->TargetType == EPESkillTargetType::Tile && !TargetTile)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] 타일 타겟팅 스킬인데 타일이 지정되지 않았습니다."));
		return false;
	}
	if (SkillData->TargetType == EPESkillTargetType::Snap_Enemy && !TargetCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] 대상 지정 스킬인데 대상이 지정되지 않았습니다."));
		return false;
	}

	bool bCanCast = true;
	if (!bIsFreeCast)
	{
		bCanCast = OwnerStatComponent->ConsumeAP(SkillData->BaseAPCost);
	}

	// AP 결제 시도 (AP가 부족하면 실패 처리)
	if (bCanCast)
	{
		// 2. 결제가 성공하면 스킬 로직을 즉시 실행하지 않고, 명세서를 만들어 큐에 집어넣습니다!
		FPESkillActionPayload Payload;
		Payload.Instigator = Caster;
		Payload.TargetTile = TargetTile;
		Payload.TargetCharacter = TargetCharacter;
		Payload.SkillData = SkillData;
		Payload.CalculatedDamage = CalculatedDamage;
		Payload.ClientRequestID = ClientRequestID;
		Payload.bIsFreeCast = bIsFreeCast;

		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
			// 결제가 성공하여 큐에 진입할 때 UI 로그를 생성하고 ID 보관
			FString LogText = FString::Printf(TEXT("%s - %s 시전"), *Caster->GetName(), *SkillData->SkillID.ToString());
			Payload.ActionLogID = GS->AddActionLog(Caster->GetTeamID(), LogText);

			GS->EnqueueSkillAction(Payload);
		}

		return true; // 큐 등록 성공 (손패에서 카드가 버려짐)
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] AP가 부족하여 스킬 발동에 실패했습니다."));
	}

	return false;
}

void UACSkillComponent::PrepareQueuedSkill(const FPESkillActionPayload& Payload)
{
	UPE_SkillData* SkillData = Payload.SkillData;
	APE_CharacterBase* Caster = Payload.Instigator;

	// 게임 스테이트 캐싱
	APE_GameState* GS = nullptr;
	if (Caster && Caster->GetWorld())
	{
		GS = Caster->GetWorld()->GetGameState<APE_GameState>();
	}

	if (!SkillData || !Caster)
	{
		if (GS) GS->EndAction(Payload.ActionTokenID, Payload.ActionLogID);
		return;
	}

	// 타일 지정 vs 캐릭터 대상 분리 및 타겟 위치 확정
	APE_CharacterBase* FinalTargetChar = nullptr;
	FVector OriginalTargetLoc = FVector::ZeroVector;
	FIntPoint TargetGridPos = FIntPoint(-999, -999);

	if (SkillData->TargetType == EPESkillTargetType::Tile)
	{
		if (Payload.TargetTile)
		{
			OriginalTargetLoc = Payload.TargetTile->GetActorLocation();
			OriginalTargetLoc.Z += 20.f;
			TargetGridPos = Payload.TargetTile->GetGridPosition();
		}
	}
	else
	{
		FinalTargetChar = Payload.TargetCharacter;
		if (FinalTargetChar && !FinalTargetChar->GetStatComponent()->IsDead())
		{
			OriginalTargetLoc = FinalTargetChar->GetActorLocation();
			if (UCapsuleComponent* TargetCap = FinalTargetChar->FindComponentByClass<UCapsuleComponent>())
				OriginalTargetLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;

			if (UACGridMovementComponent* MoveComp = FinalTargetChar->GetGridMovementComponent())
				TargetGridPos = MoveComp->GetGridPosition();
		}
	}

	// 사거리 및 타겟 유효성 재검증 (실행 시점 최신 기준)
	bool bIsValidTarget = true;

	if (SkillData->TargetType == EPESkillTargetType::Self || SkillData->TargetType == EPESkillTargetType::All_Enemies)
	{
		bIsValidTarget = true;
	}
	else if (TargetGridPos == FIntPoint(-999, -999))
	{
		bIsValidTarget = false;
	}
	else if (SkillData->TargetType != EPESkillTargetType::Self && SkillData->TargetType != EPESkillTargetType::All_Enemies)
	{
		FIntPoint CasterPos = Caster->GetGridMovementComponent()->GetGridPosition();
		int32 CurrentDistance = FMath::Abs(CasterPos.X - TargetGridPos.X) + FMath::Abs(CasterPos.Y - TargetGridPos.Y);

		if (CurrentDistance > SkillData->BaseRange)
		{
			bIsValidTarget = false;
		}
	}

	// 검증 실패 시: 환불 및 카드 반환 (ID 전송)
	if (!bIsValidTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ACSkillComponent] 실행 시점 재검증 실패. 스킬을 취소합니다."));

		if (!Payload.bIsFreeCast)
		{
			OwnerStatComponent->SetAP(OwnerStatComponent->GetCurrentAP() + SkillData->BaseAPCost);
		}

		if (APE_PlayerController* PC = Cast<APE_PlayerController>(Caster->GetController()))
		{
			PC->Client_CancelSkillExecution(Payload.ClientRequestID);
		}
		if (GS) GS->EndAction(Payload.ActionTokenID, Payload.ActionLogID);
		return;
	}

	// 몬스터의 스킬이거나 ID가 없는 경우(클라이언트 애니메이션이 불필요한 경우) 즉시 스킬 발사 처리
	if (Payload.ClientRequestID == -1)
	{
		CommitQueuedSkill(Payload);
	}
	else
	{
		// 클라이언트에게 산화(버리기) 애니메이션 재생을 명령하고 대기
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(Caster->GetController()))
		{
			PC->Client_PlaySkillAnim(Payload.ClientRequestID);
		}
	}
}

void UACSkillComponent::CommitQueuedSkill(const FPESkillActionPayload& Payload)
{
	UPE_SkillData* SkillData = Payload.SkillData;
	APE_CharacterBase* Caster = Payload.Instigator;

	APE_GameState* GS = nullptr;
	if (Caster && Caster->GetWorld()) GS = Caster->GetWorld()->GetGameState<APE_GameState>();

	if (!SkillData || !Caster || SkillData->HitPhases.IsEmpty())
	{
		if (GS) GS->EndAction(Payload.ActionTokenID, Payload.ActionLogID);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ACSkillComponent] 스킬 실행 확정: %s"), *SkillData->SkillID.ToString());

	// 타겟팅 정보 다시 추적 (방향을 위해)
	FVector OriginalTargetLoc = FVector::ZeroVector;
	APE_CharacterBase* FinalTargetChar = nullptr;

	if (SkillData->TargetType == EPESkillTargetType::Tile && Payload.TargetTile)
	{
		OriginalTargetLoc = Payload.TargetTile->GetActorLocation();
		OriginalTargetLoc.Z += 20.f;
	}
	else if (Payload.TargetCharacter)
	{
		FinalTargetChar = Payload.TargetCharacter;
		OriginalTargetLoc = FinalTargetChar->GetActorLocation();
		if (UCapsuleComponent* TargetCap = FinalTargetChar->FindComponentByClass<UCapsuleComponent>())
			OriginalTargetLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
	}

	// 클라이언트에서 애니메이션이 완전히 종료되었으므로, 논리적인 덱 매니저의 버리기를 확정 명령
	if (Payload.ClientRequestID != -1)
	{
		if (APE_PlayerController* PC = Cast<APE_PlayerController>(Caster->GetController()))
			PC->Client_ConfirmSkillExecution(Payload.ClientRequestID);
	}

	if (SkillData->TargetType != EPESkillTargetType::Self && SkillData->TargetType != EPESkillTargetType::All_Enemies)
	{
		FVector Dir = (OriginalTargetLoc - Caster->GetActorLocation()).GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
			Caster->SetActorRotation(Dir.Rotation());
	}

	FIntPoint CasterPos = Caster->GetGridMovementComponent() ? Caster->GetGridMovementComponent()->GetGridPosition() : FIntPoint(0, 0);
	FIntPoint TargetPos = FIntPoint(-999, -999);

	// 스킬 시전 공통 애니메이션/VFX
	NetMulticast_PlayCastVisuals(SkillData);

	int32 TotalPhases = SkillData->HitPhases.Num();

	// [수정 핵심] 설정된 페이즈 수만큼 루프를 돌며 각각 독립적인 타이머와 타격 범위를 생성합니다.
	for (int32 i = 0; i < TotalPhases; ++i)
	{
		// 마지막 페이즈가 끝날 때 UI의 시전 로그를 함께 날리도록 ID를 분배
		const int32 LogIDToClear = (i == TotalPhases - 1) ? Payload.ActionLogID : -1;

		// 큐 동기화: 페이즈마다 독립적인 추적 토큰을 발급받습니다.
		const int32 PhaseToken = GS
			? GS->BeginAction(FString::Printf(TEXT("SkillPhase:%s[%d/%d]/%s"),
				*SkillData->SkillID.ToString(), i + 1, TotalPhases, *GetNameSafe(Caster)), LogIDToClear)
			: INDEX_NONE;

		// [주의] TargetPos를 참조(&)로 캡처하면 TriggerTime 타이머로 지연 실행될 때
		// 이미 사라진 스택 변수를 건드리게 되므로 반드시 값으로 복사해 둡니다.
		auto ExecutePhaseFunc = [this, Caster, Payload, SkillData, i, CasterPos, TargetPos, OriginalTargetLoc, FinalTargetChar, LogIDToClear, PhaseToken]() mutable
			{
				if (!this || !Caster || !SkillData || !SkillData->HitPhases.IsValidIndex(i))
				{
					if (APE_GameState* CheckGS = GetWorld()->GetGameState<APE_GameState>())
						CheckGS->EndAction(PhaseToken, LogIDToClear);
					return;
				}

				const FPESkillHitPhase& CurrentPhase = SkillData->HitPhases[i];
				float FinalDamage = Payload.CalculatedDamage * CurrentPhase.DamageMultiplier;

				FRotator ExactRotation = Caster->GetActorRotation();
				TSet<APE_CharacterBase*> AffectedTargets;
				FVector PhaseTargetLoc = OriginalTargetLoc;
				APE_CharacterBase* PhaseTargetChar = FinalTargetChar;

				if (SkillData->TargetType == EPESkillTargetType::All_Enemies)
				{
					TArray<AActor*> AllChars;
					UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);
					for (AActor* Actor : AllChars)
					{
						if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
						{
							if (Char->GetTeamID() != Caster->GetTeamID() && Char->GetStatComponent() && !Char->GetStatComponent()->IsDead())
								AffectedTargets.Add(Char);
						}
					}
				}
				else if (SkillData->TargetType == EPESkillTargetType::Self)
				{
					AffectedTargets.Add(Caster);
				}
				else
				{
					if (PhaseTargetChar && PhaseTargetChar->GetGridMovementComponent())
						TargetPos = PhaseTargetChar->GetGridMovementComponent()->GetGridPosition();
					else if (Payload.TargetTile)
						TargetPos = Payload.TargetTile->GetGridPosition();

					if (TargetPos != FIntPoint(-999, -999) && CasterPos != TargetPos)
					{
						FVector2D DirV(TargetPos.X - CasterPos.X, TargetPos.Y - CasterPos.Y);
						DirV.Normalize();
						ExactRotation = FVector(DirV.X, DirV.Y, 0).Rotation();
					}

					if (CurrentPhase.AoEShape == EPEAoEShape::Line && TargetPos != FIntPoint(-999, -999))
					{
						FVector2D CasterV(CasterPos.X, CasterPos.Y);
						FVector2D TargetV(TargetPos.X, TargetPos.Y);
						FVector2D Dir = (TargetV - CasterV).GetSafeNormal();
						if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

						AACGridSystem* GridSystem = Cast<AACGridSystem>(UGameplayStatics::GetActorOfClass(this, AACGridSystem::StaticClass()));
						if (GridSystem)
						{
							FIntPoint LastValidPos = CasterPos;
							for (int32 step = 1; step <= SkillData->BaseRange; ++step)
							{
								FIntPoint TestPos = CasterPos + FIntPoint(FMath::RoundToInt(Dir.X * step), FMath::RoundToInt(Dir.Y * step));
								if (GridSystem->GetTileAtPosition(TestPos)) LastValidPos = TestPos;
								else break;
							}
							TargetPos = LastValidPos;

							if (APE_CharacterBase* EdgeChar = GridSystem->GetCharacterAtPosition(TargetPos))
							{
								PhaseTargetLoc = EdgeChar->GetActorLocation();
								if (UCapsuleComponent* Cap = EdgeChar->FindComponentByClass<UCapsuleComponent>()) PhaseTargetLoc.Z += Cap->GetScaledCapsuleHalfHeight() * 0.8f;
							}
							else if (AACTile* EdgeTile = GridSystem->GetTileAtPosition(TargetPos))
							{
								PhaseTargetLoc = EdgeTile->GetActorLocation();
								PhaseTargetLoc.Z += 20.f;
							}
						}
					}

					if (TargetPos != FIntPoint(-999, -999))
					{
						TSet<FIntPoint> AffectedPositions = CurrentPhase.GetAffectedGridPositions(CasterPos, TargetPos, SkillData->BaseRange);
						TArray<AActor*> AllChars;
						UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);

						for (AActor* Actor : AllChars)
						{
							if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
							{
								bool bIsValidTarget = (Char != Caster) && Char->GetStatComponent() && !Char->GetStatComponent()->IsDead();
								if (SkillData->TargetType != EPESkillTargetType::Snap_Ally)
									bIsValidTarget = bIsValidTarget && (Char->GetTeamID() != Caster->GetTeamID());

								if (bIsValidTarget)
								{
									if (UACGridMovementComponent* MoveComp = Char->GetGridMovementComponent())
									{
										if (AffectedPositions.Contains(MoveComp->GetGridPosition()))
											AffectedTargets.Add(Char);
									}
								}
							}
						}
					}
				}

				// 물리적 객체(투사체 등) 스폰
				if (CurrentPhase.SkillActorClass)
				{
					FTransform SpawnTransform = Caster->GetActorTransform();

					if (CurrentPhase.ProjectileSpeed > 0.f)
					{
						FVector StartLoc = Caster->GetActorLocation();
						if (UCapsuleComponent* Cap = Caster->FindComponentByClass<UCapsuleComponent>())
							StartLoc.Z += Cap->GetScaledCapsuleHalfHeight() * 0.7f;

						SpawnTransform.SetLocation(StartLoc + SpawnTransform.GetRotation().Vector() * 70.0f);
						SpawnTransform.SetRotation(ExactRotation.Quaternion());

						if (CurrentPhase.bDestroyOnHit)
						{
							int32 NumSegments = 20;
							FVector LastPos = SpawnTransform.GetLocation();
							FCollisionQueryParams Params;
							Params.AddIgnoredActor(Caster);
							FCollisionShape SweepShape = FCollisionShape::MakeSphere(5.f);

							for (int32 step = 1; step <= NumSegments; ++step)
							{
								float Alpha = (float)step / (float)NumSegments;
								FVector NextPos = FMath::Lerp(SpawnTransform.GetLocation(), PhaseTargetLoc, Alpha);
								if (CurrentPhase.ProjectileGravity > 0.f)
									NextPos.Z += FMath::Sin(Alpha * PI) * CurrentPhase.ProjectileGravity;

								FHitResult HitResult;
								if (GetWorld()->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
								{
									PhaseTargetLoc = HitResult.Location;
									PhaseTargetChar = Cast<APE_CharacterBase>(HitResult.GetActor());
									break;
								}
								LastPos = NextPos;
							}
						}
					}
					else
					{
						SpawnTransform.SetLocation(PhaseTargetLoc);
						SpawnTransform.SetRotation(ExactRotation.Quaternion());
					}

					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = Caster;
					SpawnParams.Instigator = Caster;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

					APE_SkillActionActor* ActionActor = GetWorld()->SpawnActor<APE_SkillActionActor>(CurrentPhase.SkillActorClass, SpawnTransform, SpawnParams);
					if (ActionActor)
					{
						// i(PhaseIndex)와 이 페이즈의 토큰을 넘겨, 액터가 타격을 마친 뒤 스스로 반납하게 합니다.
						ActionActor->InitializeActionActor(Caster, PhaseTargetChar, PhaseTargetLoc, SkillData, i, FinalDamage, LogIDToClear, PhaseToken, AffectedTargets, CasterPos, TargetPos);
					}
					else
					{
						if (APE_GameState* CheckGS = GetWorld()->GetGameState<APE_GameState>())
							CheckGS->EndAction(PhaseToken, LogIDToClear);
					}
				}
				else
				{
					// 즉발 연산 람다 시퀀스 (투사체 없음)
					FVector2D ExplosionSize;
					float ExplosionRadius;
					FRotator AoERotation;
					if (PhaseTargetChar && PhaseTargetChar->GetGridMovementComponent())
						TargetPos = PhaseTargetChar->GetGridMovementComponent()->GetGridPosition();
					else if (Payload.TargetTile)
						TargetPos = Payload.TargetTile->GetGridPosition();

					CurrentPhase.GetAoEBoundsAndRotation(CasterPos, TargetPos, SkillData->BaseRange, ExplosionSize, ExplosionRadius, AoERotation);

					auto ApplyHitFunc = [this, Caster, AffectedTargets, PhaseTargetLoc, SkillData, i, FinalDamage, LogIDToClear, PhaseToken]()
						{
							if (!this || !SkillData || !SkillData->HitPhases.IsValidIndex(i)) return;
							const FPESkillHitPhase& ExecPhase = SkillData->HitPhases[i];

							for (UPE_SkillEffectModule* Module : ExecPhase.EffectModules)
							{
								if (Module)
									Module->ApplyEffects(Caster, AffectedTargets, PhaseTargetLoc, SkillData, FinalDamage);
							}
							for (APE_CharacterBase* Target : AffectedTargets)
							{
								if (Target) NetMulticast_PlayHitVisuals(SkillData, i, Target->GetActorLocation());
							}
							if (APE_GameState* CheckGS = GetWorld()->GetGameState<APE_GameState>())
							{
								CheckGS->EndAction(PhaseToken, LogIDToClear);
							}
						};

					auto ExplodeFunc = [this, SkillData, i, PhaseTargetLoc, AoERotation, ExplosionSize, ExplosionRadius, ApplyHitFunc]()
						{
							if (!this || !SkillData || !SkillData->HitPhases.IsValidIndex(i)) return;
							const FPESkillHitPhase& ExecPhase = SkillData->HitPhases[i];

							NetMulticast_PlayExplosionVisuals(SkillData, i, PhaseTargetLoc, AoERotation, ExplosionSize, ExplosionRadius);

							if (ExecPhase.HitDelay > 0.f)
							{
								FTimerHandle HitTimer;
								GetWorld()->GetTimerManager().SetTimer(HitTimer, FTimerDelegate::CreateWeakLambda(this, ApplyHitFunc), ExecPhase.HitDelay, false);
							}
							else
							{
								ApplyHitFunc();
							}
						};

					if (CurrentPhase.ExplosionDelay > 0.f)
					{
						FTimerHandle ExplosionTimer;
						GetWorld()->GetTimerManager().SetTimer(ExplosionTimer, FTimerDelegate::CreateWeakLambda(this, ExplodeFunc), CurrentPhase.ExplosionDelay, false);
					}
					else
					{
						ExplodeFunc();
					}
				}
			}; // ExecutePhaseFunc 끝

		// 페이즈의 고유 딜레이(TriggerTime)에 맞춰 실행을 예약
		if (SkillData->HitPhases[i].TriggerTime > 0.f)
		{
			FTimerHandle PhaseTimer;
			GetWorld()->GetTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateWeakLambda(this, ExecutePhaseFunc), SkillData->HitPhases[i].TriggerTime, false);
		}
		else
		{
			ExecutePhaseFunc();
		}
	}

	// 기반 스킬 본체의 토큰 반납. (UI 로그는 마지막 페이즈가 책임지므로 여기서는 건드리지 않습니다)
	if (GS) GS->EndAction(Payload.ActionTokenID);
}

void UACSkillComponent::NetMulticast_PlayCastVisuals_Implementation(const UPE_SkillData* SkillData)
{
	if (!SkillData) return;
	APE_CharacterBase* Caster = Cast<APE_CharacterBase>(GetOwner());
	if (!Caster) return;

	if (SkillData->CastVFX) { UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SkillData->CastVFX, Caster->GetActorLocation()); }
	if (SkillData->CastSFX) { UGameplayStatics::PlaySoundAtLocation(GetWorld(), SkillData->CastSFX, Caster->GetActorLocation()); }

	if (SkillData->CastAnimMontage)
	{
		Caster->PlayAnimMontage(SkillData->CastAnimMontage, 1.0f, SkillData->CastAnimSectionName);
	}
}

void UACSkillComponent::NetMulticast_PlayExplosionVisuals_Implementation(const UPE_SkillData* SkillData, int32 PhaseIndex, FVector TargetLocation, FRotator TargetRotation, FVector2D ExplosionSize, float ExplosionRadius)
{
	if (!SkillData || !SkillData->HitPhases.IsValidIndex(PhaseIndex)) return;
	const FPESkillHitPhase& Phase = SkillData->HitPhases[PhaseIndex];

	if (Phase.ExplosionVFX)
	{
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Phase.ExplosionVFX, TargetLocation, TargetRotation);
		if (NiagaraComp)
		{
			NiagaraComp->SetVariableVec2(FName("ExplosionSize"), ExplosionSize);
			NiagaraComp->SetVariableVec3(FName("ExplosionBox"), FVector(ExplosionSize.X, ExplosionSize.Y, 100.f));
			NiagaraComp->SetVariableFloat(FName("ExplosionRadius"), ExplosionRadius);
		}
	}
	if (Phase.ExplosionSFX) { UGameplayStatics::PlaySoundAtLocation(GetWorld(), Phase.ExplosionSFX, TargetLocation); }
}

void UACSkillComponent::NetMulticast_PlayHitVisuals_Implementation(const UPE_SkillData* SkillData, int32 PhaseIndex, FVector TargetLocation)
{
	if (!SkillData || !SkillData->HitPhases.IsValidIndex(PhaseIndex)) return;
	const FPESkillHitPhase& Phase = SkillData->HitPhases[PhaseIndex];

	if (Phase.HitVFX) { UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Phase.HitVFX, TargetLocation); }
	if (Phase.HitSFX) { UGameplayStatics::PlaySoundAtLocation(GetWorld(), Phase.HitSFX, TargetLocation); }
}