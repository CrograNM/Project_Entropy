// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_CardData.generated.h"

class UPE_SkillData;
class UTexture2D;

/**
 * 스킬 데이터를 카드 형태(UI, 등급)로 플레이어에게 제공하기 위한 포장지 데이터입니다.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECT_ENTROPY_API UPE_CardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info")
	FName CardID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info")
	FText CardName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Info", meta=(MultiLine="true"))
	FText CardDescription; 

	// 카드 등급
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card|Info")
	EPECardRarity Rarity = EPECardRarity::Common;

	// 카드 발동 트리거 타입 (None이면 일반 사용, OnTurnEnd이면 턴 종료 시 자동 발동)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card|Trigger")
	EPECardTriggerType TriggerType = EPECardTriggerType::None;

	// 카드 일러스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card|Visual")
	TObjectPtr<UTexture2D> CardArt; 

	// 스킬 데이터 참조
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card|Skill")
	TObjectPtr<UPE_SkillData> SkillDataToCast;
};