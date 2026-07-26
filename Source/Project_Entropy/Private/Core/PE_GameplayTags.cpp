// Copyright CrograNM

#include "Core/PE_GameplayTags.h"
#include "GameplayTagsManager.h"

FPE_GameplayTags FPE_GameplayTags::GameplayTags;

void FPE_GameplayTags::InitializeNativeGameplayTags()
{
	// UGameplayTagsManager를 통해 C++ 코드로 태그를 직접 주입합니다.
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	// 카드 및 지형에서 사용할 기본 속성 추가
	GameplayTags.Element_Normal_Fire = Manager.AddNativeGameplayTag(
		FName("Element.Normal.Fire"),
		FString("기본 불 속성 태그입니다.")
	);

	GameplayTags.Element_Normal_Water = Manager.AddNativeGameplayTag(
		FName("Element.Normal.Water"),
		FString("기본 물 속성 태그입니다.")
	);

	// 복합 지형 속성 추가
	GameplayTags.Element_Complex_Mud = Manager.AddNativeGameplayTag(
		FName("Element.Complex.Mud"),
		FString("물과 땅이 결합된 진흙 속성 태그입니다.")
	);

	// 특수 지형 속성 추가
	GameplayTags.Element_Special_Hellfire = Manager.AddNativeGameplayTag(
		FName("Element.Special.Hellfire"),
		FString("특수 지형 불지옥 속성 태그입니다.")
	);
}