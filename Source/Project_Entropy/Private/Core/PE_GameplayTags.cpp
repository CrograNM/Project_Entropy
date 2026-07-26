// Copyright CrograNM

#include "Core/PE_GameplayTags.h"
#include "GameplayTagsManager.h"

FPE_GameplayTags FPE_GameplayTags::GameplayTags;

void FPE_GameplayTags::InitializeNativeGameplayTags()
{
	// UGameplayTagsManager를 통해 C++ 코드로 태그를 직접 주입합니다.
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	// --- 1. 기본 속성 태그 (일반) ---
	{
		GameplayTags.Element_Normal_Fire = Manager.AddNativeGameplayTag(
			FName("Element.Normal.Fire"),
			FString("기본 불 속성 태그입니다.")
		);

		GameplayTags.Element_Normal_Water = Manager.AddNativeGameplayTag(
			FName("Element.Normal.Water"),
			FString("기본 물 속성 태그입니다.")
		);

		GameplayTags.Element_Normal_Earth = Manager.AddNativeGameplayTag(
			FName("Element.Normal.Earth"),
			FString("기본 땅 속성 태그입니다.")
		);

		GameplayTags.Element_Normal_Wind = Manager.AddNativeGameplayTag(
			FName("Element.Normal.Wind"),
			FString("기본 바람 속성 태그입니다.")
		);
	}


	// --- 2. 기본 속성 태그 (특수) ---
	{
		GameplayTags.Element_Special_Ice = Manager.AddNativeGameplayTag(
			FName("Element.Special.Ice"),
			FString("특수 얼음 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Lightning = Manager.AddNativeGameplayTag(
			FName("Element.Special.Lightning"),
			FString("특수 번개 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Light = Manager.AddNativeGameplayTag(
			FName("Element.Special.Light"),
			FString("특수 빛 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Dark = Manager.AddNativeGameplayTag(
			FName("Element.Special.Dark"),
			FString("특수 어둠 속성 태그입니다.")
		);
	}


	// --- 3. 복합 지형 속성 태그 ---
	{
		GameplayTags.Element_Complex_Steam = Manager.AddNativeGameplayTag(
			FName("Element.Complex.Steam"),
			FString("불과 물이 결합된 증발 속성 태그입니다.")
		);

		GameplayTags.Element_Complex_Mud = Manager.AddNativeGameplayTag(
			FName("Element.Complex.Mud"),
			FString("물과 땅이 결합된 진흙 속성 태그입니다.")
		);
	}


	// --- 4. 상위 지형(필살기) 속성 태그 ---
	{
		GameplayTags.Element_Special_Hellfire = Manager.AddNativeGameplayTag(
			FName("Element.Special.Hellfire"),
			FString("특수 지형 불지옥 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Tsunami = Manager.AddNativeGameplayTag(
			FName("Element.Special.Tsunami"),
			FString("특수 지형 해일 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Earthquake = Manager.AddNativeGameplayTag(
			FName("Element.Special.Earthquake"),
			FString("특수 지형 지진 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Frozen = Manager.AddNativeGameplayTag(
			FName("Element.Special.Frozen"),
			FString("특수 지형 얼어붙은 땅 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Thunderstorm = Manager.AddNativeGameplayTag(
			FName("Element.Special.Thunderstorm"),
			FString("특수 지형 폭풍 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Sanctuary = Manager.AddNativeGameplayTag(
			FName("Element.Special.Sanctuary"),
			FString("특수 지형 성역 속성 태그입니다.")
		);

		GameplayTags.Element_Special_Abyss = Manager.AddNativeGameplayTag(
			FName("Element.Special.Abyss"),
			FString("특수 지형 심연 속성 태그입니다.")
		);
	}
}