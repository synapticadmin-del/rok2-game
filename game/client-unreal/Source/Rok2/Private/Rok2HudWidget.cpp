// Copyright ROK2. Unified HUD widget (P5-T3) — implementation.
// أسلوب RoK: برونز داكن + ذهب مزخرف، أزرار دائرية، شريط موارد RTL.

#include "Rok2HudWidget.h"
#include "Rok2Api.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

// لوحة الألوان من ui-ux-design-system.md §1
namespace Rok2HudStyle
{
	static const FLinearColor PanelBg(0.10f, 0.07f, 0.04f, 0.92f);      // #1A120B شبه شفاف
	static const FLinearColor BarBg(0.08f, 0.05f, 0.03f, 0.96f);
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f);                 // #C9A227
	static const FLinearColor Ivory(0.96f, 0.91f, 0.81f);                // #F5E9D0
	static const FLinearColor Danger(0.66f, 0.20f, 0.15f);               // #A93226
	static const FLinearColor Success(0.24f, 0.49f, 0.31f);              // #3E7C4F
	static const FLinearColor ResGreen(0.5f, 0.95f, 0.55f);
	static const FLinearColor GemsCyan(0.45f, 0.85f, 1.0f);
	static const FLinearColor ApPurple(0.75f, 0.55f, 1.0f);
	static const FLinearColor Muted(0.72f, 0.68f, 0.60f, 0.9f);
	static const FLinearColor InfoBlue(0.4f, 0.75f, 1.0f);
}

static FSlateFontInfo Rok2Font(UTextBlock* T, int32 Size)
{
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	return F;
}

void URok2HudWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnHudNotification.AddDynamic(this, &URok2HudWidget::OnNotification);
	Api->OnZonesUpdated.AddDynamic(this, &URok2HudWidget::OnZones);
	Api->OnConnectionState.AddDynamic(this, &URok2HudWidget::OnConnState);

	UpdateSeasonAndZones();
	UpdateQueues();
	UpdateBellBadge();
	UpdateBuildBadge();
}

void URok2HudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudRoot"));
	WidgetTree->RootWidget = RootCanvas;

	BuildTopBar(RootCanvas);
	BuildActionCluster(RootCanvas);
	BuildLeftCluster(RootCanvas);
	BuildQueuesPanel(RootCanvas);
	BuildToastsStack(RootCanvas);
	BuildNotifCenter(RootCanvas);
}

