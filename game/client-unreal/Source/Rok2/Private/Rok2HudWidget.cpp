// Copyright ROK2. Unified HUD widget (P5-T3) — implementation.
// أسلوب RoK: برونز داكن + ذهب مزخرف، أزرار دائرية، شريط موارد RTL.
// P6-T1: كل الأيقونات إجرائية من URok2ArtAssets — لا إيموجي في الواجهة.
// P6-T3: بطاقات الإشعارات تنبثق من الأسفل + كل زر بضغطة محسوسة (URok2MotionLibrary).

#include "Rok2HudWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
#include "Rok2AudioManager.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
#include "Rok2DelegateBind.h"
#include "Rok2Onboarding.h"
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

// الألوان كلها من Rok2Visual — كانت هنا لوحة محلية `Rok2HudStyle` تعيد كتابة
// الذهب والعاج والصامت بنفس القيم، وتعرّف Danger/Success أدكن من رموز المشروع.
namespace Rok2HudStyle
{
	static const FLinearColor& PanelBg() { return Rok2Visual::Panel(); }
	static const FLinearColor& BarBg()   { return Rok2Visual::Bar(); }
	static const FLinearColor& Gold()    { return Rok2Visual::GoldText(); }
	static const FLinearColor& Ivory()   { return Rok2Visual::Ivory(); }
	static const FLinearColor& Danger()  { return Rok2Visual::DangerText(); }
	static const FLinearColor& Success() { return Rok2Visual::SuccessText(); }
	static const FLinearColor& Muted()   { return Rok2Visual::Muted(); }
	static const FLinearColor& InfoBlue(){ return Rok2Visual::InformationText(); }
}

// P6-T1: مُنشئ UImage لأيقونة إجرائية بحجم ولون
static UImage* Rok2Icon(UWidgetTree* Tree, const FString& IconId, float Size, FLinearColor Tint)
{
	UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	const float Scaled = URok2Accessibility::Get()->ScaledIconSize(Size);
	Img->SetBrush(URok2ArtAssets::GetIconBrush(IconId, Scaled, Tint));
	Img->SetDesiredSizeOverride(FVector2D(Scaled, Scaled));
	Img->SetToolTipText(URok2Accessibility::LabelForIcon(IconId));
	return Img;
}

void URok2HudWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnHudNotification.AddDynamic(this, &URok2HudWidget::OnNotification);
	Api->OnToast.AddDynamic(this, &URok2HudWidget::OnToast);
	Api->OnZonesUpdated.AddDynamic(this, &URok2HudWidget::OnZones);
	Api->OnConnectionState.AddDynamic(this, &URok2HudWidget::OnConnState);

	UpdateSeasonAndZones();
	UpdateQueues();
	UpdateBellBadge();
	UpdateBuildBadge();
	UpdateChatBadge();
	UpdateNotifications();
}

