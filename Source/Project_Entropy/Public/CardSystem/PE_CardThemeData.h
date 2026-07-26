// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CardSystem/PE_DataTypes.h"
#include "PE_CardThemeData.generated.h"

class UTexture2D;
class UNiagaraSystem;
class UMaterialInstance;

/**
 * 특정 등급(Rarity) 내에 존재하는 속성별 마법진 이펙트를 담는 구조체
 */
USTRUCT(BlueprintType)
struct FPE_ElementVFXMap
{
	GENERATED_BODY()

	// 속성(ElementTag) -> 나이아가라 이펙트 매핑
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|VFX")
	TMap<FGameplayTag, TObjectPtr<UNiagaraSystem>> ElementVFXs;
};

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

	/**
	 * [등급별 -> 속성별] 마법진 이펙트 매핑
	 * 예: 전설(Key) -> [불 속성 마법진, 물 속성 마법진 ...](Value)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Theme|VFX")
	TMap<EPECardRarity, FPE_ElementVFXMap> CastingMagicCircleMap;
};