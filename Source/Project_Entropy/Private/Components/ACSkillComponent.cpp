// Copyright CrograNM

#include "Components/ACSkillComponent.h"
#include "CardSystem/PE_SkillBase.h"
#include "Components/ACStatComponent.h"
#include "Characters/PE_CharacterBase.h"

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

	// 에디터에 세팅된 스킬 클래스를 기반으로 실제 스킬 객체를 생성합니다.
	for (TSubclassOf<UPE_SkillBase> SkillClass : DefaultSkillClasses)
	{
		if (SkillClass)
		{
			UPE_SkillBase* NewSkill = NewObject<UPE_SkillBase>(this, SkillClass);
			ActiveSkills.Add(NewSkill);
		}
	}
}

bool UACSkillComponent::TryExecuteSkill(int32 SkillIndex, AACTile* TargetTile, APE_CharacterBase* TargetCharacter)
{
	if (!ActiveSkills.IsValidIndex(SkillIndex) || !OwnerStatComponent) return false;

	UPE_SkillBase* SkillToUse = ActiveSkills[SkillIndex];
	APE_CharacterBase* Caster = Cast<APE_CharacterBase>(GetOwner());

	// 1. 타겟 조건이 맞는지 검증
	if (!SkillToUse->CanExecute(Caster, TargetTile, TargetCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SkillSystem] 타겟이 유효하지 않아 스킬을 사용할 수 없습니다."));
		return false;
	}

	// 2. AP 결제 시도 (AP가 부족하면 실패 처리)
	if (OwnerStatComponent->ConsumeAP(SkillToUse->APCost))
	{
		// 3. 결제 성공 시 실제 스킬 효과 발동
		SkillToUse->Execute(Caster, TargetTile, TargetCharacter);
		return true;
	}

	return false;
}