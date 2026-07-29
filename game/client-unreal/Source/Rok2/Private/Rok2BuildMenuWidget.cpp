// Copyright ROK2. Build menu widget (P5-T3) — implementation.
// P6-T1: أيقونات المباني إجرائية من URok2ArtAssets (بدل الإيموجي).
// P6-T3: الورقة تنزلق من الأسفل + ضغطة محسوسة على التبويبات وبطاقات المباني.

#include "Rok2BuildMenuWidget.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"
#include "Rok2DelegateBind.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Blueprint/WidgetTree.h"

namespace Rok2BuildStyle
{
	static const FLinearColor SheetBg(0.10f, 0.07f, 0.04f, 0.97f);
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f);
	static const FLinearColor Ivory(0.96f, 0.91f, 0.81f);
	static const FLinearColor Muted(0.72f, 0.68f, 0.60f, 0.9f);
	static const FLinearColor CardBg(0.16f, 0.12f, 0.07f);
	static const FLinearColor TabInactive(0.55f, 0.50f, 0.42f);
}

static FSlateFontInfo BuildFont(UTextBlock* T, int32 Size)
{
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	return F;
}

void URok2BuildMenuWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
}

void URok2BuildMenuWidget::BuildCatalog()
{
	// P6-T1: الحقل الثاني أصبح معرّف أيقونة إجرائية (لا رمز إيموجي)
	Catalog = {
		{ TEXT("farm"), TEXT("wheat"), TEXT("مزرعة"), TEXT("economic") },
		{ TEXT("lumber_mill"), TEXT("wood"), TEXT("منشرة"), TEXT("economic") },
		{ TEXT("quarry"), TEXT("rock"), TEXT("محجر"), TEXT("economic") },
		{ TEXT("goldmine"), TEXT("pickaxe"), TEXT("منجم ذهب"), TEXT("economic") },
		{ TEXT("storehouse"), TEXT("box"), TEXT("مخزن"), TEXT("economic") },
		{ TEXT("courier_station"), TEXT("mail"), TEXT("بريد"), TEXT("economic") },
		{ TEXT("shop"), TEXT("cart"), TEXT("متجر"), TEXT("economic") },
		{ TEXT("barracks"), TEXT("sword"), TEXT("ثكنة"), TEXT("military") },
		{ TEXT("stable"), TEXT("horse"), TEXT("إسطبل"), TEXT("military") },
		{ TEXT("archery_range"), TEXT("bow"), TEXT("رماية"), TEXT("military") },
		{ TEXT("siege_workshop"), TEXT("wrench"), TEXT("حصار"), TEXT("military") },
		{ TEXT("hospital"), TEXT("cross"), TEXT("مستشفى"), TEXT("military") },
		{ TEXT("wall"), TEXT("bricks"), TEXT("سور"), TEXT("military") },
		{ TEXT("watchtower"), TEXT("tower"), TEXT("برج"), TEXT("military") },
		{ TEXT("castle"), TEXT("castle"), TEXT("قلعة"), TEXT("military") },
		{ TEXT("scout_camp"), TEXT("tent"), TEXT("كشافة"), TEXT("military") },
		{ TEXT("academy"), TEXT("flask"), TEXT("أكاديمية"), TEXT("military") },
		{ TEXT("monument"), TEXT("monument"), TEXT("نصب"), TEXT("decor") },
		{ TEXT("tavern"), TEXT("beer"), TEXT("حانة"), TEXT("decor") },
		{ TEXT("trading_post"), TEXT("scale"), TEXT("تجارة"), TEXT("decor") },
		{ TEXT("alliance_center"), TEXT("handshake"), TEXT("تحالف"), TEXT("decor") },
		{ TEXT("builders_hut"), TEXT("builder"), TEXT("بنّائون"), TEXT("decor") },
	};
}

