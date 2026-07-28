// Copyright ROK2. Building card — Bottom Sheet implementation (P5-T3).

#include "Rok2BuildingDetailWidget.h"
#include "Rok2Api.h"
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
	if (BuildingId == TEXT("city_hall")) { DisplayName = TEXT("🏰 قاعة المدينة"); Desc = TEXT("قلب مملكتك — سقف كل التقدم"); }
	else if (BuildingId == TEXT("farm")) { DisplayName = TEXT("🌾 المزرعة"); Desc = TEXT("تنتج الطعام لجنودك وشعبك"); }
	else if (BuildingId == TEXT("lumber_mill")) { DisplayName = TEXT("🪵 المنشرة"); Desc = TEXT("تنتج الخشب للبناء والتدريب"); }
	else if (BuildingId == TEXT("quarry")) { DisplayName = TEXT("🪨 المحجر"); Desc = TEXT("تنتج الحجر للمباني المتقدمة"); }
	else if (BuildingId == TEXT("goldmine")) { DisplayName = TEXT("🪙 منجم الذهب"); Desc = TEXT("ينتج الذهب للتدريب والبحث"); }
	else if (BuildingId == TEXT("barracks")) { DisplayName = TEXT("⚔️ الثكنة"); Desc = TEXT("تدريب وحدات المشاة"); }
	else if (BuildingId == TEXT("stable")) { DisplayName = TEXT("🐎 الإسطبل"); Desc = TEXT("تدريب وحدات الفرسان"); }
	else if (BuildingId == TEXT("archery_range")) { DisplayName = TEXT("🏹 ميدان الرماية"); Desc = TEXT("تدريب وحدات الرماة"); }
	else if (BuildingId == TEXT("siege_workshop")) { DisplayName = TEXT("🛠️ ورشة الحصار"); Desc = TEXT("بناء أسلحة الحصار"); }
	else if (BuildingId == TEXT("hospital")) { DisplayName = TEXT("🏥 المستشفى"); Desc = TEXT("شفاء الجرحى الخطيرين بعد المعارك"); }
	else if (BuildingId == TEXT("wall")) { DisplayName = TEXT("🧱 السور"); Desc = TEXT("خط الدفاع الأول عن مدينتك"); }
	else if (BuildingId == TEXT("storehouse")) { DisplayName = TEXT("📦 المخزن"); Desc = TEXT("يحمي مواردك من النهب"); }
	else if (BuildingId == TEXT("academy")) { DisplayName = TEXT("🔬 الأكاديمية"); Desc = TEXT("بحث التقنيات الاقتصادية والعسكرية"); }
	else if (BuildingId == TEXT("tavern")) { DisplayName = TEXT("🍺 الحانة"); Desc = TEXT("استدعاء القادة وفتح الصناديق"); }
	else if (BuildingId == TEXT("castle")) { DisplayName = TEXT("🏯 القلعة"); Desc = TEXT("قيادة حملات الـ Rally الجماعية"); }
	else if (BuildingId == TEXT("watchtower")) { DisplayName = TEXT("🗼 برج المراقبة"); Desc = TEXT("كشف الهجمات والكشافة القادمة"); }
	else if (BuildingId == TEXT("scout_camp")) { DisplayName = TEXT("🏕️ معسكر الكشافة"); Desc = TEXT("إرسال كشافة لكشف ضباب الحرب"); }
	else if (BuildingId == TEXT("alliance_center")) { DisplayName = TEXT("🤝 مركز التحالف"); Desc = TEXT("مساعدات وتعزيزات الأعضاء"); }
	else if (BuildingId == TEXT("trading_post")) { DisplayName = TEXT("⚖️ مركز التجارة"); Desc = TEXT("إرسال الموارد لأعضاء التحالف"); }
	else if (BuildingId == TEXT("monument")) { DisplayName = TEXT("🗿 النصب"); Desc = TEXT("تقدّم عصر المملكة ومكافآته"); }
	else if (BuildingId == TEXT("builders_hut")) { DisplayName = TEXT("⛺ كوخ البنّائين"); Desc = TEXT("طوابير البناء الإضافية"); }
	else if (BuildingId == TEXT("courier_station")) { DisplayName = TEXT("📮 محطة البريد"); Desc = TEXT("رسائل المملكة والتحالف"); }
	else if (BuildingId == TEXT("shop")) { DisplayName = TEXT("🛒 المتجر"); Desc = TEXT("شراء العناصر والتسريعات"); }

	if (TitleText) TitleText->SetText(FText::FromString(DisplayName));
	if (DescText) DescText->SetText(FText::FromString(Desc));
	if (LevelText) LevelText->SetText(FText::FromString(FString::Printf(TEXT("المستوى %d  ➔  %d"), CurrentLevel, CurrentLevel + 1)));

	// التكلفة (placeholder — تُقرأ من data/buildings.json عند ربط الـ meta)
	const int32 FoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	const int32 WoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	const int32 TimeSec = FMath::RoundToInt(60.f * FMath::Pow(1.6f, (float)CurrentLevel));
	if (CostText) CostText->SetText(FText::FromString(FString::Printf(TEXT("🍲 %d   🪵 %d"), FoodCost, WoodCost)));
	if (TimeText) TimeText->SetText(FText::FromString(FString::Printf(TEXT("⏱️ %dث"), TimeSec)));

	// الزر الثانوي حسب النوع
	const FString ActionLabel = ActionLabelForBuilding(BuildingId);
	if (ActionBtnText) ActionBtnText->SetText(FText::FromString(ActionLabel));
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
	if (K == TEXT("train")) return TEXT("⚔️ تدريب");
	if (K == TEXT("heal")) return TEXT("🏥 شفاء");
	if (K == TEXT("research")) return TEXT("🔬 بحث");
	if (K == TEXT("chests")) return TEXT("🎁 صناديق");
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

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Gold));
	TitleText->SetFont(CardFont(TitleText, 22));
	TitleText->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0, 2, 0, 2));

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

	// صف التكلفة والوقت
	UHorizontalBox* CostRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(CostRow)->SetHorizontalAlignment(HAlign_Center);
	CostRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CostText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::ResGreen));
	CostText->SetFont(CardFont(CostText, 14));
	CostRow->AddChildToHorizontalBox(CostText)->SetPadding(FMargin(8, 0, 8, 0));

	TimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TimeText->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Muted));
	TimeText->SetFont(CardFont(TimeText, 14));
	CostRow->AddChildToHorizontalBox(TimeText)->SetPadding(FMargin(8, 0, 8, 0));

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

	auto MakeBtn = [&](UButton*& OutBtn, UTextBlock*& OutTxt, const FString& Label, FLinearColor Bg, const FName Handler) {
		UBorder* B = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrushColor(Bg);
		OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		B->SetContent(OutBtn);
		OutTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutTxt->SetText(FText::FromString(Label));
		OutTxt->SetColorAndOpacity(FSlateColor(Rok2CardStyle::Ivory));
		OutTxt->SetFont(CardFont(OutTxt, 14));
		OutTxt->SetJustification(ETextJustify::Center);
		OutBtn->AddChild(OutTxt);
		OutBtn->OnClicked.AddDynamic(this, Handler);
		UHorizontalBoxSlot* S = BtnRow->AddChildToHorizontalBox(B);
		S->SetPadding(FMargin(4, 0, 4, 0));
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	};

	MakeBtn(UpgradeButton, UpgradeBtnText, TEXT("🔨 ترقية"), Rok2CardStyle::BtnGold, FName(TEXT("OnUpgradeClicked")));
	MakeBtn(ActionButton, ActionBtnText, TEXT(""), Rok2CardStyle::BtnGhost, FName(TEXT("OnActionClicked")));

	UTextBlock* CloseTxt = nullptr;
	UButton* CloseBtnRef = nullptr;
	MakeBtn(CloseBtnRef, CloseTxt, TEXT("✕ إغلاق"), Rok2CardStyle::BtnGhost, FName(TEXT("OnCloseClicked")));
	CloseButton = CloseBtnRef;
}

void URok2BuildingDetailWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// أنيميشن انزلاق الدخول (من أسفل لفوق)
	if (!bSlidIn && SheetBorder)
	{
		SlideT += InDeltaTime / 0.25f;
		if (SlideT >= 1.f) { SlideT = 1.f; bSlidIn = true; }
		const float Ease = 1.f - FMath::Pow(1.f - SlideT, 3.f);
		SheetBorder->SetRenderTranslation(FVector2D(0.f, (1.f - Ease) * 300.f));
	}
}

void URok2BuildingDetailWidget::OnUpgradeClicked()
{
	if (Api && !BuildingId.IsEmpty())
	{
		Api->UpgradeBuilding(BuildingId);
	}
	RemoveFromParent();
}

void URok2BuildingDetailWidget::OnActionClicked()
{
	const FString Kind = ActionKindForBuilding(BuildingId);
	if (!Kind.IsEmpty())
	{
		OnBuildingAction.Broadcast(BuildingId, Kind);
	}
	RemoveFromParent();
}

void URok2BuildingDetailWidget::OnCloseClicked()
{
	RemoveFromParent();
}
