// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_CardThemeData.generated.h"

class UTexture2D;
class UMaterialInstance;

/**
 * 카드의 등급(Rarity) 및 속성(Element)에 따른 UI 색상과 프레임 에셋을
 * 중앙 집중식으로 관리하는 데이터 에셋
 */
UCLASS(BlueprintType)
class PROJECT_ENTROPY_API UPE_CardThemeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 등급별 머티리얼 인스턴스(프레임)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Rarity")
	TMap<EPECardRarity, TObjectPtr<UMaterialInstance>> RarityMaterialMap;

	// 등급별 이름 텍스트 색상
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Rarity")
	TMap<EPECardRarity, FLinearColor> RarityColorMap;

	// 스킬 속성(Element)별 배경색 및 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Element")
	TMap<FGameplayTag, TObjectPtr<UTexture2D>> ElementIconMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|Element")
	TMap<FGameplayTag, FLinearColor> ElementColorMap;
};