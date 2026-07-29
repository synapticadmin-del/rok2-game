// Copyright ROK2. Editor target.

using UnrealBuildTool;
using System.Collections.Generic;

public class Rok2EditorTarget : TargetRules
{
	public Rok2EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		bOverrideBuildEnvironment = true;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Rok2");
	}
}
