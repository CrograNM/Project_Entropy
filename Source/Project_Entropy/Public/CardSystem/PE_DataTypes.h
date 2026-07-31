// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "PE_DataTypes.generated.h"

// 스킬의 타겟팅 방식
UENUM(BlueprintType)
enum class EPESkillTargetType : uint8
{
	Tile        UMETA(DisplayName = "타일 지정 (Tile)"),
	Snap_Enemy  UMETA(DisplayName = "대상 지정: 적 (Snap Enemy)"),
	Snap_Ally   UMETA(DisplayName = "대상 지정: 아군 (Snap Ally)"),
	Self        UMETA(DisplayName = "자신 (Self)"),
	All_Enemies UMETA(DisplayName = "전체 적 (All Enemies)")
};

// 카드의 희귀도(등급)
UENUM(BlueprintType)
enum class EPECardRarity : uint8
{
	Common      UMETA(DisplayName = "일반 (Common)"),
	Rare        UMETA(DisplayName = "희귀 (Rare)"),
	Epic        UMETA(DisplayName = "영웅 (Epic)"),
	Legendary   UMETA(DisplayName = "전설 (Legendary)")
};

USTRUCT(BlueprintType)
struct FPESkillActionPayload
{
	GENERATED_BODY()

	UPROPERTY()
	class APE_CharacterBase* Instigator = nullptr;

	UPROPERTY()
	class AACTile* TargetTile = nullptr;

	UPROPERTY()
	class APE_CharacterBase* TargetCharacter = nullptr;

	UPROPERTY()
	class UPE_SkillData* SkillData = nullptr;

	UPROPERTY()
	float CalculatedDamage = 0.f;
};