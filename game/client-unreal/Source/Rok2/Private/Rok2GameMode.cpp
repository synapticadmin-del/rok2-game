#include "Rok2GameMode.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
#include "Rok2AudioManager.h"
#include "Rok2WorldRenderer.h"
#include "Rok2CityBuilder.h"
#include "Rok2PlayerController.h"
#include "Rok2BootWidget.h"
#include "Rok2HudWidget.h"
#include "Rok2BuildMenuWidget.h"
#include "Rok2CommanderWidget.h"
#include "Rok2AllianceRosterWidget.h"
#include "Rok2BattleReportWidget.h"
#include "Rok2OnboardingWidget.h"
#include "Rok2CivInfoWidget.h"
#include "Rok2ChatWidget.h"
#include "Rok2ResearchWidget.h"
#include "Rok2SeasonStoryWidget.h"
#include "Rok2SettingsWidget.h"
#include "Rok2TrainHealSheetWidget.h"
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

	// P18-T6: تفضيلات اللاعب المحفوظة تُطبَّق **قبل** بناء أي ودجة.
	//
	// مقياس الواجهة يُقرأ وقت البناء (`GetScaledPx`/`ScaledSize` في كل موضع
	// أبعاد)، فاستعادته بعد إنشاء شاشة الدخول والـHUD كانت ستتركهما على مقياس
	// 1.0 حتى يُعاد بناؤهما — أي أن الإعداد المحفوظ لا يُرى في الجلسة التي
	// يُحمَّل فيها.
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		A11y->LoadAndApplySavedSettings();
	}

	// Create the API before actors: SpawnActor invokes BeginPlay immediately.
	if (!Api)
	{
		Api = NewObject<URok2Api>(this);
	}
	Api->SetCivilizations(URok2BlueprintLibrary::GetDefaultCivilizations());
	Api->Init(ApiBaseUrl, KingdomId, AdminKey);
	Api->OnPlayerLoaded.AddDynamic(this, &ARok2GameMode::OnPlayerLoadedHandler);
	Api->OnSeasonStoryEvent.AddDynamic(this, &ARok2GameMode::HandleSeasonStoryEvent);

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

	// P6-T4: طبقة إرشاد الدقيقة الأولى. تُنشأ **بعد** الـHUD ولوحة المدينة
	// لأنهما من يسجّل مراسي الإبراز — وحتى لو تأخّر التسجيل فالحلقة تُخفى
	// وتُعاد المحاولة، فلا اعتماد على ترتيب صارم.
	//
	// تُنشأ للجميع ولا تُشترط جِدّة اللاعب هنا: النموذج يقرّر بنفسه عند أول
	// حالة جاهزة، فاللاعب العائد يُصنَّف Done فلا بطاقة ولا حلقة ولا عمل في
	// الـTick. لو شرطناها هنا لاحتجنا الحالة قبل وصولها.
	if (!OnboardingWidget && GetWorld())
	{
		OnboardingWidget = Cast<URok2OnboardingWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2OnboardingWidget::StaticClass()));
		if (OnboardingWidget)
		{
			OnboardingWidget->Setup(Api);
			OnboardingWidget->AddToViewport(60);
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
	HudWidget->OnCivInfoAction.AddDynamic(this, &ARok2GameMode::HandleCivInfoAction);
	HudWidget->OnChatAction.AddDynamic(this, &ARok2GameMode::HandleChatAction);
	HudWidget->OnSeasonStoryAction.AddDynamic(this, &ARok2GameMode::HandleSeasonStoryAction);
	HudWidget->OnResearchAction.AddDynamic(this, &ARok2GameMode::HandleResearchAction);
	// P24-T1: فعلان ورثهما الـHUD من `URok2CityWidget` المتقاعد.
	HudWidget->OnCollectAction.AddDynamic(this, &ARok2GameMode::HandleCollectAction);
	HudWidget->OnTrainAction.AddDynamic(this, &ARok2GameMode::HandleTrainAction);
	// P18-T6: مدخل الإعدادات من الشريط العلوي.
	HudWidget->OnSettingsAction.AddDynamic(this, &ARok2GameMode::HandleSettingsAction);
}