TSharedRef<SWidget> URok2HudWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	if (!WidgetTree->RootWidget)
	{
		NativeConstruct();
	}
	return Super::RebuildWidget();
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
// شريط الموارد العلوي الذهبي — معلومات الحاكم والقوة + الموارد + الموسم + اتصال + جرس
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildTopBar(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	const FMargin Safe = URok2Accessibility::GetSafeAreaPadding();
	UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudTopBar"));
	Bar->SetBrush(Rok2Surface::TopBar());
	// الشريط يمتد بعرض الشاشة كاملاً (الخلفية تحت النتوء مقبولة)، لكن محتواه
	// يُزاح بالحواف الآمنة حتى لا يقع اسم الحاكم أو الجرس تحت كاميرا الهاتف.
	Bar->SetPadding(FMargin(Safe.Left, 0.f, Safe.Right, 0.f));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Bar);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
	PanelSlot->SetPosition(FVector2D(0.f, 0.f));
	PanelSlot->SetSize(FVector2D(0.f, (A11y ? A11y->GetScaledPx(48.f) : 48.f) + Safe.Top));

	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudTopHBox"));
	Bar->SetContent(H);

	// معلومات الحاكم والقوة (Avatar + Name + Power)
	{
		UImage* GovIcon = Rok2Icon(WidgetTree, TEXT("crown"), 20.f, Rok2HudStyle::Gold());
		UHorizontalBoxSlot* GovIcoSlot = H->AddChildToHorizontalBox(GovIcon);
		GovIcoSlot->SetPadding(FMargin(A11y ? A11y->GetScaledPx(12.f) : 12.f, 0, A11y ? A11y->GetScaledPx(4.f) : 4.f, 0));
		GovIcoSlot->SetVerticalAlignment(VAlign_Center);
		GovIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		GovernorNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GovernorNameText"));
		GovernorNameText->SetText(FText::FromString(TEXT("الحاكم")));
		GovernorNameText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory()));
		URok2Typography::ApplyFont(GovernorNameText, ERok2TextRole::Subtitle);
		UHorizontalBoxSlot* NameSlot = H->AddChildToHorizontalBox(GovernorNameText);
		NameSlot->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(8.f) : 8.f, 0));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		UImage* SwordIcon = Rok2Icon(WidgetTree, TEXT("sword"), 16.f, Rok2HudStyle::Gold());
		UHorizontalBoxSlot* SwordIcoSlot = H->AddChildToHorizontalBox(SwordIcon);
		SwordIcoSlot->SetPadding(FMargin(A11y ? A11y->GetScaledPx(4.f) : 4.f, 0, A11y ? A11y->GetScaledPx(2.f) : 2.f, 0));
		SwordIcoSlot->SetVerticalAlignment(VAlign_Center);
		SwordIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		GovernorPowerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GovernorPowerText"));
		// لا نعرض رقماً افتراضياً قبل وصول الملف السلطوي؛ الشرطة حالة تحميل صادقة.
		GovernorPowerText->SetText(FText::FromString(TEXT("—")));
		GovernorPowerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
		URok2Typography::ApplyFont(GovernorPowerText, ERok2TextRole::Numeric);
		UHorizontalBoxSlot* PowerSlot = H->AddChildToHorizontalBox(GovernorPowerText);
		PowerSlot->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(16.f) : 16.f, 0));
		PowerSlot->SetVerticalAlignment(VAlign_Center);
	}

	// الموارد
	auto AddRes = [&](UTextBlock*& Out, const FString& IconId, FLinearColor Color) {
		UImage* Ico = Rok2Icon(WidgetTree, IconId, 18.f, Color);
		UHorizontalBoxSlot* IcoSlot = H->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(A11y ? A11y->GetScaledPx(10.f) : 10.f, 0, A11y ? A11y->GetScaledPx(4.f) : 4.f, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		Out = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		// حالة تحميل محايدة حتى تصل موارد المدينة من الخادم؛ لا قيم وهمية.
		Out->SetText(FText::FromString(TEXT("—")));
		Out->SetColorAndOpacity(FSlateColor(Color));
		URok2Typography::ApplyFont(Out, ERok2TextRole::Numeric);
		UHorizontalBoxSlot* TxtSlot = H->AddChildToHorizontalBox(Out);
		TxtSlot->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(2.f) : 2.f, 0));
		TxtSlot->SetVerticalAlignment(VAlign_Center);
	};

	AddRes(ResFoodText, TEXT("food"), Rok2Visual::ResourceFood());
	AddRes(ResWoodText, TEXT("wood"), Rok2Visual::ResourceWood());
	AddRes(ResStoneText, TEXT("stone"), Rok2Visual::ResourceStone());
	AddRes(ResGoldText, TEXT("gold"), Rok2HudStyle::Gold());
	AddRes(ResGemsText, TEXT("gems"), Rok2Visual::ResourceGems());
	AddRes(ResApText, TEXT("ap"), Rok2Visual::ResourceActionPoints());

	// فاصل
	USpacer* Sp1 = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	UHorizontalBoxSlot* Sp1Slot = H->AddChildToHorizontalBox(Sp1);
	Sp1Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// يوم الموسم
	{
		UImage* Ico = Rok2Icon(WidgetTree, TEXT("calendar"), 16.f, Rok2HudStyle::Gold());
		UHorizontalBoxSlot* IcoSlot = H->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 4, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	SeasonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SeasonText->SetText(FText::FromString(TEXT("يوم 0")));
	SeasonText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
	URok2Typography::ApplyFont(SeasonText, ERok2TextRole::Caption);
	H->AddChildToHorizontalBox(SeasonText)->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(16.f) : 16.f, 0));

	ZoneTimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ZoneTimerText->SetText(FText::FromString(TEXT("المناطق مفتوحة")));
	ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue()));
	URok2Typography::ApplyFont(ZoneTimerText, ERok2TextRole::Caption);
	H->AddChildToHorizontalBox(ZoneTimerText)->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(16.f) : 16.f, 0));

	// شارة الاتصال
	ConnIcon = Rok2Icon(WidgetTree, TEXT("conn"), 14.f, Rok2HudStyle::Success());
	{
		UHorizontalBoxSlot* IcoSlot = H->AddChildToHorizontalBox(ConnIcon);
		IcoSlot->SetPadding(FMargin(0, 0, 4, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	ConnStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ConnStateText->SetText(FText::FromString(TEXT("متصل")));
	ConnStateText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Success()));
	URok2Typography::ApplyFont(ConnStateText, ERok2TextRole::Caption);
	H->AddChildToHorizontalBox(ConnStateText)->SetPadding(FMargin(0, 0, A11y ? A11y->GetScaledPx(10.f) : 10.f, 0));

	UButton* BellBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	BellBtn->SetStyle(Rok2Surface::GhostButton());
	BellBtn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBellClicked);
	BellIcon = Rok2Icon(WidgetTree, TEXT("bell"), 18.f, Rok2HudStyle::Muted());
	BellBtn->AddChild(BellIcon);
	H->AddChildToHorizontalBox(BellBtn)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XS, Rok2Space::XS, Rok2Space::XS));

	BellBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BellBadgeText->SetText(FText::FromString(TEXT("")));
	BellBadgeText->SetColorAndOpacity(FSlateColor(URok2Accessibility::HighContrastForState(false)));
	URok2Typography::ApplyFont(BellBadgeText, ERok2TextRole::Caption);
	H->AddChildToHorizontalBox(BellBadgeText)->SetPadding(FMargin(0, 4, A11y ? A11y->GetScaledPx(16.f) : 16.f, 4));
}

