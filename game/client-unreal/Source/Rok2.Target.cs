// Copyright ROK2. Game target (Windows + Android).

using UnrealBuildTool;
using System.Collections.Generic;

public class Rok2Target : TargetRules // bisi
{
	public Rok2Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Rok2");

		// Enable Android support
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Default Android settings; UE injects APK packaging
		}
	}
}
