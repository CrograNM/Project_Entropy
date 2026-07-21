// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project_Entropy : ModuleRules
{
	public Project_Entropy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Public
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
            "GameplayTags"
        });

        // Private
        PrivateDependencyModuleNames.AddRange(new string[] { 
			"UMG", 
			"Slate", 
			"SlateCore",
            "Niagara"
        });

        // AI 및 네비게이션 모듈
        /*
		PrivateDependencyModuleNames.AddRange(new string[] {
		    "AIModule",
		    "NavigationSystem",
		    "GameplayTasks"
		});
		*/

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