// ---------------------------------------------------------------------------
// مجموعة الأزرار الدائرية أسفل يمين — زر البناء الكبير + 4 أصغر
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildActionCluster(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	const FMargin Safe = URok2Accessibility::GetSafeAreaPadding();
	UCanvasPanel* Cluster = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudActionCluster"));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Cluster);
	PanelSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 1.f));
	// أزرار العمل هي أكثر ما يُلمس، فتُزاح داخل الحدود الآمنة قبل كل شيء آخر.
	PanelSlot->SetPosition(FVector2D(
		-((A11y ? A11y->GetScaledPx(18.f) : 18.f) + Safe.Right),
		-((A11y ? A11y->GetScaledPx(18.f) : 18.f) + Safe.Bottom)));
	PanelSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(220.f) : 220.f, A11y ? A11y->GetScaledPx(220.f) : 220.f));

	// زر البناء الكبير
	{
		UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildCircle"));
		Circle->SetBrush(Rok2Surface::Circle(Rok2Visual::PrimaryAction()));
		UCanvasPanelSlot* S = Cluster->AddChildToCanvas(Circle);
		S->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(0.f, 0.f));
		const float BuildSize = A11y ? A11y->GetScaledPx(96.f) : 96.f;
		S->SetSize(FVector2D(BuildSize, BuildSize));

		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BuildBtn"));
		Btn->SetStyle(Rok2Surface::GhostButton());
		Circle->SetContent(Btn);
		Btn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBuildClickedHandler);
		URok2MotionLibrary::BindPress(Btn, Circle);

		URok2Onboarding::Get()->RegisterAnchor(Rok2FtueSpec::AnchorBuild, Circle);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);
		UImage* Ico = Rok2Icon(WidgetTree, TEXT("build"), 40.f, Rok2HudStyle::Ivory());
		V->AddChildToVerticalBox(Ico)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Lbl->SetText(FText::FromString(TEXT("بناء")));
		Lbl->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
		URok2Typography::ApplyFont(Lbl, ERok2TextRole::Micro);
		Lbl->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Lbl)->SetHorizontalAlignment(HAlign_Center);

		BuildBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		BuildBadgeText->SetText(FText::FromString(TEXT("")));
		BuildBadgeText->SetColorAndOpacity(FSlateColor(URok2Accessibility::HighContrastForState(false)));
		URok2Typography::ApplyFont(BuildBadgeText, ERok2TextRole::Caption);
		V->AddChildToVerticalBox(BuildBadgeText)->SetHorizontalAlignment(HAlign_Right);
	}

	const float SmallD = A11y ? A11y->GetScaledPx(56.f) : 56.f;
	const FVector2D StartPos(A11y ? A11y->GetScaledPx(-8.f) : -8.f, A11y ? A11y->GetScaledPx(-108.f) : -108.f);
	int32 i = 0;

	auto SpawnSmall = [&](const FString& IconId, const FString& Label, const FName Handler) {
		UBorder* Circle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Circle->SetBrush(Rok2Surface::Circle(Rok2Visual::Panel()));
		UCanvasPanelSlot* S = Cluster->AddChildToCanvas(Circle);
		S->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(StartPos + FVector2D(-i * (SmallD + (A11y ? A11y->GetScaledPx(Rok2Space::S) : Rok2Space::S)), 0.f));
		S->SetSize(FVector2D(SmallD, SmallD));

		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(Rok2Surface::GhostButton());
		Circle->SetContent(Btn);
		Rok2BindClickByName(Btn, this, Handler);
		URok2MotionLibrary::BindPress(Btn, Circle);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);
		UImage* Ico = Rok2Icon(WidgetTree, IconId, 24.f, Rok2HudStyle::Ivory());
		V->AddChildToVerticalBox(Ico)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Lbl->SetText(FText::FromString(Label));
		Lbl->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
		URok2Typography::ApplyFont(Lbl, ERok2TextRole::Micro);
		Lbl->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Lbl)->SetHorizontalAlignment(HAlign_Center);
		i++;
	};

	SpawnSmall(TEXT("helmet"), TEXT("قادة"), FName(TEXT("OnCommandersClickedHandler")));
	SpawnSmall(TEXT("shield"), TEXT("تحالف"), FName(TEXT("OnAllianceClickedHandler")));
	SpawnSmall(TEXT("bag"), TEXT("حقيبة"), FName(TEXT("OnItemsClickedHandler")));
	SpawnSmall(TEXT("banner"), TEXT("أحداث"), FName(TEXT("OnEventsClickedHandler")));
	SpawnSmall(TEXT("crown"), TEXT("حضارتي"), FName(TEXT("OnCivInfoClickedHandler")));
	// البحث العلمي: كانت شاشته موجودة ولا يفتحها شيء — SpawnSmall سادسة.
	SpawnSmall(TEXT("research"), TEXT("بحث"), FName(TEXT("OnResearchClickedHandler")));
}

