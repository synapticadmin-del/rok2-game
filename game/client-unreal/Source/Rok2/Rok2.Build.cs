// Copyright ROK2. Building module rules.

using UnrealBuildTool;

public class Rok2 : ModuleRules
{
	public Rok2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// وحدات تظهر في الهيدرات العامة (Public/) — يجب أن تكون عامة، لأن أي
		// مستهلك لهيدراتنا يحتاج مسارات تضمينها:
		//   UMG        → Blueprint/UserWidget.h + Components/Button.h (14 هيدر)
		//   SlateCore  → Styling/SlateBrush.h (Rok2ArtAssets.h, Rok2IconLibrary.h)
		//   Slate      → مرافقة لـ UMG/SlateCore حسب اصطلاح المحرك
		//   HTTP       → Interfaces/IHttpRequest.h + IHttpResponse.h (Rok2Api.h)
		//   WebSockets → IWebSocket.h (Rok2Api.h)
		//   Json       → FJsonObject في تواقيع عامة (Rok2Api.h, Rok2CivThemes.h)
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"HTTP",
			"WebSockets",
			"Json"
		});

		// وحدات لا يذكرها أي هيدر عام — تبقى خاصة (تُستخدم في .cpp فقط)
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"JsonUtilities",
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
