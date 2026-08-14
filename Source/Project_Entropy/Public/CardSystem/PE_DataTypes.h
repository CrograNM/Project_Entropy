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
	
	UPROPERTY() class APE_CharacterBase* Instigator = nullptr;
	UPROPERTY() class AACTile* TargetTile = nullptr;
	UPROPERTY() class APE_CharacterBase* TargetCharacter = nullptr;
	UPROPERTY() class UPE_SkillData* SkillData = nullptr;
	UPROPERTY() float CalculatedDamage = 0.f;

	UPROPERTY() class APE_CardActor* SourceCard = nullptr; // 원본 카드 (큐에서 시전 시 성공/실패 처리를 위해 참조)

	// 로컬 클라이언트의 카드 식별용 고유 요청 번호
	UPROPERTY() int32 ClientRequestID = -1;
};

// 광역 스킬(AoE)의 형태 정의
UENUM(BlueprintType)
enum class EPEAoEShape : uint8
{
	None    UMETA(DisplayName = "단일 공격 (Single)"),
	Cross   UMETA(DisplayName = "십자 (Cross)"),
	Square  UMETA(DisplayName = "정사각형 (Square)"),
	Ring    UMETA(DisplayName = "도넛/링 (Ring)"),
	Custom  UMETA(DisplayName = "커스텀 (Custom Offset)")
};