// ---------------------------------------------------------------------------
// مجموعة أسفل يسار/وسط — خريطة + تقارير + تحرير المدينة
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildLeftCluster(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	const FMargin Safe = URok2Accessibility::GetSafeAreaPadding();
	UHorizontalBox* H = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HudLeftCluster"));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(H);
	PanelSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
	PanelSlot->SetPosition(FVector2D(
		(A11y ? A11y->GetScaledPx(18.f) : 18.f) + Safe.Left,
		-((A11y ? A11y->GetScaledPx(18.f) : 18.f) + Safe.Bottom)));
	PanelSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(470.f) : 470.f, A11y ? A11y->GetScaledPx(52.f) : 52.f));

	auto MakePill = [&](const FString& IconId, const FString& Label, const FName Handler) -> UBorder* {
		UBorder* Pill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Pill->SetBrush(Rok2Surface::OutlinedPill(Rok2Visual::Panel(), Rok2Visual::Edge()));
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(Rok2Surface::GhostButton());
		Pill->SetContent(Btn);
		Rok2BindClickByName(Btn, this, Handler);
		URok2MotionLibrary::BindPress(Btn, Pill);
		UHorizontalBox* PillBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Btn->AddChild(PillBox);
		UImage* Ico = Rok2Icon(WidgetTree, IconId, 16.f, Rok2HudStyle::Ivory());
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(IconId));
		Btn->SetToolTipText(FText::FromString(Label));
		UHorizontalBoxSlot* IcoSlot = PillBox->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::S, Rok2Space::Hair, Rok2Space::XS, Rok2Space::Hair));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Label));
		T->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory()));
		URok2Typography::ApplyFont(T, ERok2TextRole::Caption);
		UHorizontalBoxSlot* TxtSlot = PillBox->AddChildToHorizontalBox(T);
		TxtSlot->SetPadding(FMargin(Rok2Space::Hair, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		TxtSlot->SetVerticalAlignment(VAlign_Center);
		H->AddChildToHorizontalBox(Pill)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
		return Pill;
	};

	UBorder* MapPill = MakePill(TEXT("map"), TEXT("الخريطة"), FName(TEXT("OnMapBtnClickedHandler")));
	MakePill(TEXT("scroll"), TEXT("التقارير"), FName(TEXT("OnReportsBtnClickedHandler")));
	MakePill(TEXT("scroll"), TEXT("حكاية المملكة"), FName(TEXT("OnSeasonStoryClickedHandler")));
	MakePill(TEXT("edit"), TEXT("تحرير المدينة"), FName(TEXT("OnEditCityClickedHandler")));

	// P24-T1: حبّتان ورثهما الـHUD من `URok2CityWidget` المتقاعد. زرّاهما هناك
	// كانا داخل ألواح مطوية بـ`Collapsed` فلا يراهما لاعب، وكانا المستدعي
	// الوحيد لـ`CollectCityProduction` و`Train` في المشروع.
	MakePill(TEXT("food"), TEXT("تحصيل"), FName(TEXT("OnCollectClickedHandler")));
	UBorder* TrainPill = MakePill(TEXT("sword"), TEXT("تدريب"), FName(TEXT("OnTrainClickedHandler")));

	// زر الدردشة الحية
	{
		ChatButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ChatPill"));
		// كان `SetNormal(FSlateNoResource())` وحده: حشو معدوم وبلا تحويم ولا ضغط،
		// بينما جيرانه الأربعة في الصف نفسه تحصل على رد فعل. الآن حبّة كاملة.
		ChatButton->SetStyle(Rok2Surface::TintedButton(Rok2Visual::Panel()));
		UHorizontalBox* PillBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		ChatButton->AddChild(PillBox);
		ChatIcon = Rok2Icon(WidgetTree, TEXT("bell"), 14.f, Rok2Visual::InformationText());
		UHorizontalBoxSlot* IcoSlot = PillBox->AddChildToHorizontalBox(ChatIcon);
		IcoSlot->SetPadding(FMargin(6, 0, 4, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("الدردشة")));
		T->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory()));
		URok2Typography::ApplyFont(T, ERok2TextRole::Caption);
		PillBox->AddChildToHorizontalBox(T)->SetPadding(FMargin(2, 0, 6, 0));
		ChatBadgeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ChatBadgeText->SetText(FText::GetEmpty());
		ChatBadgeText->SetColorAndOpacity(FSlateColor(URok2Accessibility::HighContrastForState(false)));
		URok2Typography::ApplyFont(ChatBadgeText, ERok2TextRole::Caption);
		PillBox->AddChildToHorizontalBox(ChatBadgeText)->SetVerticalAlignment(VAlign_Center);
		ChatIcon->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("bell")));
		ChatButton->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("chat")));
		ChatButton->OnClicked.AddDynamic(this, &URok2HudWidget::OnChatClickedHandler);
		URok2MotionLibrary::BindPress(ChatButton);
		H->AddChildToHorizontalBox(ChatButton)->SetPadding(FMargin(0, 0, 10, 0));
	}

	URok2Onboarding::Get()->RegisterAnchor(Rok2FtueSpec::AnchorMap, MapPill);

	// P24-T1: مرساة خطوة «درّب أول جنودك» كانت مسجّلة على زر داخل لوح مطوي في
	// `URok2CityWidget`، فكانت هندسته صفرية والحلقة الذهبية تُخفى أبداً. الحبّة
	// هنا مرئية فعلاً، فيُشير الإرشاد إلى ما يستطيع اللاعب لمسه.
	URok2Onboarding::Get()->RegisterAnchor(Rok2FtueSpec::AnchorTrain, TrainPill);
}

