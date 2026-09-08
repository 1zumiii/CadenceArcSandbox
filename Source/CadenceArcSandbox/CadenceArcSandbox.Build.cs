// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CadenceArcSandbox : ModuleRules
{
	public CadenceArcSandbox(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange([
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"CadenceArc"
		]);
		
	}
}