// ---------------------------------------------------------------------------
// شريط الموارد العلوي الذهبي — RTL: طعام/خشب/حجر/ذهب/gems/AP + موسم + مناطق + اتصال + جرس
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildTopBar(UCanvasPanel* RootCanvas)
{
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudTopBar"));
	Bar->SetBrushColor(Rok2HudStyle::BarBg);
	// إطار سفلي ذهبي خفيف عبر Border thickness يُدار من المادة — نستخدم لون الخلفية فقط
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Bar);
	Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(0.f, 0.f));
	Slot->SetPosition(FVector2D(0.f, 0.f));
	Slot->SetSize(FVector2D(0.f, 46.f));

	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudTopHBox"));
	Bar->SetContent(H);

	auto AddRes = [&](UTextBlock*& Out, const FString& Icon, FLinearColor Color) {
		Out = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Out->SetText(FText::FromString(Icon + TEXT(" 0")));
		Out->SetColorAndOpacity(FSlateColor(Color));
		Out->SetFont(Rok2Font(Out, 14));
		H->AddChildToHorizontalBox(Out)->SetPadding(FMargin(14, 0, 2, 0));
	};

	// RTL: نضيف من اليمين — الطعام أولاً
	AddRes(ResFoodText, TEXT("🍲"), Rok2HudStyle::ResGreen);
	AddRes(ResWoodText, TEXT("🪵"), FLinearColor(0.85f, 0.65f, 0.4f));
	AddRes(ResStoneText, TEXT("🪨"), FLinearColor(0.75f, 0.75f, 0.78f));
	AddRes(ResGoldText, TEXT("🪙"), Rok2HudStyle::Gold);
	AddRes(ResGemsText, TEXT("💎"), Rok2HudStyle::GemsCyan);
	AddRes(ResApText, TEXT("⚡"), Rok2HudStyle::ApPurple);

	// فاصل
	USpacer* Sp1 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* Sp1Slot = H->AddChildToHorizontalBox(Sp1);
	Sp1Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	SeasonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SeasonText->SetText(FText::FromString(TEXT("📅 يوم 0")));
	SeasonText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	SeasonText->SetFont(Rok2Font(SeasonText, 13));
	H->AddChildToHorizontalBox(SeasonText)->SetPadding(FMargin(0, 0, 16, 0));

	ZoneTimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ZoneTimerText->SetText(FText::FromString(TEXT("")));
	ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue));
	ZoneTimerText->SetFont(Rok2Font(ZoneTimerText, 12));
	H->AddChildToHorizontalBox(ZoneTimerText)->SetPadding(FMargin(0, 0, 16, 0));

	ConnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConnText->SetText(FText::FromString(TEXT("🟢")));
	ConnText->SetFont(Rok2Font(ConnText, 12));
	H->AddChildToHorizontalBox(ConnText)->SetPadding(FMargin(0, 0, 10, 0));

	UButton* BellBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	BellBtn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBellClicked);
	BellBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BellBadgeText->SetText(FText::FromString(TEXT("🔔")));
	BellBadgeText->SetFont(Rok2Font(BellBadgeText, 13));
	BellBtn->AddChild(BellBadgeText);
	H->AddChildToHorizontalBox(BellBtn)->SetPadding(FMargin(0, 4, 16, 4));
}

// ---------------------------------------------------------------------------
// مجموعة الأزرار الدائرية أسفل يمين — زر البناء الكبير + 4 أصغر (قادة/تحالف/حقيبة/أحداث)
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildActionCluster(UCanvasPanel* RootCanvas)
{
	UOverlay* Cluster = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HudActionCluster"));
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Cluster);
	Slot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
	Slot->SetAlignment(FVector2D(1.f, 1.f));
	Slot->SetPosition(FVector2D(-18.f, -18.f));
	Slot->SetSize(FVector2D(220.f, 220.f));

	// زر البناء الكبير — أسفل يمين الزاوية
	{
		UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildCircle"));
		Circle->SetBrushColor(FLinearColor(0.16f, 0.11f, 0.05f, 0.95f));
		UCanvasPanelSlot* S = Cluster->AddChildToCanvas(Circle);
		S->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(0.f, 0.f));
		S->SetSize(FVector2D(96.f, 96.f));

		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BuildBtn"));
		Circle->SetContent(Btn);
		Btn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBuildClickedHandler);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);
		UTextBlock* Ico = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Ico->SetText(FText::FromString(TEXT("🔨")));
		Ico->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
		Ico->SetFont(Rok2Font(Ico, 34));
		Ico->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Ico)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Lbl->SetText(FText::FromString(TEXT("بناء")));
		Lbl->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
		Lbl->SetFont(Rok2Font(Lbl, 11));
		Lbl->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Lbl)->SetHorizontalAlignment(HAlign_Center);

		// شارة البنّاء الخامل
		BuildBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		BuildBadgeText->SetText(FText::FromString(TEXT("")));
		BuildBadgeText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Danger));
		BuildBadgeText->SetFont(Rok2Font(BuildBadgeText, 12));
		V->AddChildToVerticalBox(BuildBadgeText)->SetHorizontalAlignment(HAlign_Right);
	}

	// الأزرار الأربعة الأصغر — صف أفقي فوق زر البناء
	const float SmallD = 58.f;
	const FVector2D StartPos(-8.f, -110.f);
	int32 i = 0;

	auto SpawnSmall = [&](const FString& Icon, const FString& Label, const FName Handler) {
		UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Circle->SetBrushColor(Rok2HudStyle::PanelBg);
		UCanvasPanelSlot* S = Cluster->AddChildToCanvas(Circle);
		S->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(StartPos + FVector2D(-i * (SmallD + 8.f), 0.f));
		S->SetSize(FVector2D(SmallD, SmallD));

		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Circle->SetContent(Btn);
		Btn->OnClicked.AddDynamic(this, Handler);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);
		UTextBlock* Ico = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Ico->SetText(FText::FromString(Icon));
		Ico->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
		Ico->SetFont(Rok2Font(Ico, 20));
		Ico->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Ico)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Lbl->SetText(FText::FromString(Label));
		Lbl->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
		Lbl->SetFont(Rok2Font(Lbl, 8));
		Lbl->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Lbl)->SetHorizontalAlignment(HAlign_Center);
		i++;
	};

	SpawnSmall(TEXT("🪖"), TEXT("قادة"), FName(TEXT("OnCommandersClickedHandler")));
	SpawnSmall(TEXT("🛡️"), TEXT("تحالف"), FName(TEXT("OnAllianceClickedHandler")));
	SpawnSmall(TEXT("🎒"), TEXT("حقيبة"), FName(TEXT("OnItemsClickedHandler")));
	SpawnSmall(TEXT("🚩"), TEXT("أحداث"), FName(TEXT("OnEventsClickedHandler")));
}

