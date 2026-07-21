// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PE_SkillBase.generated.h"

class APE_CharacterBase;
class AACTile;

/** 스킬이 누구를 타겟으로 하는지 정의 */
UENUM(BlueprintType)
enum class EPESkillTargetType : uint8
{
	Enemy       UMETA(DisplayName = "Enemy (적군)"),
	Ally        UMETA(DisplayName = "Ally (아군)"),
	Self        UMETA(DisplayName = "Self (자신)"),
	EmptyTile   UMETA(DisplayName = "Empty Tile (빈 타일)")
};

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class PROJECT_ENTROPY_API UPE_SkillBase : public UObject
{
	GENERATED_BODY()

public:
	UPE_SkillBase();

	// UObject 내부에서 타이머나 스폰(Spawn) 함수를 쓰기 위해 필수적인 오버라이드
	virtual UWorld* GetWorld() const override;

	/** 스킬 발동 가능 여부를 검증하는 함수 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	bool CanExecute(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);
	virtual bool CanExecute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);

	/** 실제 스킬 효과(데미지, 버프, 투사체 발사 등)를 적용하는 함수 */
	UFUNCTION(BlueprintNativeEvent, Category = "Skill")
	void Execute(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);
	virtual void Execute_Implementation(APE_CharacterBase* Caster, AACTile* TargetTile, APE_CharacterBase* TargetCharacter);

	/** --- 스킬 기본 데이터 --- */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Data")
	FName SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Data")
	int32 APCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Data")
	int32 Range;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill|Data")
	EPESkillTargetType TargetType;
};