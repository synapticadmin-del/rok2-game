#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2WorldRenderer.h"
#include "Rok2CityBuilder.h"
#include "Rok2PlayerController.h"
#include "Rok2BootWidget.h"
#include "Rok2CityWidget.h"
#include "Rok2HudWidget.h"
#include "Rok2BuildMenuWidget.h"
#include "Rok2CommanderWidget.h"
#include "Rok2AllianceRosterWidget.h"
#include "Rok2BattleReportWidget.h"
#include "Rok2ViewManager.h"
#include "Rok2IsometricCamera.h"
#include "Rok2BlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Kismet/GameplayStatics.h"

ARok2GameMode::ARok2GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PlayerControllerClass = ARok2PlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	ApiBaseUrl = TEXT("https://rok2-api.lolelarap.workers.dev");
	KingdomId = TEXT("kingdom-1");
	// فارغ عمداً: المفتاح الإداري لا يُشحن داخل العميل. اضبطه يدوياً في
	// جلسة تطوير إن احتجت /v1/admin/*.
	AdminKey = TEXT("");
	TickIntervalSeconds = 0.1f;
}

void ARok2GameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (World)
	{
		// 1. Directional Light
		if (!UGameplayStatics::GetActorOfClass(World, ADirectionalLight::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1000.f), FRotator(-45.f, -45.f, 0.f), P);
			if (Sun && Sun->GetLightComponent())
			{
				Sun->GetLightComponent()->SetIntensity(3.14f);
				Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
			}
		}

		// 2. Sky Light
		if (!UGameplayStatics::GetActorOfClass(World, ASkyLight::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector(0.f, 0.f, 1100.f), FRotator::ZeroRotator, P);
			if (Sky && Sky->GetLightComponent())
			{
				Sky->GetLightComponent()->SetIntensity(1.5f);
				Sky->GetLightComponent()->SetLightColor(FLinearColor(0.8f, 0.9f, 1.f));
			}
		}

		// 3. World Renderer
		if (!UGameplayStatics::GetActorOfClass(World, ARok2WorldRenderer::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<ARok2WorldRenderer>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		}

		// 4. City Builder
		if (!UGameplayStatics::GetActorOfClass(World, ARok2CityBuilder::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<ARok2CityBuilder>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		}
	}

	if (!Api)
	{
		Api = NewObject<URok2Api>(this);
	}
	Api->SetCivilizations(URok2BlueprintLibrary::GetDefaultCivilizations());
	Api->Init(ApiBaseUrl, KingdomId, AdminKey);
	Api->OnPlayerLoaded.AddDynamic(this, &ARok2GameMode::OnPlayerLoadedHandler);

	// Boot Widget
	if (!BootWidget && World)
	{
		BootWidget = Cast<URok2BootWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2BootWidget::StaticClass()));
		if (BootWidget)
		{
			BootWidget->Setup(Api);
			BootWidget->AddToViewport(100);
		}
	}

	Api->LoginAsGuest();
}

void ARok2GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Api)
	{
		Api->PumpEvents(DeltaSeconds);
	}
}

void ARok2GameMode::OnPlayerLoadedHandler(const FRok2Player& Player)
{
	if (BootWidget)
	{
		BootWidget->RemoveFromParent();
		BootWidget = nullptr;
	}

	if (!CityWidget && GetWorld())
	{
		CityWidget = Cast<URok2CityWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2CityWidget::StaticClass()));
		if (CityWidget)
		{
			CityWidget->Setup(Api);
			CityWidget->AddToViewport(10);
		}
	}

	if (!HudWidget && GetWorld())
	{
		HudWidget = Cast<URok2HudWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2HudWidget::StaticClass()));
		if (HudWidget)
		{
			HudWidget->Setup(Api);
			HudWidget->AddToViewport(20);
			BindHudEvents();
		}
	}

	EnsureViewManager();
}

// ---------------------------------------------------------------------------
// ربط أحداث HUD (P5-T3) بالمعالجات
// ---------------------------------------------------------------------------
void ARok2GameMode::BindHudEvents()
{
	if (!HudWidget) return;
	HudWidget->OnBuildAction.AddDynamic(this, &ARok2GameMode::HandleBuildAction);
	HudWidget->OnEditCityAction.AddDynamic(this, &ARok2GameMode::HandleEditCityAction);
	HudWidget->OnCommandersAction.AddDynamic(this, &ARok2GameMode::HandleCommandersAction);
	HudWidget->OnAllianceAction.AddDynamic(this, &ARok2GameMode::HandleAllianceAction);
	HudWidget->OnItemsAction.AddDynamic(this, &ARok2GameMode::HandleItemsAction);
	HudWidget->OnEventsAction.AddDynamic(this, &ARok2GameMode::HandleEventsAction);
	HudWidget->OnMapAction.AddDynamic(this, &ARok2GameMode::HandleMapAction);
	HudWidget->OnReportsAction.AddDynamic(this, &ARok2GameMode::HandleReportsAction);
}