// ---------------------------------------------------------------------------
// مجموعة أسفل يسار/وسط — خريطة + تقارير + تحرير المدينة
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildLeftCluster(UCanvasPanel* RootCanvas)
{
	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudLeftCluster"));
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(H);
	Slot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	Slot->SetAlignment(FVector2D(0.f, 1.f));
	Slot->SetPosition(FVector2D(18.f, -18.f));
	Slot->SetSize(FVector2D(340.f, 52.f));

	auto MakePill = [&](const FString& IconLabel, const FName Handler) {
		UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Pill->SetBrushColor(Rok2HudStyle::PanelBg);
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Pill->SetContent(Btn);
		Btn->OnClicked.AddDynamic(this, Handler);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(IconLabel));
		T->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
		T->SetFont(Rok2Font(T, 13));
		Btn->AddChild(T);
		H->AddChildToHorizontalBox(Pill)->SetPadding(FMargin(0, 0, 10, 0));
	};

	MakePill(TEXT("🗺️ الخريطة"), FName(TEXT("OnMapBtnClickedHandler")));
	MakePill(TEXT("📜 التقارير"), FName(TEXT("OnReportsBtnClickedHandler")));
	MakePill(TEXT("🏗️ تحرير المدينة"), FName(TEXT("OnEditCityClickedHandler")));
}

// ---------------------------------------------------------------------------
// لوحة الطوابير (يمين أعلى)
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildQueuesPanel(UCanvasPanel* RootCanvas)
{
	QueuesPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudQueuesPanel"));
	QueuesPanel->SetBrushColor(Rok2HudStyle::PanelBg);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(QueuesPanel);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-14.f, 56.f));
	Slot->SetSize(FVector2D(280.f, 150.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	QueuesPanel->SetContent(V);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("⏳ الطوابير")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	Title->SetFont(Rok2Font(Title, 13));
	V->AddChildToVerticalBox(Title)->SetPadding(FMargin(10, 8, 10, 4));

	QueuesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	V->AddChildToVerticalBox(QueuesBox)->SetPadding(FMargin(10, 0, 10, 8));
}

void URok2HudWidget::BuildToastsStack(UCanvasPanel* RootCanvas)
{
	ToastsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudToastsBox"));
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ToastsBox);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-14.f, 216.f));
	Slot->SetSize(FVector2D(300.f, 400.f));
}

