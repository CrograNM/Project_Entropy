// Copyright CrograNM

#include "Components/ACSkillComponent.h"
#include "CardSystem/PE_SkillData.h"
#include "CardSystem/PE_SkillLogicBase.h"
#include "Components/ACStatComponent.h"
#include "Characters/PE_CharacterBase.h"
#include "Grid/ACTile.h"

UACSkillComponent::UACSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
	// 서버가 아니라면 스킬 실행 자체를 차단하여 해킹/오류 방지
	if (!GetOwner()->HasAuthority()) return false;
	if (!SkillData || !OwnerStatComponent) return false;

	APE_CharacterBase* Caster = Cast<APE_CharacterBase>(GetOwner());

	// 1. 타겟 조건이 맞는지 1차 검증 (TargetType 기반)
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

	// 2. AP 결제 시도 (AP가 부족하면 실패 처리)
	if (OwnerStatComponent->ConsumeAP(SkillData->BaseAPCost))
	{
		// 3. 결제 성공 시, 데이터에 명시된 로직 클래스를 동적으로 생성(Instancing)하여 실행합니다.
		if (SkillData->LogicClass)
		{
			UPE_SkillLogicBase* SkillLogic = NewObject<UPE_SkillLogicBase>(this, SkillData->LogicClass);

			// 타일이 존재한다면 타일의 위치를, 아니라면 ZeroVector를 넘겨줍니다.
			FVector TargetLoc = TargetTile ? TargetTile->GetActorLocation() : FVector::ZeroVector;

			// BlueprintNativeEvent 호출 (로직 실행)
			SkillLogic->ExecuteSkill(Caster, TargetCharacter, TargetLoc, SkillData, CalculatedDamage);

			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SkillSystem] SkillData에 LogicClass가 지정되지 않았습니다: %s"), *SkillData->SkillID.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] AP가 부족하여 스킬 발동에 실패했습니다."));
	}

	return false;
}