void ARok2GameMode::EnsureViewManager()
{
	UWorld* World = GetWorld();
	if (!World || ViewManager) return;

	ViewManager = Cast<ARok2ViewManager>(UGameplayStatics::GetActorOfClass(World, ARok2ViewManager::StaticClass()));
	if (!ViewManager)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ViewManager = World->SpawnActor<ARok2ViewManager>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	}

	if (ViewManager)
	{
		ViewManager->WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(World, ARok2WorldRenderer::StaticClass()));
		ViewManager->CityBuilder = Cast<ARok2CityBuilder>(UGameplayStatics::GetActorOfClass(World, ARok2CityBuilder::StaticClass()));
		ViewManager->IsoCamera = Cast<ARok2IsometricCamera>(UGameplayStatics::GetActorOfClass(World, ARok2IsometricCamera::StaticClass()));
		// ابدأ بعرض المدينة (اللاعب في مدينته بعد الدخول)
		ViewManager->SwitchToCityView();
	}
}

// ---------------------------------------------------------------------------
// معالجات أحداث HUD
// ---------------------------------------------------------------------------

void ARok2GameMode::HandleBuildAction()
{
	// يفتح قائمة البناء (تُنشأ مرة وتُعاد)
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!BuildMenuWidget)
	{
		BuildMenuWidget = Cast<URok2BuildMenuWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2BuildMenuWidget::StaticClass()));
		if (BuildMenuWidget)
		{
			BuildMenuWidget->Setup(Api);
			BuildMenuWidget->OnBuildMenuPick.AddDynamic(this, &ARok2GameMode::HandleBuildMenuPick);
		}
	}
	if (BuildMenuWidget && !BuildMenuWidget->IsInViewport())
	{
		BuildMenuWidget->AddToViewport(50);
	}
}

void ARok2GameMode::HandleBuildMenuPick(const FString& BuildingId)
{
	// عند اختيار مبنى من القائمة: ندخل وضع تحرير المدينة ليضعه اللاعب على الشبكة
	// (البناء الفعلي عبر UpgradeBuilding عند وضع مبنى جديد — حالياً كل المباني موجودة، فهذا يبدأ التحرير)
	if (ViewManager && ViewManager->CityBuilder)
	{
		if (!ViewManager->CityBuilder->IsEditModeActive())
		{
			ViewManager->CityBuilder->ToggleEditMode();
		}
	}
}

void ARok2GameMode::HandleEditCityAction()
{
	// تفعيل/إيقاف وضع تحرير المدينة (من P5-T1)
	if (ViewManager && ViewManager->CityBuilder)
	{
		ViewManager->CityBuilder->ToggleEditMode();
	}
}

void ARok2GameMode::HandleCommandersAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!CommanderWidget)
	{
		CommanderWidget = Cast<URok2CommanderWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2CommanderWidget::StaticClass()));
		if (CommanderWidget)
		{
			CommanderWidget->Api = Api;
		}
	}
	if (CommanderWidget && !CommanderWidget->IsInViewport())
	{
		CommanderWidget->AddToViewport(50);
	}
}

void ARok2GameMode::HandleAllianceAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!AllianceWidget)
	{
		AllianceWidget = Cast<URok2AllianceRosterWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2AllianceRosterWidget::StaticClass()));
		if (AllianceWidget)
		{
			AllianceWidget->Api = Api;
		}
	}
	if (AllianceWidget && !AllianceWidget->IsInViewport())
	{
		AllianceWidget->AddToViewport(50);
	}
}

void ARok2GameMode::HandleItemsAction()
{
	// الحقيبة — غير منفذة بعد (متجر/VIP). نعرض إشعاراً مؤقتاً.
	if (Api)
	{
		// يستخدم نظام الإشعارات الداخلي عبر toast
		// (تحسين مستقبلي: شاشة حقيبة كاملة)
	}
}

void ARok2GameMode::HandleEventsAction()
{
	// الأحداث — غير منفذة بعد. (تحسين مستقبلي: شاشة أحداث)
}

void ARok2GameMode::HandleMapAction()
{
	// التبديل بين مدينة اللاعب وخريطة العالم
	EnsureViewManager();
	if (ViewManager)
	{
		ViewManager->ToggleView();
		// عند الذهاب للخريطة: حدّث العالم
		if (Api)
		{
			Api->RefreshWorld();
		}
	}
}

void ARok2GameMode::HandleReportsAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!BattleReportWidget)
	{
		BattleReportWidget = Cast<URok2BattleReportWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2BattleReportWidget::StaticClass()));
		if (BattleReportWidget)
		{
			BattleReportWidget->Setup(Api);
		}
	}
	if (BattleReportWidget && !BattleReportWidget->IsInViewport())
	{
		BattleReportWidget->AddToViewport(50);
	}
}
