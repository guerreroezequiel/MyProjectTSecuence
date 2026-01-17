// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProjectTSecuence : ModuleRules
{
	public MyProjectTSecuence(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"MassEntity", 
			"MassCommon",
			"MassSpawner",
			"MassMovement",
			"MassActors",
			"MassRepresentation",
			"MassSignals",
			"MassSimulation",
			"GameplayDebugger", 
			"TurboSequence_Lf", 
			"UMG", "Slate", "SlateCore"
        });

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"Blutility"
			});

			PrivateDependencyModuleNames.AddRange(new string[] { 
				"UnrealEd",
				"EditorScriptingUtilities",
				"EditorStyle"
			});
		}
	}
}