// ---------------------------------------------------------------------------
// P24-T1: تحصيل الإنتاج وورقة التدريب.
//
// كان زرّاهما داخل ألواح `URok2CityWidget` المطوية بـ`ESlateVisibility::Collapsed`،
// فلا يراهما لاعب — و`URok2Api::CollectCityProduction` لم يكن له مستدعٍ آخر في
// المشروع كله. الورقة نفسها هي `URok2TrainHealSheetWidget` التي تفتحها بطاقة
// المبنى، فلا واجهة تدريب ثانية بمنطق ثانٍ.
// ---------------------------------------------------------------------------
void ARok2GameMode::HandleCollectAction()
{
	if (Api)
	{
		Api->CollectCityProduction();
	}
}

void ARok2GameMode::HandleTrainAction()
{
	// الثكنة هي المبنى الافتراضي من الشريط السفلي؛ فتح الورقة من بطاقة مبنى
	// آخر يمرّر معرّفه فتتغيّر قائمة الوحدات إلى فرعه.
	HandleBuildingAction(TEXT("barracks"), TEXT("train"));
}

// ---------------------------------------------------------------------------
// P18-T6: شاشة الإعدادات — المستدعي الغائب لدوال `URok2Accessibility`.
// ---------------------------------------------------------------------------
void ARok2GameMode::HandleSettingsAction()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!SettingsWidget)
	{
		SettingsWidget = Cast<URok2SettingsWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2SettingsWidget::StaticClass()));
	}
	if (SettingsWidget && !SettingsWidget->IsInViewport())
	{
		// فوق اللوحات (50) وتحت طبقة الإرشاد (60): الإعدادات تعلو ما تضبط شكله،
		// ولا تحجب بطاقة الإرشاد.
		SettingsWidget->AddToViewport(58);
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::UiPanelOpen);
		}
	}
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
	}
	if (CommanderWidget)
	{
		// أعد المزامنة كلما فتح اللاعب الشاشة بعد بقائها مخفية.
		CommanderWidget->SetupWithApi(Api);
		if (!CommanderWidget->IsInViewport())
		{
			CommanderWidget->AddToViewport(50);
		}
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
	// الراليات حالات مؤقتة؛ لا نعيد فتح واجهة التحالف بلقطة قديمة.
	Api->FetchAllianceRallies();
}

// ---------------------------------------------------------------------------
// P18-T2: مسار أزرار المباني الثانوية. كان بطاقة المبنى تبث OnBuildingAction
// بلا أي مشترك فأزرار (تدريب/شفاء/بحث/صناديق) تفتح لا شيء. المسارات:
//   research → شاشة البحث (P18-T1) | train/heal → ورقة لمسية (P18-T2)
//   chests → شاشة الحانة (P19-T4) — حتى بنائها يُعلم اللاعب بصدق لا بصمت.
// ---------------------------------------------------------------------------
void ARok2GameMode::HandleBuildingAction(const FString& BuildingId, const FString& ActionKind)
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (ActionKind == TEXT("research"))
	{
		OpenResearchScreen();
	}
	else if (ActionKind == TEXT("train") || ActionKind == TEXT("heal"))
	{
		if (URok2TrainHealSheetWidget* Sheet = Cast<URok2TrainHealSheetWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2TrainHealSheetWidget::StaticClass())))
		{
			Sheet->Setup(Api, ActionKind, BuildingId);
			Sheet->AddToViewport(150);
		}
	}
	else if (ActionKind == TEXT("chests"))
	{
		Api->EmitToast(TEXT("الحانة تُفتح من شاشة الأحداث قريباً (P19-T4)"));
	}
}

// P18-T1: إنشاء كسول كنمط بقية اللوحات — تُنشأ مرة وتبقى متزامنة عبر Setup.
void ARok2GameMode::OpenResearchScreen()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!ResearchWidget)
	{
		ResearchWidget = Cast<URok2ResearchWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2ResearchWidget::StaticClass()));
		if (ResearchWidget)
		{
			ResearchWidget->Setup(Api);
		}
	}
	if (ResearchWidget && !ResearchWidget->IsInViewport())
	{
		ResearchWidget->AddToViewport(50);
		// لقطة حديثة عند كل فتح — المستويات والتكاليف قد تغيّرت منذ الزيارة السابقة.
		Api->FetchResearch();
	}
}


void ARok2GameMode::HandleItemsAction()
{
	if (Api) Api->EmitToast(TEXT("الحقيبة قيد التجهيز — ستظهر العناصر هنا عند توفرها"));
}

void ARok2GameMode::HandleEventsAction()
{
	if (Api) Api->EmitToast(TEXT("لا توجد أحداث نشطة حالياً"));
}

