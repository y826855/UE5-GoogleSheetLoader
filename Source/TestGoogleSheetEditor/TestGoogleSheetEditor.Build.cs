using UnrealBuildTool;

public class TestGoogleSheetEditor : ModuleRules
{
    public TestGoogleSheetEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "TestGoogleSheet",
                "GoogleSheetLoader",
                "Paper2D"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
            }
        );
    }
}
