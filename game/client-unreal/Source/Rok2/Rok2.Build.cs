// Copyright ROK2. Building module rules.

using UnrealBuildTool;

public class Rok2 : ModuleRules
{
	public Rok2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP",
			"WebSockets",
			"Json",
			"JsonUtilities",
			"UMG",
			"Slate",
			"SlateCore",
			"NavigationSystem",
			"AIModule",
			"RenderCore",
			"RHI"
		});

		// Allow Android target build
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "Launch" });
		}
	}
}