// ---------------------------------------------------------------------------
// لوحة الطوابير (يمين أعلى)
// ---------------------------------------------------------------------------
void URok2HudWidget::BuildQueuesPanel(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	QueuesPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudQueuesPanel"));
	QueuesPanel->SetBrush(Rok2Surface::Panel());
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(QueuesPanel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(A11y ? A11y->GetScaledPx(-14.f) : -14.f, A11y ? A11y->GetScaledPx(56.f) : 56.f));
	PanelSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(280.f) : 280.f, A11y ? A11y->GetScaledPx(150.f) : 150.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	QueuesPanel->SetContent(V);

	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	V->AddChildToVerticalBox(TitleRow)->SetPadding(FMargin(10, 8, 10, 4));
	{
		UImage* Ico = Rok2Icon(WidgetTree, TEXT("hourglass"), 15.f, Rok2HudStyle::Gold());
		UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("الطوابير")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
	URok2Typography::ApplyFont(Title, ERok2TextRole::TitleCompact);
	TitleRow->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

	QueuesBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	V->AddChildToVerticalBox(QueuesBox)->SetPadding(FMargin(10, 0, 10, 8));
}

void URok2HudWidget::BuildToastsStack(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	ToastsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudToastsBox"));
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(ToastsBox);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(A11y ? A11y->GetScaledPx(-14.f) : -14.f, A11y ? A11y->GetScaledPx(216.f) : 216.f));
	PanelSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(300.f) : 300.f, A11y ? A11y->GetScaledPx(400.f) : 400.f));
}

void URok2HudWidget::BuildNotifCenter(UCanvasPanel* RootCanvas)
{
	URok2Accessibility* A11y = URok2Accessibility::Get();
	NotifCenterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HudNotifCenter"));
	NotifCenterPanel->SetBrush(Rok2Surface::Panel());
	NotifCenterPanel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(NotifCenterPanel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(A11y ? A11y->GetScaledPx(-14.f) : -14.f, A11y ? A11y->GetScaledPx(56.f) : 56.f));
	PanelSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(320.f) : 320.f, A11y ? A11y->GetScaledPx(420.f) : 420.f));

	UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	NotifCenterPanel->SetContent(V);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	V->AddChildToVerticalBox(Header)->SetPadding(FMargin(12, 10, 12, 6));
	{
		UImage* Ico = Rok2Icon(WidgetTree, TEXT("bell"), 16.f, Rok2HudStyle::Gold());
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 6, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("مركز الإشعارات")));
	Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
	URok2Typography::ApplyFont(Title, ERok2TextRole::TitleCompact);
	Header->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

	USpacer* Sp = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Header->AddChildToHorizontalBox(Sp)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CloseBtn->SetStyle(Rok2Surface::GhostButton());
	CloseBtn->OnClicked.AddDynamic(this, &URok2HudWidget::OnBellClicked);
	UTextBlock* X = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	X->SetText(FText::FromString(TEXT("✕")));
	X->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted()));
	URok2Typography::ApplyFont(X, ERok2TextRole::Caption);
	CloseBtn->AddChild(X);
	Header->AddChildToHorizontalBox(CloseBtn)->SetVerticalAlignment(VAlign_Center);

	NotifList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	V->AddChildToVerticalBox(NotifList)->SetPadding(FMargin(12, 0, 12, 12));
}

void URok2HudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	HudRefreshAccumulator += InDeltaTime;
	if (HudRefreshAccumulator < 0.25f) return;
	HudRefreshAccumulator = 0.f;
	UpdateResources();
	UpdateQueues();
	UpdateBuildBadge();
	UpdateNotifications();
}

void URok2HudWidget::OnBuildClickedHandler() { OnBuildAction.Broadcast(); }
void URok2HudWidget::OnCommandersClickedHandler() { OnCommandersAction.Broadcast(); }
void URok2HudWidget::OnAllianceClickedHandler() { OnAllianceAction.Broadcast(); }
void URok2HudWidget::OnItemsClickedHandler() { OnItemsAction.Broadcast(); }
void URok2HudWidget::OnEventsClickedHandler() { OnEventsAction.Broadcast(); }
void URok2HudWidget::OnCivInfoClickedHandler() { OnCivInfoAction.Broadcast(); }
void URok2HudWidget::OnMapBtnClickedHandler() { OnMapAction.Broadcast(); }
void URok2HudWidget::OnReportsBtnClickedHandler() { OnReportsAction.Broadcast(); }
void URok2HudWidget::OnEditCityClickedHandler() { OnEditCityAction.Broadcast(); }
void URok2HudWidget::OnChatClickedHandler() { OnChatAction.Broadcast(); }
void URok2HudWidget::OnSeasonStoryClickedHandler() { OnSeasonStoryAction.Broadcast(); }
void URok2HudWidget::OnResearchClickedHandler() { OnResearchAction.Broadcast(); }
void URok2HudWidget::OnCollectClickedHandler() { OnCollectAction.Broadcast(); }
void URok2HudWidget::OnTrainClickedHandler() { OnTrainAction.Broadcast(); }

void URok2HudWidget::OnBellClicked()
{
	if (!NotifCenterPanel) return;
	const bool bVisible = NotifCenterPanel->GetVisibility() == ESlateVisibility::Visible;
	NotifCenterPanel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (!bVisible && Api)
	{
		Api->MarkNotificationsRead();
		UpdateBellBadge();
		UpdateNotifications();
	}
}

