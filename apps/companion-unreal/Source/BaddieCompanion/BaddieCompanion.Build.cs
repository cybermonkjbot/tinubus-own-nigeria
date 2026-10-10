using UnrealBuildTool;

public class BaddieCompanion : ModuleRules
{
    public BaddieCompanion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "AIModule" });
        PrivateDependencyModuleNames.AddRange(new[] { "ApplicationCore", "HTTP", "InputCore", "Json", "NavigationSystem", "Slate", "SlateCore" });
    }
}
