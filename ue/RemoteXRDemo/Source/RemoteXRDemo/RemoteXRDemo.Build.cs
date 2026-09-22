using UnrealBuildTool;

public class RemoteXRDemo : ModuleRules
{
    public RemoteXRDemo(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "XRBase" });
    }
}
