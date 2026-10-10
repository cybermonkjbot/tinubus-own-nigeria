using UnrealBuildTool;

public class BaddieCompanionEditorTarget : TargetRules
{
    public BaddieCompanionEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.AddRange(new[] { "BaddieCompanion", "BaddieCompanionEditor" });
    }
}
