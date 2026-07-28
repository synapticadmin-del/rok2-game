// Copyright ROK2. Unified HUD widget (P2-T6) — implementation.
// Style: شريط استراتيجي داكن بخطوط واضحة — موارد حية، طوابير بتقدم، إشعارات متلاشية.

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
#include "Blueprint/WidgetTree.h"

namespace Rok2HudStyle
{
	static const FLinearColor BarBg(0.02f, 0.05f, 0.12f, 0.94f);
	static const FLinearColor PanelBg(0.04f, 0.07f, 0.14f, 0.90f);
	static const FLinearColor Gold(1.0f, 0.84f, 0.2f);
	static const FLinearColor ResGreen(0.4f, 1.0f, 0.6f);
	static const FLinearColor InfoBlue(0.35f, 0.75f, 1.0f);
	static const FLinearColor WarnAmber(1.0f, 0.65f, 0.25f);
	static const FLinearColor BadRed(1.0f, 0.45f, 0.4f);
	static const FLinearColor OkGreen(0.4f, 1.0f, 0.5f);
	static const FLinearColor Muted(0.75f, 0.78f, 0.85f, 0.9f);
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
}

void URok2HudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudRoot"));
	WidgetTree->RootWidget = RootCanvas;

	BuildTopBar(RootCanvas);
	BuildQueuesPanel(RootCanvas);
	BuildToastsStack(RootCanvas);
	BuildBottomBar(RootCanvas);
	BuildNotifCenter(RootCanvas);
}

// ---------------------------------------------------------------------------
// البناء: شريط علوي بارتفاع 40px تحت شريط CityWidget (54px)
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildTopBar(UCanvasPanel* RootCanvas)
{
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudTopBar"));
	Bar->SetBrushColor(Rok2HudStyle::BarBg);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Bar);
	Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(0.f, 0.f));
	Slot->SetPosition(FVector2D(0.f, 56.f));
	Slot->SetSize(FVector2D(0.f, 38.f));

	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudTopHBox"));
	Bar->SetContent(H);

	HudResourcesText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudResourcesText"));
	HudResourcesText->SetText(FText::FromString(TEXT("🍲 0  🪵 0  🪨 0  🪙 0")));
	HudResourcesText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::ResGreen));
	HudResourcesText->SetFont(Rok2Font(HudResourcesText, 13));
	H->AddChildToHorizontalBox(HudResourcesText)->SetPadding(FMargin(16, 0, 24, 0));

	SeasonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SeasonText"));
	SeasonText->SetText(FText::FromString(TEXT("📅 يوم 0")));
	SeasonText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	SeasonText->SetFont(Rok2Font(SeasonText, 13));
	H->AddChildToHorizontalBox(SeasonText)->SetPadding(FMargin(0, 0, 18, 0));

	ZoneTimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneTimerText"));
	ZoneTimerText->SetText(FText::FromString(TEXT("")));
	ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue));
	ZoneTimerText->SetFont(Rok2Font(ZoneTimerText, 12));
	H->AddChildToHorizontalBox(ZoneTimerText)->SetPadding(FMargin(0, 0, 18, 0));

	// spacer يدفع الجرس والاتصال لليمين
	USpacer* Sp = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HudTopSpacer"));
	UHorizontalBoxSlot* SpSlot = H->AddChildToHorizontalBox(Sp);
	SpSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	ConnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudConnText"));
	ConnText->SetText(FText::FromString(TEXT("🟢")));
	ConnText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::OkGreen));
	ConnText->SetFont(Rok2Font(ConnText, 12));
	H->AddChildToHorizontalBox(ConnText)->SetPadding(FMargin(0, 0, 12, 0));

	UButton* BellBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HudBellBtn"));
	BellBtn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBellClicked);
	BellBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudBellText"));
	BellBadgeText->SetText(FText::FromString(TEXT("🔔")));
	BellBadgeText->SetFont(Rok2Font(BellBadgeText, 13));
	BellBtn->AddChild(BellBadgeText);
	H->AddChildToHorizontalBox(BellBtn)->SetPadding(FMargin(0, 4, 16, 4));
}