void ARok2GameMode::HandleMapAction()
{
	EnsureViewManager();
	if (!ViewManager) return;

	const bool bOpeningMap = ViewManager->IsCityView();
	ViewManager->ToggleView();
	if (bOpeningMap)
	{
		if (ARok2PlayerController* PC = Cast<ARok2PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)))
		{
			PC->FocusOnPlayerCity();
		}
		if (Api)
		{
			Api->EmitToast(TEXT("جارٍ تحديث خريطة المملكة…"));
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
	// التقرير قد يصل أثناء إغلاق النافذة؛ اقرأ السجل الخاص المصفّى عند كل فتح.
	Api->FetchBattleReports();
}

// ---------------------------------------------------------------------------
// P6-T5: شاشة هوية الحضارة + تحيتها
// ---------------------------------------------------------------------------

void ARok2GameMode::HandleCivInfoAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!CivInfoWidget)
	{
		CivInfoWidget = Cast<URok2CivInfoWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2CivInfoWidget::StaticClass()));
		if (CivInfoWidget)
		{
			CivInfoWidget->Setup(Api);
		}
	}
	if (CivInfoWidget && !CivInfoWidget->IsInViewport())
	{
		// ترتيب 50 كبقية اللوحات (تحت طبقة الإرشاد 60، فوق الـHUD 20)
		CivInfoWidget->AddToViewport(50);
		// تُعاد القراءة عند كل فتح: اللوحة تُنشأ مرة وتُعاد للعرض مراراً، ولو
		// اعتمدنا على Setup وحده لبقيت على حضارة أول حمولة وصلت.
		CivInfoWidget->RefreshFromPlayer();
	}
}

// P18-T1: شاشة البحث — كانت موجودة منذ P2-3 ولا يفتحها أي زر في اللعبة.
void ARok2GameMode::HandleResearchAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!ResearchWidget)
	{
		ResearchWidget = Cast<URok2ResearchWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2ResearchWidget::StaticClass()));
		if (ResearchWidget)
		{
			ResearchWidget->Setup(Api);
		}
	}
	if (ResearchWidget && !ResearchWidget->IsInViewport())
	{
		// نفس طبقة اللوحات: فوق الـHUD وتحت طبقة الإرشاد.
		ResearchWidget->AddToViewport(50);
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::UiPanelOpen);
		}
	}
}

// P7-T1: شاشة حكاية المملكة — تُنشأ عند فتحها وتُملأ من اللقطة المحفوظة.
void ARok2GameMode::HandleSeasonStoryAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!SeasonStoryWidget)
	{
		SeasonStoryWidget = Cast<URok2SeasonStoryWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2SeasonStoryWidget::StaticClass()));
	}
	if (SeasonStoryWidget)
	{
		SeasonStoryWidget->SetStoryEvents(Api->GetSeasonStory());
		if (!SeasonStoryWidget->IsInViewport())
		{
			// نفس طبقة لوحات الدردشة والتقارير، فوق HUD وتحت الإرشاد.
			SeasonStoryWidget->AddToViewport(50);
			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::UiPanelOpen);
			}
		}
		else if (SeasonStoryWidget->GetVisibility() != ESlateVisibility::Visible)
		{
			SeasonStoryWidget->SetVisibility(ESlateVisibility::Visible);
			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::UiPanelOpen);
			}
		}
	}
}

void ARok2GameMode::HandleSeasonStoryEvent(const FRok2SeasonStoryEntry& Event)
{
	// تظل الأحداث مخزنة في API إن كانت الشاشة مغلقة. أما إن كانت منشأة فيُحدّث
	// خطها فوراً، سواءً كانت مرئية أو مخفية في انتظار إعادة فتحها.
	if (SeasonStoryWidget)
	{
		SeasonStoryWidget->AddStoryEvent(Event);
	}
}

// P6-T6: دردشة حية — إنشاء كسند ودجة (نفس نمط AllianceWidget)
void ARok2GameMode::HandleChatAction()
{
	UWorld* World = GetWorld();
	if (!World || !Api) return;

	if (!ChatWidget)
	{
		ChatWidget = Cast<URok2ChatWidget>(
			URok2BlueprintLibrary::CreateRok2Widget(World, URok2ChatWidget::StaticClass()));
		if (ChatWidget)
		{
			ChatWidget->Api = Api;
		}
	}
	if (ChatWidget && !ChatWidget->IsInViewport())
	{
		// ترتيب 50 كبقية اللوحات (تحت طبقة الإرشاد 60، فوق الـHUD 20)
		ChatWidget->AddToViewport(50);
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::UiPanelOpen);
		}
	}
}