void URok2HudWidget::UpdateResources()
{
	if (!Api) return;
	const FRok2City& C = Api->GetCity();

	if (Api->HasPlayer())
	{
		const FRok2Player& P = Api->GetPlayer();
		if (GovernorNameText)
		{
			GovernorNameText->SetText(FText::FromString(P.Name.IsEmpty() ? TEXT("الحاكم") : P.Name));
		}
		if (GovernorPowerText)
		{
			const int32 Power = FMath::Max(0, P.Power);
			GovernorPowerText->SetText(FText::FromString(FString::Printf(TEXT("%s"), *FText::AsNumber(Power).ToString())));
		}
	}

	const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	double ElapsedSec = 0.0;
	if (C.UpdatedAt > 0 && NowMs > C.UpdatedAt)
	{
		ElapsedSec = FMath::Max(0.0, (double)(NowMs - C.UpdatedAt) / 1000.0);
	}
	const double H = ElapsedSec / 3600.0;

	auto SetRes = [&](UTextBlock* T, double Val, double Rate) {
		if (!T) return;
		auto Fmt = [](double V) -> FString {
			if (V >= 1e6) return FString::Printf(TEXT("%.1fM"), V / 1e6);
			if (V >= 1e3) return FString::Printf(TEXT("%.1fK"), V / 1e3);
			return FString::FromInt((int32)V);
		};
		T->SetText(FText::FromString(Fmt(Val + Rate * H)));
	};

	SetRes(ResFoodText, C.Resources.Food, C.Rates.Food);
	SetRes(ResWoodText, C.Resources.Wood, C.Rates.Wood);
	SetRes(ResStoneText, C.Resources.Stone, C.Rates.Stone);
	SetRes(ResGoldText, C.Resources.Gold, C.Rates.Gold);
	SetRes(ResGemsText, C.Gems, 0.0);
	SetRes(ResApText, Api->GetWorldSnapshot().ApState.Ap, 0.0);
}

void URok2HudWidget::UpdateSeasonAndZones()
{
	if (!Api) return;
	const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

	if (SeasonText)
	{
		SeasonText->SetText(FText::FromString(FString::Printf(TEXT("يوم %d"), W.SeasonDay)));
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
			Next = FString::Printf(TEXT("Zone %d يوم %d"), BestZone, BestDay);
			ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::InfoBlue()));
		}
		else
		{
			Next = TEXT("المناطق مفتوحة");
			ZoneTimerText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted()));
	URok2Typography::ApplyFont(ZoneTimerText, ERok2TextRole::Caption);
		}
		ZoneTimerText->SetText(FText::FromString(Next));
	}
}