void URok2HudWidget::BuildNotifCenter(UCanvasPanel* RootCanvas)
{
	NotifCenterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudNotifCenter"));
	NotifCenterPanel->SetBrushColor(Rok2HudStyle::PanelBg);
	NotifCenterPanel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(NotifCenterPanel);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-14.f, 56.f));
	Slot->SetSize(FVector2D(320.f, 420.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	NotifCenterPanel->SetContent(V);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("🔔 مركز الإشعارات")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	Title->SetFont(Rok2Font(Title, 14));
	V->AddChildToVerticalBox(Title)->SetPadding(FMargin(12, 10, 12, 6));

	NotifList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	V->AddChildToVerticalBox(NotifList)->SetPadding(FMargin(12, 0, 12, 10));
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------
void URok2HudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateResources();
	TickToasts(InDeltaTime);

	QueuesRefreshTimer += InDeltaTime;
	if (QueuesRefreshTimer >= 0.5f)
	{
		QueuesRefreshTimer = 0.f;
		UpdateQueues();
		UpdateSeasonAndZones();
		UpdateBuildBadge();
	}
}

void URok2HudWidget::UpdateResources()
{
	if (!Api) return;
	const FRok2City& C = Api->GetCity();

	double ElapsedSec = 0.0;
	if (C.UpdatedAt > 0)
	{
		const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
		ElapsedSec = FMath::Max(0.0, (double)(NowMs - C.UpdatedAt) / 1000.0);
	}
	const double H = ElapsedSec / 3600.0;

	auto SetRes = [&](UTextBlock* T, const FString& Icon, double Val, double Rate) {
		if (!T) return;
		// تنسيق مختصر (1.2M / 845K)
		auto Fmt = [](double V) -> FString {
			if (V >= 1e6) return FString::Printf(TEXT("%.1fM"), V / 1e6);
			if (V >= 1e3) return FString::Printf(TEXT("%.1fK"), V / 1e3);
			return FString::FromInt((int32)V);
		};
		T->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), *Icon, *Fmt(Val + Rate * H))));
	};

	SetRes(ResFoodText, TEXT("🍲"), C.Resources.Food, C.Rates.Food);
	SetRes(ResWoodText, TEXT("🪵"), C.Resources.Wood, C.Rates.Wood);
	SetRes(ResStoneText, TEXT("🪨"), C.Resources.Stone, C.Rates.Stone);
	SetRes(ResGoldText, TEXT("🪙"), C.Resources.Gold, C.Rates.Gold);
	// ملاحظة: Gems و ActionPoints تُقرأ من الخادم عند إتاحتهما في FRok2City (يُسجَّل في PLAN) — تُعرض 0 مؤقتاً
	if (ResGemsText) ResGemsText->SetText(FText::FromString(TEXT("💎 0")));
	if (ResApText) ResApText->SetText(FText::FromString(TEXT("⚡ 0")));
}

void URok2HudWidget::UpdateSeasonAndZones()
{
	if (!Api) return;
	const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

	if (SeasonText)
	{
		SeasonText->SetText(FText::FromString(FString::Printf(TEXT("📅 يوم %d"), W.SeasonDay)));
	}

	if (ZoneTimerText)
	{
		FString Next;
		int32 BestDay = MAX_int32;
		int32 BestZone = 0;
		for (const FRok2ZoneStatus& Z : W.Zones)
		{
			if (!Z.bUnlocked && Z.UnlockDay > W.SeasonDay && Z.UnlockDay < BestDay)
			{
				BestDay = Z.UnlockDay;
				BestZone = Z.ZoneId;
			}
		}
		if (BestDay != MAX_int32)
		{
			Next = FString::Printf(TEXT("🔒 Zone %d يوم %d"), BestZone, BestDay);
			ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue));
		}
		else
		{
			Next = TEXT("🗺️ المناطق مفتوحة");
			ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
		}
		ZoneTimerText->SetText(FText::FromString(Next));
	}
}

