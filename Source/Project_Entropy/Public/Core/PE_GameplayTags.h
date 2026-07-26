// Copyright CrograNM

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * C++에서 안전하게 사용할 수 있도록 네이티브 게임플레이 태그를 싱글톤으로 관리하는 클래스입니다.
 * FPE_GameplayTags::Get().Element_Normal_Fire 와 같은 방식으로 안전하게 접근 가능.
 */
class PROJECT_ENTROPY_API FPE_GameplayTags
{
public:
	static const FPE_GameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	// --- Element Tags ---
	FGameplayTag Element_Normal_Fire;
	FGameplayTag Element_Normal_Water;
	FGameplayTag Element_Normal_Earth;
	FGameplayTag Element_Normal_Wind;

	FGameplayTag Element_Special_Ice;
	FGameplayTag Element_Special_Lightning;
	FGameplayTag Element_Special_Light;
	FGameplayTag Element_Special_Dark;

	FGameplayTag Element_Complex_Steam;					// 불 + 물
	FGameplayTag Element_Complex_Mud;					// 물 + 땅
	
	// FGameplayTag Element_Complex_Lava;				// 불 + 땅
	// FGameplayTag Element_Complex_FlameSpread;		// 불 + 바람
	// FGameplayTag Element_Complex_Rain;				// 물 + 바람
	// FGameplayTag Element_Complex_Sandstorm;			// 땅 + 바람

	FGameplayTag Element_Special_Hellfire;
	FGameplayTag Element_Special_Tsunami;
	FGameplayTag Element_Special_Earthquake;
	FGameplayTag Element_Special_Frozen;
	FGameplayTag Element_Special_Thunderstorm;
	FGameplayTag Element_Special_Sanctuary;
	FGameplayTag Element_Special_Abyss;

private:
	static FPE_GameplayTags GameplayTags;
};