void URok2HudWidget::UpdateQueues()
{
	if (!Api || !QueuesBox) return;
	const FRok2City& C = Api->GetCity();
	QueuesBox->ClearChildren();
	// الوسائط تُعاد بناؤها مع الصفوف؛ بلا تفريغ تتراكم عبر الجلسة كلها.
	QueueActions.Empty();

	if (C.ActiveQueues.Num() == 0)
	{
		UTextBlock* Empty = NewObject<UTextBlock>(this);
		Empty->SetText(FText::FromString(TEXT("لا طوابير نشطة")));
		Empty->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Muted()));
		URok2Typography::ApplyFont(Empty, ERok2TextRole::Caption);
		QueuesBox->AddChildToVerticalBox(Empty);
		return;
	}

	const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	int32 Shown = 0;
	for (const FRok2QueueEntry& Q : C.ActiveQueues)
	{
		if (Shown >= 3) break;
		Shown++;

		UHorizontalBox* ItemRow = NewObject<UHorizontalBox>(this);
		// P24-T1: أسماء الطوابير كانت `RefId Lv%d` — معرّف خادمي لاتيني في واجهة
		// عربية. الترجمة من نفس أسماء `Api->GetMeta()` التي تستخدمها ورقة التدريب،
		// فلا اختراع اسم في العميل.
		const bool bBuildQueue = (Q.Type == TEXT("build") || Q.Type == TEXT("building"));
		const bool bResearchQueue = (Q.Type == TEXT("research"));
		const bool bHealQueue = (Q.Type == TEXT("heal") || Q.Type == TEXT("healing"));
		const TCHAR* IconId = bBuildQueue ? TEXT("build")
			: bResearchQueue ? TEXT("flask")
			: bHealQueue ? TEXT("cross")
			: TEXT("sword");
		const FLinearColor RowColor = bBuildQueue ? Rok2HudStyle::InfoBlue()
			: bResearchQueue ? Rok2Visual::ResourceActionPoints()
			: bHealQueue ? Rok2HudStyle::Success()
			: Rok2HudStyle::Gold();

		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 14.f, RowColor));
		Ico->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(FString(IconId)));
		UHorizontalBoxSlot* IcoSlot = ItemRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UVerticalBox* Item = NewObject<UVerticalBox>(this);
		ItemRow->AddChildToHorizontalBox(Item)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UTextBlock* Label = NewObject<UTextBlock>(this);
		const double RemainSec = FMath::Max(0.0, (double)(Q.EndMs - NowMs) / 1000.0);
		const int32 Minutes = (int32)RemainSec / 60;
		const int32 Seconds = (int32)RemainSec % 60;
		const FString Verb = bBuildQueue ? TEXT("ترقية")
			: bResearchQueue ? TEXT("بحث")
			: bHealQueue ? TEXT("شفاء")
			: TEXT("تدريب");
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s %s · %02d:%02d"),
			*Verb, *QueueSubjectName(Q), Minutes, Seconds)));
		Label->SetColorAndOpacity(FSlateColor(RowColor));
		URok2Typography::ApplyFont(Label, ERok2TextRole::Micro);
		Item->AddChildToVerticalBox(Label);

		UProgressBar* Bar = NewObject<UProgressBar>(this);
		const double Total = FMath::Max(1.0, (double)(Q.EndMs - Q.StartMs));
		const float Pct = (float)FMath::Clamp((double)(NowMs - Q.StartMs) / Total, 0.0, 1.0);
		Bar->SetPercent(Pct);

		// مسار حجري مستدير بدل الشريط المستطيل الافتراضي — من §4 في وثيقة الهوية.
		FProgressBarStyle BarStyle;
		BarStyle.SetBackgroundImage(Rok2Surface::ProgressTrack());
		BarStyle.SetFillImage(Rok2Surface::ProgressFill(RowColor));
		Bar->SetWidgetStyle(BarStyle);
		Bar->SetFillColorAndOpacity(FLinearColor::White);

		Item->AddChildToVerticalBox(Bar)->SetPadding(FMargin(Rok2Space::None, Rok2Space::Hair, Rok2Space::None, Rok2Space::S));

		// P24-T1: زر التسريع. `SpeedupQueue` كان بلا أي مستدعٍ بعد تقاعد
		// `URok2CityWidget`، أي أن إنهاء الطابور بالجواهر كان غير قابل للوصول.
		{
			URok2HudQueueAction* Action = NewObject<URok2HudQueueAction>(this);
			Action->QueueId = Q.Id;
			Action->Api = Api;
			QueueActions.Add(Action);

			UButton* Speedup = NewObject<UButton>(this);
			Speedup->SetStyle(Rok2Surface::SecondaryButton());
			Speedup->OnClicked.AddDynamic(Action, &URok2HudQueueAction::HandleClick);
			Speedup->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("speedup")));
			URok2MotionLibrary::BindPress(Speedup);

			UHorizontalBox* BtnBox = NewObject<UHorizontalBox>(this);
			Speedup->AddChild(BtnBox);
			UImage* SpdIco = NewObject<UImage>(this);
			SpdIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("speedup"), 12.f, Rok2HudStyle::Ivory()));
			SpdIco->SetDesiredSizeOverride(FVector2D(12.f, 12.f));
			UHorizontalBoxSlot* SpdIcoSlot = BtnBox->AddChildToHorizontalBox(SpdIco);
			SpdIcoSlot->SetPadding(FMargin(Rok2Space::XS, Rok2Space::None, Rok2Space::Hair, Rok2Space::None));
			SpdIcoSlot->SetVerticalAlignment(VAlign_Center);
			SpdIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			UTextBlock* BtnText = NewObject<UTextBlock>(this);
			// السعر يأتي من الخادم؛ عند غيابه لا نخترع رقماً بل نُسمّي الفعل.
			BtnText->SetText(FText::FromString(Q.FinishCostGems > 0
				? FString::Printf(TEXT("%d ج"), Q.FinishCostGems)
				: TEXT("إنهاء")));
			BtnText->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory()));
			URok2Typography::ApplyFont(BtnText, ERok2TextRole::Micro);
			BtnBox->AddChildToHorizontalBox(BtnText)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));

			UHorizontalBoxSlot* BtnSlot = ItemRow->AddChildToHorizontalBox(Speedup);
			BtnSlot->SetVerticalAlignment(VAlign_Center);
			BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		QueuesBox->AddChildToVerticalBox(ItemRow);
	}
}

FString URok2HudWidget::QueueSubjectName(const FRok2QueueEntry& Q) const
{
	if (!Api)
	{
		return Q.RefId;
	}

	// وحدة قابلة للتدريب؟ الاسم العربي جاهز في بيانات التوازن الموحّدة.
	for (const FRok2TrainableUnit& Unit : Api->GetMeta().TrainableUnits)
	{
		if (Unit.Id == Q.RefId && !Unit.Name.IsEmpty())
		{
			return Q.Level > 0 ? FString::Printf(TEXT("%s Lv%d"), *Unit.Name, Q.Level) : Unit.Name;
		}
	}

	// وإلا مبنى: خريطة أسماء buildings.json العربية. المعرّف نفسه احتياطٌ صادق
	// أفضل من ترجمة مخترعة لمبنى لم يُسمَّ بعد.
	static const TMap<FString, FString> BuildingNames = {
		{ TEXT("city_hall"),       TEXT("قاعة المدينة") },
		{ TEXT("farm"),            TEXT("مزرعة") },
		{ TEXT("lumber_mill"),     TEXT("منشرة") },
		{ TEXT("quarry"),          TEXT("محجر") },
		{ TEXT("goldmine"),        TEXT("منجم ذهب") },
		{ TEXT("storehouse"),      TEXT("مخزن") },
		{ TEXT("barracks"),        TEXT("ثكنة") },
		{ TEXT("stable"),          TEXT("إسطبل") },
		{ TEXT("archery_range"),   TEXT("ميدان رماية") },
		{ TEXT("siege_workshop"),  TEXT("ورشة حصار") },
		{ TEXT("hospital"),        TEXT("مستشفى") },
		{ TEXT("wall"),            TEXT("سور") },
		{ TEXT("watchtower"),      TEXT("برج مراقبة") },
		{ TEXT("academy"),         TEXT("أكاديمية") },
		{ TEXT("tavern"),          TEXT("حانة") },
		{ TEXT("trading_post"),    TEXT("مركز تبادل") },
		{ TEXT("alliance_center"), TEXT("مركز التحالف") },
		{ TEXT("scout_camp"),      TEXT("معسكر كشافة") },
		{ TEXT("castle"),          TEXT("قلعة") },
		{ TEXT("shop"),            TEXT("متجر") },
		{ TEXT("courier_station"), TEXT("مركز بريد") },
		{ TEXT("builders_hut"),    TEXT("كوخ البنّائين") },
		{ TEXT("monument"),        TEXT("نصب") },
	};
	const FString* Found = BuildingNames.Find(Q.RefId);
	const FString Base = Found ? *Found : Q.RefId;
	return Q.Level > 0 ? FString::Printf(TEXT("%s → %d"), *Base, Q.Level) : Base;
}

