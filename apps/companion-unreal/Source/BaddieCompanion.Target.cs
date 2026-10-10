using UnrealBuildTool;

public class BaddieCompanionTarget : TargetRules
{
    public BaddieCompanionTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.Add("BaddieCompanion");
    }
}