// ---------------------------------------------------------------------------
// لوحة الطوابير (يمين أعلى تحت الشريط) — شريط تقدم + عدّ تنازلي لكل طابور
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildQueuesPanel(UCanvasPanel* RootCanvas)
{
	QueuesPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudQueuesPanel"));
	QueuesPanel->SetBrushColor(Rok2HudStyle::PanelBg);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(QueuesPanel);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-12.f, 104.f));
	Slot->SetSize(FVector2D(300.f, 150.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudQueuesVBox"));
	QueuesPanel->SetContent(V);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudQueuesTitle"));
	Title->SetText(FText::FromString(TEXT("⏳ الطوابير")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	Title->SetFont(Rok2Font(Title, 13));
	V->AddChildToVerticalBox(Title)->SetPadding(FMargin(10, 8, 10, 4));

	QueuesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudQueuesBox"));
	V->AddChildToVerticalBox(QueuesBox)->SetPadding(FMargin(10, 0, 10, 8));
}

void URok2HudWidget::BuildToastsStack(UCanvasPanel* RootCanvas)
{
	ToastsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudToastsBox"));
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(ToastsBox);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-12.f, 262.f));
	Slot->SetSize(FVector2D(320.f, 400.f));
}

void URok2HudWidget::BuildBottomBar(UCanvasPanel* RootCanvas)
{
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudBottomBar"));
	Bar->SetBrushColor(Rok2HudStyle::BarBg);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Bar);
	Slot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
	Slot->SetAlignment(FVector2D(0.5f, 1.f));
	Slot->SetPosition(FVector2D(0.f, -8.f));
	Slot->SetSize(FVector2D(360.f, 46.f));

	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudBottomHBox"));
	Bar->SetContent(H);

	// زر الخريطة
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HudMapBtn"));
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudMapBtnText"));
		T->SetText(FText::FromString(TEXT("🗺️ الخريطة")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		T->SetFont(Rok2Font(T, 13));
		B->AddChild(T);
		B->OnClicked.AddDynamic(this, &URok2HudWidget::OnMapBtnClicked);
		H->AddChildToHorizontalBox(B)->SetPadding(FMargin(10, 6, 10, 6));
	}
	// زر التقارير
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HudReportsBtn"));
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudReportsBtnText"));
		T->SetText(FText::FromString(TEXT("📜 التقارير")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		T->SetFont(Rok2Font(T, 13));
		B->AddChild(T);
		B->OnClicked.AddDynamic(this, &URok2HudWidget::OnReportsBtnClicked);
		H->AddChildToHorizontalBox(B)->SetPadding(FMargin(10, 6, 10, 6));
	}
}

void URok2HudWidget::BuildNotifCenter(UCanvasPanel* RootCanvas)
{
	NotifCenterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudNotifCenter"));
	NotifCenterPanel->SetBrushColor(Rok2HudStyle::PanelBg);
	NotifCenterPanel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(NotifCenterPanel);
	Slot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	Slot->SetAlignment(FVector2D(1.f, 0.f));
	Slot->SetPosition(FVector2D(-12.f, 104.f));
	Slot->SetSize(FVector2D(340.f, 420.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudNotifVBox"));
	NotifCenterPanel->SetContent(V);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HudNotifTitle"));
	Title->SetText(FText::FromString(TEXT("🔔 مركز الإشعارات")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold));
	Title->SetFont(Rok2Font(Title, 14));
	V->AddChildToVerticalBox(Title)->SetPadding(FMargin(12, 10, 12, 6));

	NotifList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HudNotifList"));
	V->AddChildToVerticalBox(NotifList)->SetPadding(FMargin(12, 0, 12, 10));
}

// ---------------------------------------------------------------------------
// Tick: موارد حية + عدّ تنازلي للطوابير + تلاشي الإشعارات
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
	}
}

void URok2HudWidget::UpdateResources()
{
	if (!Api || !HudResourcesText) return;
	const FRok2City& C = Api->GetCity();

	double ElapsedSec = 0.0;
	if (C.UpdatedAt > 0)
	{
		const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
		ElapsedSec = FMath::Max(0.0, (double)(NowMs - C.UpdatedAt) / 1000.0);
	}
	const double H = ElapsedSec / 3600.0;
	HudResourcesText->SetText(FText::FromString(FString::Printf(
		TEXT("🍲 %d (+%d)  🪵 %d (+%d)  🪨 %d (+%d)  🪙 %d (+%d)"),
		(int32)(C.Resources.Food + C.Rates.Food * H), (int32)C.Rates.Food,
		(int32)(C.Resources.Wood + C.Rates.Wood * H), (int32)C.Rates.Wood,
		(int32)(C.Resources.Stone + C.Rates.Stone * H), (int32)C.Rates.Stone,
		(int32)(C.Resources.Gold + C.Rates.Gold * H), (int32)C.Rates.Gold)));
}

void URok2HudWidget::UpdateSeasonAndZones()
{
	if (!Api) return;
	const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

	if (SeasonText)
	{
		SeasonText->SetText(FText::FromString(FString::Printf(TEXT("📅 يوم %d"), W.SeasonDay)));
	}

	// أقرب منطقة مقفلة → مؤقّت "Zone X يوم N"
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
			Next = FString::Printf(TEXT("🔒 Zone %d يُفتح يوم %d (بعد %d)"), BestZone, BestDay, BestDay - W.SeasonDay);
			ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue));
		}
		else
		{
			Next = TEXT("🗺️ كل المناطق المجدولة مفتوحة");
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
		if (Shown >= 3) break; // أقصى 3 في اللوحة — الباقي في مركز المدينة
		Shown++;

		UVerticalBox* Item = NewObject<UVerticalBox>(this);

		UTextBlock* Label = NewObject<UTextBlock>(this);
		FString Icon = Q.Type == TEXT("build") ? TEXT("🏗️") : Q.Type == TEXT("research") ? TEXT("🔬") : Q.Type == TEXT("heal") ? TEXT("🏥") : TEXT("⚔️");
		const double RemainSec = FMath::Max(0.0, (double)(Q.EndMs - NowMs) / 1000.0);
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s %s Lv%d — %.0fث"), *Icon, *Q.RefId, Q.Level, RemainSec)));
		Label->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
		Label->SetFont(Rok2Font(Label, 11));
		Item->AddChildToVerticalBox(Label);

		UProgressBar* Bar = NewObject<UProgressBar>(this);
		const double Total = FMath::Max(1.0, (double)(Q.EndMs - Q.StartMs));
		const float Pct = (float)FMath::Clamp((double)(NowMs - Q.StartMs) / Total, 0.0, 1.0);
		Bar->SetPercent(Pct);
		Bar->SetFillColorAndOpacity(Rok2HudStyle::InfoBlue);
		Item->AddChildToVerticalBox(Bar)->SetPadding(FMargin(0, 2, 0, 6));

		QueuesBox->AddChildToVerticalBox(Item);
	}
}