void URok2HudQueueAction::HandleClick()
{
	if (Api)
	{
		Api->SpeedupQueue(QueueId);
	}
}

void URok2HudWidget::UpdateBuildBadge()
{
	if (!Api || !BuildBadgeText) return;
	const FRok2City& C = Api->GetCity();
	bool bBuilding = false;
	for (const FRok2QueueEntry& Q : C.ActiveQueues)
	{
		if (Q.Type == TEXT("build")) { bBuilding = true; break; }
	}
	BuildBadgeText->SetText(FText::FromString(bBuilding ? TEXT("") : TEXT("(خامل)")));
	BuildBadgeText->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("build_idle")));
}

void URok2HudWidget::OnNotification(const FRok2HudNotification& N)
{
	UpdateBellBadge();
}

void URok2HudWidget::OnToast(const FString& Message)
{
	if (!ToastsBox || Message.IsEmpty()) return;
	while (ToastsBox->GetChildrenCount() >= 3)
	{
		ToastsBox->RemoveChildAt(0);
	}
	UBorder* Card = NewObject<UBorder>(this);
	Card->SetBrush(Rok2Surface::AccentCard(Rok2Visual::Gold()));
	Card->SetPadding(FMargin(Rok2Space::M, Rok2Space::S));
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(FText::FromString(Message));
	Text->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	Text->SetAutoWrapText(true);
	URok2Typography::ApplyFont(Text, ERok2TextRole::Body);
	Card->SetContent(Text);
	ToastsBox->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.f, Rok2Space::XS));
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::Notification);
	}
}

void URok2HudWidget::UpdateNotifications()
{
	if (!Api || !NotifList) return;
	const TArray<FRok2HudNotification>& Items = Api->GetNotifications();
	if (Items.Num() == LastNotifCount) return;
	LastNotifCount = Items.Num();

	NotifList->ClearChildren();
	for (const FRok2HudNotification& Item : Items)
	{
		UBorder* Card = NewObject<UBorder>(this);
		Card->SetBrush(Rok2Surface::Card());
		Card->SetPadding(FMargin(8));

		UVerticalBox* V = NewObject<UVerticalBox>(this);
		UTextBlock* Title = NewObject<UTextBlock>(this);
		Title->SetText(FText::FromString(Item.Title));
		Title->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Gold()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Caption);
		V->AddChildToVerticalBox(Title);

		UTextBlock* Body = NewObject<UTextBlock>(this);
		Body->SetText(FText::FromString(Item.Body));
		Body->SetColorAndOpacity(FSlateColor(Rok2HudStyle::Ivory()));
		URok2Typography::ApplyFont(Body, ERok2TextRole::Micro);
		Body->SetAutoWrapText(true);
		V->AddChildToVerticalBox(Body);

		Card->SetContent(V);
		NotifList->AddChild(Card);
	}
}

void URok2HudWidget::UpdateBellBadge()
{
	if (!Api || !BellBadgeText) return;
	const int32 Unread = Api->GetUnreadNotificationsCount();
	BellBadgeText->SetText(FText::FromString(Unread > 0 ? FString::Printf(TEXT("%d"), Unread) : TEXT("")));
	if (BellIcon)
	{
		BellIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("bell"), 18.f,
			Unread > 0 ? Rok2HudStyle::Gold() : Rok2HudStyle::Muted()));
	}
}

void URok2HudWidget::UpdateChatBadge()
{
	if (!Api || !ChatBadgeText) return;
	const int32 Unread = Api->GetUnreadChatCount();
	ChatBadgeText->SetText(Unread > 0 ? FText::FromString(FString::Printf(TEXT("(%d)"), Unread)) : FText::GetEmpty());
}

void URok2HudWidget::OnZones(const TArray<FRok2ZoneStatus>& Zones)
{
	UpdateSeasonAndZones();
}

void URok2HudWidget::OnConnState(bool bOnline, const FString& StatusMessage)
{
	if (ConnIcon)
	{
		const FLinearColor StateColor = URok2Accessibility::HighContrastForState(bOnline);
		ConnIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("conn"), 14.f, StateColor));
	}
	if (ConnStateText)
	{
		ConnStateText->SetText(FText::FromString(bOnline ? TEXT("متصل") : TEXT("منقطع")));
		ConnStateText->SetColorAndOpacity(FSlateColor(URok2Accessibility::HighContrastForState(bOnline)));
	}
}
