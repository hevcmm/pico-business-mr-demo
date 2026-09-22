using UnrealBuildTool;

public class PicoBusinessMR : ModuleRules
{
    public PicoBusinessMR(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "HeadMountedDisplay", "OpenXRHMD", "XRBase",
            "RenderCore", "Renderer", "RHI", "Projects"
        });
        AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");
    }
}
