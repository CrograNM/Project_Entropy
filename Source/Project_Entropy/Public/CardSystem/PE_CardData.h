// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PE_CardData.generated.h"

class UPE_SkillBase;
class UTexture2D;

/** 카드의 종류 (UI 필터링, 정렬, 특정 효과 발동 조건 등에 사용) */
UENUM(BlueprintType)
enum class EPECardType : uint8
{
	Attack      UMETA(DisplayName = "공격 (Attack)"),
	Skill       UMETA(DisplayName = "스킬 (Skill)"),
	Power       UMETA(DisplayName = "파워/버프 (Power)"),
	Curse       UMETA(DisplayName = "저주/상태이상 (Curse)")
};

/** * 카드의 모든 고정(정적) 데이터를 담는 데이터 에셋 */
UCLASS(BlueprintType, Blueprintable)
class PROJECT_ENTROPY_API UPE_CardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// --- 1. 기본 식별 정보 ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info")
	FName CardID; // 시스템 내부 식별용 ID (예: "Card_Strike")

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info")
	FText CardName; // 유저에게 보여질 이름 (예: "기본 타격")

	// MultiLine=true를 통해 에디터에서 줄바꿈 입력 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info", meta=(MultiLine="true"))
	FText CardDescription; // 카드 설명 (예: "적에게 피해를 20 줍니다.")

	// --- 2. 시각적(UI) 정보 ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Visual")
	TObjectPtr<UTexture2D> CardArt; // 카드 일러스트

	// --- 3. 게임플레이 스탯 ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Stats")
	EPECardType CardType;

	// 카드의 AP 비용 (원래 SkillBase에도 APCost가 있지만, 
	// UI에 표시하기 위해 메모리에 스킬을 올리지 않고도 미리 값을 알 수 있어야 하므로 Card에도 정의해 둡니다)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Stats")
	int32 CostAP;

	// --- 4. 핵심 로직 연동 (어떤 스킬이 나갈 것인가?) ---
	// 유저가 이 카드를 냈을 때, 실제로 캐릭터가 발동할 스킬 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Logic")
	TSubclassOf<UPE_SkillBase> SkillClass;
};