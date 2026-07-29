// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GoogleSheetLoader : ModuleRules
{
	public GoogleSheetLoader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AssetRegistry",
				"UnrealEd",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"InputCore",
				"HTTP",
				"Json",
				"PropertyEditor",
				"ToolMenus",
				"WorkspaceMenuStructure",
			}
			);
	}
}
