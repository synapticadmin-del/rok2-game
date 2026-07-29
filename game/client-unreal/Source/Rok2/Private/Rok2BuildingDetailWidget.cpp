// Copyright ROK2. Building card — Bottom Sheet implementation (P5-T3).
// P6-T1: أيقونات إجرائية من URok2ArtAssets بدل الإيموجي في العناوين والأزرار.
// P6-T3: حركة الدخول والضغطات من URok2MotionLibrary (بدل انزلاق محلي في Tick).

#include "Rok2BuildingDetailWidget.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/Spacer.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

namespace Rok2CardStyle
{
	static const FLinearColor SheetBg(0.10f, 0.07f, 0.04f, 0.97f);
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f);
	static const FLinearColor Ivory(0.96f, 0.91f, 0.81f);
	static const FLinearColor ResGreen(0.5f, 0.95f, 0.55f);
	static const FLinearColor Muted(0.72f, 0.68f, 0.60f, 0.9f);
	static const FLinearColor BtnGold(0.55f, 0.42f, 0.10f);
	static const FLinearColor BtnGhost(0.20f, 0.16f, 0.12f);
}

static FSlateFontInfo CardFont(UTextBlock* T, int32 Size)
{
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	return F;
}

void URok2BuildingDetailWidget::SetupBuilding(URok2Api* InApi, const FString& InBuildingId, int32 InLevel)
{
	Api = InApi;
	BuildingId = InBuildingId;
	CurrentLevel = InLevel;

	FString DisplayName = BuildingId;
	FString Desc = TEXT("");
	FString IconId = TEXT("build");
	if (BuildingId == TEXT("city_hall")) { IconId = TEXT("castle"); DisplayName = TEXT("قاعة المدينة"); Desc = TEXT("قلب مملكتك — سقف كل التقدم"); }
	else if (BuildingId == TEXT("farm")) { IconId = TEXT("wheat"); DisplayName = TEXT("المزرعة"); Desc = TEXT("تنتج الطعام لجنودك وشعبك"); }
	else if (BuildingId == TEXT("lumber_mill")) { IconId = TEXT("wood"); DisplayName = TEXT("المنشرة"); Desc = TEXT("تنتج الخشب للبناء والتدريب"); }
	else if (BuildingId == TEXT("quarry")) { IconId = TEXT("rock"); DisplayName = TEXT("المحجر"); Desc = TEXT("تنتج الحجر للمباني المتقدمة"); }
	else if (BuildingId == TEXT("goldmine")) { IconId = TEXT("pickaxe"); DisplayName = TEXT("منجم الذهب"); Desc = TEXT("ينتج الذهب للتدريب والبحث"); }
	else if (BuildingId == TEXT("barracks")) { IconId = TEXT("sword"); DisplayName = TEXT("الثكنة"); Desc = TEXT("تدريب وحدات المشاة"); }
	else if (BuildingId == TEXT("stable")) { IconId = TEXT("horse"); DisplayName = TEXT("الإسطبل"); Desc = TEXT("تدريب وحدات الفرسان"); }
	else if (BuildingId == TEXT("archery_range")) { IconId = TEXT("bow"); DisplayName = TEXT("ميدان الرماية"); Desc = TEXT("تدريب وحدات الرماة"); }
	else if (BuildingId == TEXT("siege_workshop")) { IconId = TEXT("wrench"); DisplayName = TEXT("ورشة الحصار"); Desc = TEXT("بناء أسلحة الحصار"); }
	else if (BuildingId == TEXT("hospital")) { IconId = TEXT("cross"); DisplayName = TEXT("المستشفى"); Desc = TEXT("شفاء الجرحى الخطيرين بعد المعارك"); }
	else if (BuildingId == TEXT("wall")) { IconId = TEXT("bricks"); DisplayName = TEXT("السور"); Desc = TEXT("خط الدفاع الأول عن مدينتك"); }
	else if (BuildingId == TEXT("storehouse")) { IconId = TEXT("box"); DisplayName = TEXT("المخزن"); Desc = TEXT("يحمي مواردك من النهب"); }
	else if (BuildingId == TEXT("academy")) { IconId = TEXT("flask"); DisplayName = TEXT("الأكاديمية"); Desc = TEXT("بحث التقنيات الاقتصادية والعسكرية"); }
	else if (BuildingId == TEXT("tavern")) { IconId = TEXT("beer"); DisplayName = TEXT("الحانة"); Desc = TEXT("استدعاء القادة وفتح الصناديق"); }
	else if (BuildingId == TEXT("castle")) { IconId = TEXT("castle"); DisplayName = TEXT("القلعة"); Desc = TEXT("قيادة حملات الـ Rally الجماعية"); }
	else if (BuildingId == TEXT("watchtower")) { IconId = TEXT("tower"); DisplayName = TEXT("برج المراقبة"); Desc = TEXT("كشف الهجمات والكشافة القادمة"); }
	else if (BuildingId == TEXT("scout_camp")) { IconId = TEXT("tent"); DisplayName = TEXT("معسكر الكشافة"); Desc = TEXT("إرسال كشافة لكشف ضباب الحرب"); }
	else if (BuildingId == TEXT("alliance_center")) { IconId = TEXT("handshake"); DisplayName = TEXT("مركز التحالف"); Desc = TEXT("مساعدات وتعزيزات الأعضاء"); }
	else if (BuildingId == TEXT("trading_post")) { IconId = TEXT("scale"); DisplayName = TEXT("مركز التجارة"); Desc = TEXT("إرسال الموارد لأعضاء التحالف"); }
	else if (BuildingId == TEXT("monument")) { IconId = TEXT("monument"); DisplayName = TEXT("النصب"); Desc = TEXT("تقدّم عصر المملكة ومكافآته"); }
	else if (BuildingId == TEXT("builders_hut")) { IconId = TEXT("builder"); DisplayName = TEXT("كوخ البنّائين"); Desc = TEXT("طوابير البناء الإضافية"); }
	else if (BuildingId == TEXT("courier_station")) { IconId = TEXT("mail"); DisplayName = TEXT("محطة البريد"); Desc = TEXT("رسائل المملكة والتحالف"); }
	else if (BuildingId == TEXT("shop")) { IconId = TEXT("cart"); DisplayName = TEXT("المتجر"); Desc = TEXT("شراء العناصر والتسريعات"); }

	// P6-T1: أيقونة المبنى الإجرائية في الترويسة (تُزرع في HeaderIconBox عند البناء)
	if (TitleText) TitleText->SetText(FText::FromString(DisplayName));
	if (HeaderIcon) HeaderIcon->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 28.f, Rok2CardStyle::Gold));
	if (DescText) DescText->SetText(FText::FromString(Desc));
	if (LevelText) LevelText->SetText(FText::FromString(FString::Printf(TEXT("المستوى %d  ➔  %d"), CurrentLevel, CurrentLevel + 1)));

	// التكلفة (placeholder — تُقرأ من data/buildings.json عند ربط الـ meta)
	const int32 FoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	const int32 WoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	const int32 TimeSec = FMath::RoundToInt(60.f * FMath::Pow(1.6f, (float)CurrentLevel));
	if (CostFoodText) CostFoodText->SetText(FText::FromString(FString::FromInt(FoodCost)));
	if (CostText) CostText->SetText(FText::FromString(FString::FromInt(WoodCost)));
	if (CostFoodIcon) CostFoodIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("food"), 16.f, Rok2CardStyle::ResGreen));
	if (CostWoodIcon) CostWoodIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("wood"), 16.f, Rok2CardStyle::ResGreen));
	if (TimeText) TimeText->SetText(FText::FromString(FString::Printf(TEXT("%dث"), TimeSec)));
	if (TimeIcon) TimeIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("clock"), 16.f, Rok2CardStyle::Muted));

	// الزر الثانوي حسب النوع
	const FString ActionLabel = ActionLabelForBuilding(BuildingId);
	if (ActionBtnText) ActionBtnText->SetText(FText::FromString(ActionLabel));
	if (ActionBtnIcon)
	{
		const FString ActionIcon = ActionIconForBuilding(BuildingId);
		ActionBtnIcon->SetBrush(URok2ArtAssets::GetIconBrush(ActionIcon, 18.f, Rok2CardStyle::Ivory));
		ActionBtnIcon->SetVisibility(ActionIcon.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (ActionButton) ActionButton->SetVisibility(ActionLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

FString URok2BuildingDetailWidget::ActionKindForBuilding(const FString& Id) const
{
	if (Id == TEXT("barracks") || Id == TEXT("stable") || Id == TEXT("archery_range") || Id == TEXT("siege_workshop")) return TEXT("train");
	if (Id == TEXT("hospital")) return TEXT("heal");
	if (Id == TEXT("academy")) return TEXT("research");
	if (Id == TEXT("tavern")) return TEXT("chests");
	return TEXT("");
}

FString URok2BuildingDetailWidget::ActionLabelForBuilding(const FString& Id) const
{
	const FString K = ActionKindForBuilding(Id);
	if (K == TEXT("train")) return TEXT("تدريب");
	if (K == TEXT("heal")) return TEXT("شفاء");
	if (K == TEXT("research")) return TEXT("بحث");
	if (K == TEXT("chests")) return TEXT("صناديق");
	return TEXT("");
}

// P6-T1: أيقونة الزر الثانوي الإجرائية حسب نوع الإجراء
FString URok2BuildingDetailWidget::ActionIconForBuilding(const FString& Id) const
{
	const FString K = ActionKindForBuilding(Id);
	if (K == TEXT("train")) return TEXT("sword");
	if (K == TEXT("heal")) return TEXT("cross");
	if (K == TEXT("research")) return TEXT("flask");
	if (K == TEXT("chests")) return TEXT("gift");
	return TEXT("");
}

void URok2BuildingDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// خلفية معتمة خفيفة خلف البطاقة (تلمس للإغلاق)
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetSize(FVector2D(0.f, 0.f));
	Backdrop->OnClicked.AddDynamic(this, &URok2BuildingDetailWidget::OnCloseClicked);

	// البطاقة — Bottom Sheet بعرض كامل أسفل الشاشة
	SheetBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SheetBorder"));
	SheetBorder->SetBrushColor(Rok2CardStyle::SheetBg);
	UCanvasPanelSlot* SheetSlot = RootCanvas->AddChildToCanvas(SheetBorder);
	SheetSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	SheetSlot->SetAlignment(FVector2D(0.5f, 1.f));
	SheetSlot->SetPosition(FVector2D(0.f, 0.f));
	SheetSlot->SetSize(FVector2D(0.f, 300.f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SheetBorder->SetContent(VBox);

	// مقبض السحب العلوي (شريط صغير)
	UBorder* Handle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Handle->SetBrushColor(Rok2CardStyle::Gold);
	UVerticalBoxSlot* HandleSlot = VBox->AddChildToVerticalBox(Handle);
	HandleSlot->SetHorizontalAlignment(HAlign_Center);
	HandleSlot->SetPadding(FMargin(0, 8, 0, 4));
	// حجم المقبض يُدار من SizeBox لاحقاً — نكتفي بالشريط الرفيع هنا

	// P6-T1: ترويسة البطاقة = أيقونة المبنى + عنوان، بمحاذاة مركزية
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(0, 2, 0, 2));
	HeaderRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	HeaderIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	HeaderIcon->SetDesiredSizeOverride(FVector2D(28.f, 28.f));
	UHorizontalBoxSlot* HeaderIcoSlot = HeaderRow->AddChildToHorizontalBox(HeaderIcon);
	HeaderIcoSlot->SetPadding(FMargin(0, 0, 8, 0));
	HeaderIcoSlot->SetVerticalAlignment(VAlign_Center);
	HeaderIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Gold));
	TitleText->SetFont(CardFont(TitleText, 22));
	HeaderRow->AddChildToHorizontalBox(TitleText)->SetVerticalAlignment(VAlign_Center);
	HeaderRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DescText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Muted));
	DescText->SetFont(CardFont(DescText, 12));
	DescText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(DescText)->SetPadding(FMargin(0, 0, 0, 6));

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LevelText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Ivory));
	LevelText->SetFont(CardFont(LevelText, 15));
	LevelText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(LevelText)->SetPadding(FMargin(0, 0, 0, 10));

	// صف التكلفة والوقت — P6-T1: أيقونة طعام + رقم، أيقونة خشب + رقم، أيقونة ساعة + مدة
	UHorizontalBox* CostRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(CostRow)->SetHorizontalAlignment(HAlign_Center);
	CostRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	auto MakeCostPair = [&](UImage*& OutIco, UTextBlock*& OutTxt, FLinearColor TxtColor) {
		OutIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		OutIco->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
		UHorizontalBoxSlot* IcoSlot = CostRow->AddChildToHorizontalBox(OutIco);
		IcoSlot->SetPadding(FMargin(8, 0, 3, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		OutTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutTxt->SetColorAndOpacity(FSlateColor(TxtColor));
		OutTxt->SetFont(CardFont(OutTxt, 14));
		UHorizontalBoxSlot* TxtSlot = CostRow->AddChildToHorizontalBox(OutTxt);
		TxtSlot->SetPadding(FMargin(0, 0, 6, 0));
		TxtSlot->SetVerticalAlignment(VAlign_Center);
	};

	MakeCostPair(CostFoodIcon, CostFoodText, Rok2CardStyle::ResGreen);
	MakeCostPair(CostWoodIcon, CostText, Rok2CardStyle::ResGreen);
	MakeCostPair(TimeIcon, TimeText, Rok2CardStyle::Muted);

	CostRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// شريط طابور حالي (يُملأ من الـ API)
	QueueBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	QueueBar->SetPercent(0.f);
	QueueBar->SetFillColorAndOpacity(Rok2CardStyle::Gold);
	VBox->AddChildToVerticalBox(QueueBar)->SetPadding(FMargin(30, 8, 30, 2));

	QueueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	QueueText->SetText(FText::FromString(TEXT("")));
	QueueText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Muted));
	QueueText->SetFont(CardFont(QueueText, 11));
	QueueText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(QueueText)->SetPadding(FMargin(0, 0, 0, 10));

	// صف الأزرار
	UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(BtnRow)->SetPadding(FMargin(20, 6, 20, 14));

	// P6-T1: كل زر = أيقونة إجرائية + نص
	auto MakeBtn = [&](UButton*& OutBtn, UTextBlock*& OutTxt, UImage*& OutIco, const FString& IconId, const FString& Label, FLinearColor Bg, const FName Handler) {
		UBorder* B = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrushColor(Bg);
		OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		B->SetContent(OutBtn);
		UHorizontalBox* BtnBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		OutBtn->AddChild(BtnBox);
		OutIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		OutIco->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 18.f, Rok2CardStyle::Ivory));
		OutIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
		UHorizontalBoxSlot* IcoSlot = BtnBox->AddChildToHorizontalBox(OutIco);
		IcoSlot->SetPadding(FMargin(6, 2, 4, 2));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		OutTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutTxt->SetText(FText::FromString(Label));
		OutTxt->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Ivory));
		OutTxt->SetFont(CardFont(OutTxt, 14));
		UHorizontalBoxSlot* TxtSlot = BtnBox->AddChildToHorizontalBox(OutTxt);
		TxtSlot->SetPadding(FMargin(0, 2, 6, 2));
		TxtSlot->SetVerticalAlignment(VAlign_Center);
		OutBtn->OnClicked.AddDynamic(this, Handler);
		// P6-T3: ضغطة محسوسة (تصغير + نقرة) على حاوية الزر
		URok2MotionLibrary::BindPress(OutBtn, B);
		UHorizontalBoxSlot* S = BtnRow->AddChildToHorizontalBox(B);
		S->SetPadding(FMargin(4, 0, 4, 0));
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	};

	UImage* UpgradeIco = nullptr;
	MakeBtn(UpgradeButton, UpgradeBtnText, UpgradeIco, TEXT("build"), TEXT("ترقية"), Rok2CardStyle::BtnGold, FName(TEXT("OnUpgradeClicked")));
	MakeBtn(ActionButton, ActionBtnText, ActionBtnIcon, TEXT("sword"), TEXT(""), Rok2CardStyle::BtnGhost, FName(TEXT("OnActionClicked")));

	UTextBlock* CloseTxt = nullptr;
	UImage* CloseIco = nullptr;
	UButton* CloseBtnRef = nullptr;
	MakeBtn(CloseBtnRef, CloseTxt, CloseIco, TEXT("close"), TEXT("إغلاق"), Rok2CardStyle::BtnGhost, FName(TEXT("OnCloseClicked")));
	CloseButton = CloseBtnRef;

	// P6-T3: اللوحة تنزلق من الأسفل كـ Bottom Sheet (0.25s ease-out) — المعيار الموحد
	URok2MotionLibrary::PlaySlideInBottom(SheetBorder);
}

void URok2BuildingDetailWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// P6-T3: انزلاق الدخول انتقل إلى URok2MotionLibrary (يُشغَّل مرة في NativeConstruct)
	// فلم يبق عمل لكل إطار هنا.
}

void URok2BuildingDetailWidget::OnUpgradeClicked()
{
	if (Api && !BuildingId.IsEmpty())
	{
		Api->UpgradeBuilding(BuildingId);
	}
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2BuildingDetailWidget::OnActionClicked()
{
	const FString Kind = ActionKindForBuilding(BuildingId);
	if (!Kind.IsEmpty())
	{
		OnBuildingAction.Broadcast(BuildingId, Kind);
	}
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2BuildingDetailWidget::OnCloseClicked()
{
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}