void URok2HudWidget::UpdateQueues()
{
	if (!Api || !QueuesBox) return;
	const FRok2City& C = Api->GetCity();
	QueuesBox->ClearChildren();

	if (C.ActiveQueues.Num() == 0)
	{
		UTextBlock* Empty = NewObject<UTextBlock>(this);
		Empty->SetText(FText::FromString(TEXT("لا طوابير نشطة")));
		Empty->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
		Empty->SetFont(Rok2Font(Empty, 11));
		QueuesBox->AddChildToVerticalBox(Empty);
		return;
	}

	const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	int32 Shown = 0;
	for (const FRok2QueueEntry& Q : C.ActiveQueues)
	{
		if (Shown >= 3) break;
		Shown++;

		UVerticalBox* Item = NewObject<UVerticalBox>(this);

		UTextBlock* Label = NewObject<UTextBlock>(this);
		FString Icon = Q.Type == TEXT("build") ? TEXT("🏗️") : Q.Type == TEXT("research") ? TEXT("🔬") : Q.Type == TEXT("heal") ? TEXT("🏥") : TEXT("⚔️");
		const double RemainSec = FMath::Max(0.0, (double)(Q.EndMs - NowMs) / 1000.0);
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s %s Lv%d — %.0fث"), *Icon, *Q.RefId, Q.Level, RemainSec)));
		Label->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
		Label->SetFont(Rok2Font(Label, 11));
		Item->AddChildToVerticalBox(Label);

		UProgressBar* Bar = NewObject<UProgressBar>(this);
		const double Total = FMath::Max(1.0, (double)(Q.EndMs - Q.StartMs));
		const float Pct = (float)FMath::Clamp((double)(NowMs - Q.StartMs) / Total, 0.0, 1.0);
		Bar->SetPercent(Pct);
		Bar->SetFillColorAndOpacity(Rok2HudStyle::Gold);
		Item->AddChildToVerticalBox(Bar)->SetPadding(FMargin(0, 2, 0, 6));

		QueuesBox->AddChildToVerticalBox(Item);
	}
}

void URok2HudWidget::UpdateBuildBadge()
{
	if (!Api || !BuildBadgeText) return;
	// بنّاء خامل؟ لا يوجد طابور build نشط
	const FRok2City& C = Api->GetCity();
	bool bBuilding = false;
	for (const FRok2QueueEntry& Q : C.ActiveQueues)
	{
		if (Q.Type == TEXT("build")) { bBuilding = true; break; }
	}
	BuildBadgeText->SetText(FText::FromString(bBuilding ? TEXT("") : TEXT("!")));
}

void URok2HudWidget::OnNotification(const FRok2HudNotification& N)
{
	if (!ToastsBox) return;

	UBorder* Card = NewObject<UBorder>(this);
	FLinearColor Bg = Rok2HudStyle::PanelBg;
	if (N.Kind == TEXT("combat")) Bg = FLinearColor(0.22f, 0.08f, 0.08f, 0.95f);
	else if (N.Kind == TEXT("zone")) Bg = FLinearColor(0.06f, 0.14f, 0.24f, 0.95f);
	else if (N.Kind == TEXT("rally")) Bg = FLinearColor(0.18f, 0.14f, 0.04f, 0.95f);
	Card->SetBrushColor(Bg);
	Card->SetPadding(FMargin(8));

	UVerticalBox* V = NewObject<UVerticalBox>(this);
	UTextBlock* Title = NewObject<UTextBlock>(this);
	Title->SetText(FText::FromString(N.Title));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	Title->SetFont(Rok2Font(Title, 12));
	V->AddChildToVerticalBox(Title);
	UTextBlock* Body = NewObject<UTextBlock>(this);
	Body->SetText(FText::FromString(N.Body));
	Body->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
	Body->SetFont(Rok2Font(Body, 11));
	Body->SetAutoWrapText(true);
	V->AddChildToVerticalBox(Body);
	Card->SetContent(V);

	ToastsBox->AddChildToVerticalBox(Card)->SetPadding(FMargin(0, 0, 0, 6));
	ToastCardRefs.Add(Card);

	FToastEntry E;
	E.Id = N.Id;
	E.Remaining = N.TtlSeconds;
	E.Card = Card;
	ActiveToasts.Add(E);

	while (ActiveToasts.Num() > 4)
	{
		if (ActiveToasts[0].Card)
		{
			ActiveToasts[0].Card->RemoveFromParent();
			ToastCardRefs.Remove(ActiveToasts[0].Card);
		}
		ActiveToasts.RemoveAt(0);
	}

	UpdateBellBadge();
}

