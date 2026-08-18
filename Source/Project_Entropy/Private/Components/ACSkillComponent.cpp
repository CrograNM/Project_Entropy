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
		if (GS) GS->ReportActionEnded(Payload.ActionLogID);
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
		if (GS) GS->ReportActionEnded(Payload.ActionLogID);
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

	if (!SkillData || !Caster)
	{
		if (GS) GS->ReportActionEnded(Payload.ActionLogID);
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
		{
			PC->Client_ConfirmSkillExecution(Payload.ClientRequestID);
		}
	}

	if (SkillData->TargetType != EPESkillTargetType::Self && SkillData->TargetType != EPESkillTargetType::All_Enemies)
	{
		FVector Dir = (OriginalTargetLoc - Caster->GetActorLocation()).GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
		{
			Caster->SetActorRotation(Dir.Rotation());
		}
	}

	// 타겟 탐색을 즉발/투사체 가리지 않고 통일하여 연산합니다.
	TSet<APE_CharacterBase*> AffectedTargets;
	FIntPoint CasterPos = Caster->GetGridMovementComponent() ? Caster->GetGridMovementComponent()->GetGridPosition() : FIntPoint(0, 0);

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
		FIntPoint TargetPos(-999, -999);
		if (FinalTargetChar && FinalTargetChar->GetGridMovementComponent())
			TargetPos = FinalTargetChar->GetGridMovementComponent()->GetGridPosition();
		else if (Payload.TargetTile)
			TargetPos = Payload.TargetTile->GetGridPosition();

		// 레이저일 경우 서버 연산 시점에서도 타겟을 화면 끝으로 확장
		if (SkillData->AoEShape == EPEAoEShape::Line && TargetPos != FIntPoint(-999, -999))
		{
			FVector2D CasterV(CasterPos.X, CasterPos.Y);
			FVector2D TargetV(TargetPos.X, TargetPos.Y);
			FVector2D Dir = (TargetV - CasterV).GetSafeNormal();
			if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);
			TargetPos = CasterPos + FIntPoint(FMath::RoundToInt(Dir.X * SkillData->BaseRange), FMath::RoundToInt(Dir.Y * SkillData->BaseRange));
		}

		if (TargetPos != FIntPoint(-999, -999))
		{
			TSet<FIntPoint> AffectedPositions = SkillData->GetAffectedGridPositions(CasterPos, TargetPos);
			TArray<AActor*> AllChars;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), APE_CharacterBase::StaticClass(), AllChars);
			for (AActor* Actor : AllChars)
			{
				if (APE_CharacterBase* Char = Cast<APE_CharacterBase>(Actor))
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

	if (SkillData->SkillActorClass)
	{
		NetMulticast_PlayCastVisuals(SkillData);

		FVector StartLoc = Caster->GetActorLocation();
		if (UCapsuleComponent* Cap = Caster->FindComponentByClass<UCapsuleComponent>())
			StartLoc.Z += Cap->GetScaledCapsuleHalfHeight() * 0.7f;

		FTransform SpawnTransform = Caster->GetActorTransform();
		SpawnTransform.SetLocation(StartLoc + SpawnTransform.GetRotation().Vector() * 70.0f);

		FVector FinalTargetLoc = OriginalTargetLoc;

		if (SkillData->ProjectileSpeed > 0.f)
		{
			int32 NumSegments = 20;
			FVector LastPos = SpawnTransform.GetLocation();

			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Caster);
			FCollisionShape SweepShape = FCollisionShape::MakeSphere(5.f);

			for (int32 i = 1; i <= NumSegments; ++i)
			{
				float Alpha = (float)i / (float)NumSegments;
				FVector NextPos = FMath::Lerp(SpawnTransform.GetLocation(), OriginalTargetLoc, Alpha);

				if (SkillData->ProjectileGravity > 0.f)
				{
					NextPos.Z += FMath::Sin(Alpha * PI) * SkillData->ProjectileGravity;
				}

				FHitResult HitResult;
				if (GetWorld()->SweepSingleByChannel(HitResult, LastPos, NextPos, FQuat::Identity, ECC_Visibility, SweepShape, Params))
				{
					FinalTargetLoc = HitResult.Location;
					if (APE_CharacterBase* HitChar = Cast<APE_CharacterBase>(HitResult.GetActor()))
					{
						FinalTargetChar = HitChar;
					}
					else
					{
						FinalTargetChar = nullptr;
					}
					break;
				}
				LastPos = NextPos;
			}
		}

		// 시전자와 겹쳐도 무조건 스폰되도록 보장
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Caster;
		SpawnParams.Instigator = Caster;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APE_SkillActionActor* ActionActor = GetWorld()->SpawnActor<APE_SkillActionActor>(SkillData->SkillActorClass, SpawnTransform, SpawnParams);
		if (ActionActor)
		{
			ActionActor->InitializeActionActor(Caster, FinalTargetChar, FinalTargetLoc, SkillData, Payload.CalculatedDamage, Payload.ActionLogID, AffectedTargets);
		}
		else
		{
			// 생성 실패 시 큐 무한 대기를 막기 위한 예외 처리
			if (GS) GS->ReportActionEnded(Payload.ActionLogID);
		}
	}
	else
	{
		NetMulticast_PlayCastVisuals(SkillData);

		// 수집된 타겟들에게 즉시 이펙트 적용
		for (UPE_SkillEffectModule* Module : SkillData->EffectModules)
		{
			if (Module)
			{
				Module->ApplyEffects(Caster, AffectedTargets, OriginalTargetLoc, SkillData, Payload.CalculatedDamage);
			}
		}

		NetMulticast_PlayHitVisuals(SkillData, OriginalTargetLoc);

		// 즉발 스킬은 여기서 스스로 액션 종료를 보고해야 큐가 넘어감
		if (GS) GS->ReportActionEnded(Payload.ActionLogID);
	}
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

void UACSkillComponent::NetMulticast_PlayHitVisuals_Implementation(const UPE_SkillData* SkillData, FVector TargetLocation)
{
	if (!SkillData) return;

	if (SkillData->HitVFX) { UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SkillData->HitVFX, TargetLocation); }
	if (SkillData->HitSFX) { UGameplayStatics::PlaySoundAtLocation(GetWorld(), SkillData->HitSFX, TargetLocation); }
}