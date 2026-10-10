using UnrealBuildTool;

public class BaddieCompanion : ModuleRules
{
    public BaddieCompanion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.AddRange(new[] { "CoreUObject", "Engine", "Json" });
    }
}