void URok2HudWidget::TickToasts(float DeltaSeconds)
{
	for (int32 i = ActiveToasts.Num() - 1; i >= 0; --i)
	{
		FToastEntry& E = ActiveToasts[i];
		E.Remaining -= DeltaSeconds;
		if (E.Card && E.Remaining < 1.5f)
		{
			E.Card->SetRenderOpacity(FMath::Clamp(E.Remaining / 1.5f, 0.f, 1.f));
		}
		if (E.Remaining <= 0.f)
		{
			if (E.Card)
			{
				E.Card->RemoveFromParent();
				ToastCardRefs.Remove(E.Card);
			}
			ActiveToasts.RemoveAt(i);
		}
	}
}

void URok2HudWidget::UpdateBellBadge()
{
	if (!Api || !BellBadgeText) return;
	const int32 Unread = Api->GetUnreadNotificationsCount();
	BellBadgeText->SetText(FText::FromString(
		Unread > 0 ? FString::Printf(TEXT("🔔 %d"), Unread) : TEXT("🔔")));
	BellBadgeText->SetColorAndOpacity(FSlateColor(
		Unread > 0 ? Rok2HudStyle::Danger : Rok2HudStyle::Muted));
}

void URok2HudWidget::OnZones(const TArray<FRok2ZoneStatus>& Zones)
{
	UpdateSeasonAndZones();
}

void URok2HudWidget::OnConnState(bool bOnline, const FString& StatusMessage)
{
	if (!ConnText) return;
	ConnText->SetText(FText::FromString(bOnline ? TEXT("🟢") : TEXT("🔴")));
	ConnText->SetColorAndOpacity(FSlateColor(bOnline ? Rok2HudStyle::Success : Rok2HudStyle::Danger));
}

void URok2HudWidget::OnBellClicked()
{
	if (!Api || !NotifCenterPanel || !NotifList) return;
	bNotifCenterOpen = !bNotifCenterOpen;
	NotifCenterPanel->SetVisibility(bNotifCenterOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!bNotifCenterOpen) return;

	Api->MarkNotificationsRead();
	UpdateBellBadge();

	NotifList->ClearChildren();
	const TArray<FRok2HudNotification>& All = Api->GetNotifications();
	if (All.Num() == 0)
	{
		UTextBlock* Empty = NewObject<UTextBlock>(this);
		Empty->SetText(FText::FromString(TEXT("لا إشعارات بعد")));
		Empty->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
		NotifList->AddChild(Empty);
		return;
	}
	for (const FRok2HudNotification& N : All)
	{
		UTextBlock* Line = NewObject<UTextBlock>(this);
		Line->SetText(FText::FromString(FString::Printf(TEXT("%s — %s"), *N.Title, *N.Body)));
		Line->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory));
		Line->SetFont(Rok2Font(Line, 11));
		Line->SetAutoWrapText(true);
		NotifList->AddChild(Line);
	}
}

// ---------------------------------------------------------------------------
// معالجات أزرار الأكشن — تبثّ الأحداث المفوَّضة للخارج
// ---------------------------------------------------------------------------
void URok2HudWidget::OnBuildClickedHandler() { OnBuildAction.Broadcast(); }
void URok2HudWidget::OnMapBtnClickedHandler() { OnMapAction.Broadcast(); }
void URok2HudWidget::OnReportsBtnClickedHandler() { OnReportsAction.Broadcast(); }
void URok2HudWidget::OnEditCityClickedHandler() { OnEditCityAction.Broadcast(); }
void URok2HudWidget::OnCommandersClickedHandler() { OnCommandersAction.Broadcast(); }
void URok2HudWidget::OnAllianceClickedHandler() { OnAllianceAction.Broadcast(); }
void URok2HudWidget::OnItemsClickedHandler() { OnItemsAction.Broadcast(); }
void URok2HudWidget::OnEventsClickedHandler() { OnEventsAction.Broadcast(); }