// ---------------------------------------------------------------------------
// إشعارات: بطاقات تتلاشى + مركز إشعارات + عدّاد الجرس
// ---------------------------------------------------------------------------
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
	Body->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
	Body->SetFont(Rok2Font(Body, 11));
	Body->SetAutoWrapText(true);
	V->AddChildToVerticalBox(Body);
	Card->SetContent(V);

	ToastsBox->AddChildToVerticalBox(Card)->SetPadding(FMargin(0, 0, 0, 6));
	ToastCardRefs.Add(Card); // حماية من الـ GC حتى التلاشي

	FToastEntry E;
	E.Id = N.Id;
	E.Remaining = N.TtlSeconds;
	E.Card = Card;
	ActiveToasts.Add(E);

	// حد أقصى 4 بطاقات ظاهرة
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
		Unread > 0 ? Rok2HudStyle::WarnAmber : Rok2HudStyle::Muted));
}

void URok2HudWidget::OnZones(const TArray<FRok2ZoneStatus>& Zones)
{
	UpdateSeasonAndZones();
}

void URok2HudWidget::OnConnState(bool bOnline, const FString& StatusMessage)
{
	if (!ConnText) return;
	ConnText->SetText(FText::FromString(bOnline ? TEXT("🟢") : TEXT("🔴")));
	ConnText->SetColorAndOpacity(FSlateColor(bOnline ? Rok2HudStyle::OkGreen : Rok2HudStyle::BadRed));
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
		Line->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted));
		Line->SetFont(Rok2Font(Line, 11));
		Line->SetAutoWrapText(true);
		NotifList->AddChild(Line);
	}
}

void URok2HudWidget::OnMapBtnClicked()
{
	OnMapAction.Broadcast();
}

void URok2HudWidget::OnReportsBtnClicked()
{
	OnReportsAction.Broadcast();
}
