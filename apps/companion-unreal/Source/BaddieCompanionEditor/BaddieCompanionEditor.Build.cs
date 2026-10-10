using UnrealBuildTool;

public class BaddieCompanionEditor : ModuleRules
{
    public BaddieCompanionEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.Add("BaddieCompanion");
    }
}
