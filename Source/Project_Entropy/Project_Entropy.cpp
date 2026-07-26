// Copyright Epic Games, Inc. All Rights Reserved.

#include "Project_Entropy.h"
#include "Modules/ModuleManager.h"
#include "Core/PE_GameplayTags.h"

class FProjectEntropyModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
		FPE_GameplayTags::InitializeNativeGameplayTags();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FProjectEntropyModule, Project_Entropy, "Project_Entropy" );