void URok2BuildMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	BuildCatalog();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// خلفية معتمة للإغلاق
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Backdrop->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	Backdrop->OnClicked.AddDynamic(this, &URok2BuildMenuWidget::OnCloseClicked);

	// الورقة السفلية
	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Sheet->SetBrushColor(Rok2BuildStyle::SheetBg);
	UCanvasPanelSlot* SheetSlot = RootCanvas->AddChildToCanvas(Sheet);
	SheetSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	SheetSlot->SetAlignment(FVector2D(0.5f, 1.f));
	SheetSlot->SetSize(FVector2D(0.f, 340.f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Sheet->SetContent(VBox);

	// عنوان: أيقونة مطرقة إجرائية + نص
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(TitleRow)->SetPadding(FMargin(0, 10, 0, 6));
	TitleRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	{
		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("build"), 20.f, Rok2BuildStyle::Gold));
		Ico->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
		UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 6, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("قائمة البناء")));
	Title->SetColorAndOpacity(FSlateColor(Rok2BuildStyle::Gold));
	Title->SetFont(BuildFont(Title, 18));
	TitleRow->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);
	TitleRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// تبويبات الفئات
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(Tabs)->SetHorizontalAlignment(HAlign_Center);

	// P6-T1: كل تبويب = أيقونة إجرائية + نص
	auto MakeTab = [&](UTextBlock*& OutTxt, const FString& IconId, const FString& Label, const FName Handler) {
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Rok2BindClickByName(B, this, Handler);
		URok2MotionLibrary::BindPress(B);	// P6-T3: ضغطة محسوسة على التبويب
		UHorizontalBox* TabBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		B->AddChild(TabBox);
		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 16.f, Rok2BuildStyle::Gold));
		Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
		UHorizontalBoxSlot* IcoSlot = TabBox->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 4, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		OutTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutTxt->SetText(FText::FromString(Label));
		OutTxt->SetFont(BuildFont(OutTxt, 14));
		TabBox->AddChildToHorizontalBox(OutTxt)->SetVerticalAlignment(VAlign_Center);
		Tabs->AddChildToHorizontalBox(B)->SetPadding(FMargin(12, 2, 12, 2));
	};

	MakeTab(TabEconText, TEXT("wheat"), TEXT("اقتصاد"), FName(TEXT("OnTabEcon")));
	MakeTab(TabMilText, TEXT("sword"), TEXT("عسكري"), FName(TEXT("OnTabMil")));
	MakeTab(TabDecorText, TEXT("art"), TEXT("زخرفة"), FName(TEXT("OnTabDecor")));

	// الشبكة
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	VBox->AddChildToVerticalBox(Grid)->SetPadding(FMargin(16, 8, 16, 16));

	FillGrid(CurrentCategory);

	// P6-T3: ورقة البناء تنزلق من الأسفل كـ Bottom Sheet (المعيار الموحد)
	URok2MotionLibrary::PlaySlideInBottom(Sheet);
}

void URok2BuildMenuWidget::FillGrid(const FString& Category)
{
	if (!Grid) return;
	Grid->ClearChildren();
	Proxies.Empty();

	// تحديث ألوان التبويبات
	auto TabColor = [&](UTextBlock* T, bool bActive) {
		if (T) T->SetColorAndOpacity(FSlateColor(bActive ? Rok2BuildStyle::Gold : Rok2BuildStyle::TabInactive));
	};
	TabColor(TabEconText, Category == TEXT("economic"));
	TabColor(TabMilText, Category == TEXT("military"));
	TabColor(TabDecorText, Category == TEXT("decor"));

	int32 Col = 0, Row = 0;
	const int32 MaxCols = 6;

	for (const FBuildEntry& E : Catalog)
	{
		if (E.Cat != Category) continue;

		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Card->SetBrushColor(Rok2BuildStyle::CardBg);

		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Card->SetContent(Btn);

		// وسيط يخزن الـ id ويعيد بثّ الضغط
		URok2BuildButtonProxy* Proxy = NewObject<URok2BuildButtonProxy>(this);
		Proxy->Id = E.Id;
		Proxy->OnPick.AddDynamic(this, &URok2BuildMenuWidget::HandleBuildingPicked);
		Btn->OnClicked.AddDynamic(Proxy, &URok2BuildButtonProxy::HandleClick);
		URok2MotionLibrary::BindPress(Btn, Card);	// P6-T3: البطاقة كلها تُضغط
		Proxies.Add(Proxy);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);

		// P6-T1: أيقونة المبنى إجرائية 28px بدل الإيموجي النصي
		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(E.Icon, 28.f, Rok2BuildStyle::Ivory));
		Ico->SetDesiredSizeOverride(FVector2D(28.f, 28.f));
		V->AddChildToVerticalBox(Ico)->SetHorizontalAlignment(HAlign_Center);

		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Name->SetText(FText::FromString(E.Name));
		Name->SetColorAndOpacity(FSlateColor(Rok2BuildStyle::Ivory));
		Name->SetFont(BuildFont(Name, 10));
		Name->SetJustification(ETextJustify::Center);
		V->AddChildToVerticalBox(Name)->SetHorizontalAlignment(HAlign_Center);

		UUniformGridSlot* GS = Grid->AddChildToUniformGrid(Card, Row, Col);
		GS->SetHorizontalAlignment(HAlign_Fill);
		GS->SetVerticalAlignment(VAlign_Fill);

		Col++;
		if (Col >= MaxCols) { Col = 0; Row++; }
	}
}

void URok2BuildMenuWidget::HandleBuildingPicked(const FString& BuildingId)
{
	OnBuildMenuPick.Broadcast(BuildingId);
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2BuildMenuWidget::OnTabEcon() { FillGrid(TEXT("economic")); }
void URok2BuildMenuWidget::OnTabMil() { FillGrid(TEXT("military")); }
void URok2BuildMenuWidget::OnTabDecor() { FillGrid(TEXT("decor")); }

void URok2BuildMenuWidget::OnCloseClicked()
{
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
}
