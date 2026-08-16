// Copyright CrograNM

#include "Components/ACSkillComponent.h"
#include "Core/PE_PlayerController.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillActionActor.h"
#include "Components/ACStatComponent.h"
#include "Components/ACGridMovementComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Grid/ACTile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Core/PE_GameState.h"
#include "Components/CapsuleComponent.h" // [추가됨] 콜리전 높이 계산용

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

bool UACSkillComponent::TryExecuteSkillByData(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage, int32 ClientRequestID)
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

	// AP 결제 시도 (AP가 부족하면 실패 처리)
	if (OwnerStatComponent->ConsumeAP(SkillData->BaseAPCost))
	{
		// 2. 결제가 성공하면 스킬 로직을 즉시 실행하지 않고, 명세서를 만들어 큐에 집어넣습니다!
		FPESkillActionPayload Payload;
		Payload.Instigator = Caster;
		Payload.TargetTile = TargetTile;
		Payload.TargetCharacter = TargetCharacter;
		Payload.SkillData = SkillData;
		Payload.CalculatedDamage = CalculatedDamage;
		Payload.ClientRequestID = ClientRequestID;

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

void UACSkillComponent::ExecuteQueuedSkill(const FPESkillActionPayload& Payload)
{
	UPE_SkillData* SkillData = Payload.SkillData;
	APE_CharacterBase* Caster = Payload.Instigator;
	APE_GameState* GS = nullptr;

	if (!SkillData || !Caster)
	{
		GS = Caster->GetWorld()->GetGameState<APE_GameState>();
		if (GS) GS->ReportActionEnded();
		return;
	}

	// --- [1. 타일 지정 vs 캐릭터 대상 분리 및 타겟 위치 확정] ---
	APE_CharacterBase* FinalTargetChar = nullptr;
	FVector OriginalTargetLoc = FVector::ZeroVector;
	FIntPoint TargetGridPos = FIntPoint(-999, -999);

	if (SkillData->TargetType == EPESkillTargetType::Tile)
	{
		// 타일 지정일 경우, 캐릭터 추적(TargetCharacter)을 완전히 무시하고 타일 좌표로 고정
		if (Payload.TargetTile)
		{
			OriginalTargetLoc = Payload.TargetTile->GetActorLocation();
			OriginalTargetLoc.Z += 20.f; // 스윕 오프셋 일치화
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

	// --- [2. 사거리 및 타겟 유효성 재검증 (실행 시점 최신 기준)] ---
	bool bIsValidTarget = true;

	if (TargetGridPos == FIntPoint(-999, -999))
	{
		bIsValidTarget = false;
	}
	else if (SkillData->TargetType != EPESkillTargetType::Self && SkillData->TargetType != EPESkillTargetType::All_Enemies)
	{
		// 발동 시점의 맨해튼 거리 재계산
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
		OwnerStatComponent->SetAP(OwnerStatComponent->GetCurrentAP() + SkillData->BaseAPCost);

		if (APE_PlayerController* PC = Cast<APE_PlayerController>(Caster->GetController()))
		{
			PC->Client_CancelSkillExecution(Payload.ClientRequestID);
		}
		if (GS) GS->ReportActionEnded();
		return;
	}

	// 검증 성공: 실제 스킬 발동 시작 및 카드 영구 소멸 확정 (ID 전송)
	if (APE_PlayerController* PC = Cast<APE_PlayerController>(Caster->GetController()))
	{
		PC->Client_ConfirmSkillExecution(Payload.ClientRequestID);
	}

	// 물리 연산 및 발사 전, 대상을 향해 정확히 회전시켜 방향 벡터 오차를 100% 억제
	if (SkillData->TargetType != EPESkillTargetType::Self && SkillData->TargetType != EPESkillTargetType::All_Enemies)
	{
		FVector Dir = (OriginalTargetLoc - Caster->GetActorLocation()).GetSafeNormal2D();
		if (!Dir.IsNearlyZero())
		{
			Caster->SetActorRotation(Dir.Rotation());
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

		// --- [핵심 수정됨: 물리 서버측 궤적 스윕 동기화] ---
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
						FinalTargetChar = HitChar; // 요격당한 대상을 최종 타겟으로 덮어씀
					}
					else
					{
						FinalTargetChar = nullptr; // 지형지물에 막힘
					}
					break;
				}
				LastPos = NextPos;
			}
		}

		APE_SkillActionActor* ActionActor = GetWorld()->SpawnActor<APE_SkillActionActor>(SkillData->SkillActorClass, SpawnTransform);
		if (ActionActor)
		{
			ActionActor->InitializeActionActor(Caster, FinalTargetChar, FinalTargetLoc, SkillData, Payload.CalculatedDamage, Payload.ActionLogID);
		}
	}
	else
	{
		if (GS) GS->ReportActionEnded(Payload.ActionLogID);
	}
}

// 모든 클라이언트에서 동일한 시전(Cast) 애니메이션과 사운드를 재생합니다.
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

// 모든 클라이언트에서 동일한 적중(Hit) 폭발과 사운드를 재생합니다.
void UACSkillComponent::NetMulticast_PlayHitVisuals_Implementation(const UPE_SkillData* SkillData, FVector TargetLocation)
{
	if (!SkillData) return;

	if (SkillData->HitVFX) { UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), SkillData->HitVFX, TargetLocation); }
	if (SkillData->HitSFX) { UGameplayStatics::PlaySoundAtLocation(GetWorld(), SkillData->HitSFX, TargetLocation); }
}