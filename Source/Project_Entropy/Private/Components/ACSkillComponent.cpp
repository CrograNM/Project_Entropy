// Copyright CrograNM

#include "Components/ACSkillComponent.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillActionActor.h"
#include "Components/ACStatComponent.h"
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

bool UACSkillComponent::TryExecuteSkillByData(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage)
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!SkillData || !OwnerStatComponent) return false;

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

		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
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

// 큐에서 대기하다가 자기 차례가 오면 GameState가 이 함수를 부릅니다.
void UACSkillComponent::ExecuteQueuedSkill(UPE_SkillData* SkillData, AACTile* TargetTile, APE_CharacterBase* TargetCharacter, float CalculatedDamage)
{
	APE_CharacterBase* Caster = Cast<APE_CharacterBase>(GetOwner());

	if (SkillData && SkillData->SkillActorClass)
	{
		NetMulticast_PlayCastVisuals(SkillData);

		// 1. 발사 시작 지점(Muzzle) 계산: 바닥이 아닌 캐릭터의 가슴/명치 높이에서 발사하도록 보정
		FVector StartLoc = Caster->GetActorLocation();
		if (UCapsuleComponent* Cap = Caster->FindComponentByClass<UCapsuleComponent>())
		{
			StartLoc.Z += Cap->GetScaledCapsuleHalfHeight() * 0.7f; // 약간 위쪽(가슴 높이)으로 조정
		}

		FTransform SpawnTransform = Caster->GetActorTransform();
		SpawnTransform.SetLocation(StartLoc + SpawnTransform.GetRotation().Vector() * 70.0f); // 앞으로 조금 전진

		// 2. 1차 목표 지점 계산
		APE_CharacterBase* FinalTargetChar = TargetCharacter;
		FVector FinalTargetLoc = TargetTile ? TargetTile->GetActorLocation() : FVector::ZeroVector;

		if (FinalTargetChar)
		{
			FinalTargetLoc = FinalTargetChar->GetActorLocation();
			// 타겟 높이는 머리 부분으로 맞추기. (캡슐 기준 80% 정도 높이)
			if (UCapsuleComponent* TargetCap = FinalTargetChar->FindComponentByClass<UCapsuleComponent>())
			{
				FinalTargetLoc.Z += TargetCap->GetScaledCapsuleHalfHeight() * 0.8f;
			}
		}

		// 3. [핵심] 직사/곡사 물리적 충돌 판정 (Raycast)
		// 곡사(Gravity > 0)가 아닌 직사 투사체라면 물리적으로 날아가는 길을 검사합니다.
		if (SkillData->ProjectileSpeed > 0.f && SkillData->ProjectileGravity == 0.f)
		{
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(Caster); // 시전자 본인은 통과

			// ECC_Visibility 채널로 레이저를 쏩니다. (가슴 높이 -> 가슴 높이)
			if (GetWorld()->LineTraceSingleByChannel(HitResult, SpawnTransform.GetLocation(), FinalTargetLoc, ECC_Visibility, Params))
			{
				// 가는 길에 누군가(적, 파괴가능 장애물 등) 대신 맞았다면 타겟 가로채기!
				if (APE_CharacterBase* HitChar = Cast<APE_CharacterBase>(HitResult.GetActor()))
				{
					FinalTargetChar = HitChar;
				}
				else
				{
					// 벽 같은 맵 지형에 막혔다면 대상 없음
					FinalTargetChar = nullptr;
				}

				// [시각화 연동] 실제 콜리전 표면에 부딪힌 정확한 3D 좌표(높이 포함)로 도착 지점 보정!
				FinalTargetLoc = HitResult.Location;
			}
		}

		// 4. 최종 확정된 정보로 스킬 액터 스폰 및 발사
		APE_SkillActionActor* ActionActor = GetWorld()->SpawnActor<APE_SkillActionActor>(SkillData->SkillActorClass, SpawnTransform);
		if (ActionActor)
		{
			ActionActor->InitializeActionActor(Caster, FinalTargetChar, FinalTargetLoc, SkillData, CalculatedDamage);
		}
	}
	else
	{
		// 투사체가 아예 없는 순수 버프 등의 경우 액터 스폰 없이 큐를 즉시 비웁니다.
		if (APE_GameState* GS = GetWorld()->GetGameState<APE_GameState>())
		{
			GS->CompleteCurrentAction();
		}
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