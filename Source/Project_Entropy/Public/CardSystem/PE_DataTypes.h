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

// 속성 태그(Element.Normal.Fire 등)는 언리얼 에디터의 [프로젝트 세팅 -> Gameplay Tags] 메뉴에서 직접 타이핑하여 추가
// C++에서는 FGameplayTag 구조체를 사용하여 이 태그들을 받을 수 있음