// Copyright ROK2. Build menu widget (P5-T3) — implementation.

#include "Rok2BuildMenuWidget.h"
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
	Catalog = {
		{ TEXT("farm"), TEXT("🌾"), TEXT("مزرعة"), TEXT("economic") },
		{ TEXT("lumber_mill"), TEXT("🪵"), TEXT("منشرة"), TEXT("economic") },
		{ TEXT("quarry"), TEXT("🪨"), TEXT("محجر"), TEXT("economic") },
		{ TEXT("goldmine"), TEXT("🪙"), TEXT("منجم ذهب"), TEXT("economic") },
		{ TEXT("storehouse"), TEXT("📦"), TEXT("مخزن"), TEXT("economic") },
		{ TEXT("courier_station"), TEXT("📮"), TEXT("بريد"), TEXT("economic") },
		{ TEXT("shop"), TEXT("🛒"), TEXT("متجر"), TEXT("economic") },
		{ TEXT("barracks"), TEXT("⚔️"), TEXT("ثكنة"), TEXT("military") },
		{ TEXT("stable"), TEXT("🐎"), TEXT("إسطبل"), TEXT("military") },
		{ TEXT("archery_range"), TEXT("🏹"), TEXT("رماية"), TEXT("military") },
		{ TEXT("siege_workshop"), TEXT("🛠️"), TEXT("حصار"), TEXT("military") },
		{ TEXT("hospital"), TEXT("🏥"), TEXT("مستشفى"), TEXT("military") },
		{ TEXT("wall"), TEXT("🧱"), TEXT("سور"), TEXT("military") },
		{ TEXT("watchtower"), TEXT("🗼"), TEXT("برج"), TEXT("military") },
		{ TEXT("castle"), TEXT("🏯"), TEXT("قلعة"), TEXT("military") },
		{ TEXT("scout_camp"), TEXT("🏕️"), TEXT("كشافة"), TEXT("military") },
		{ TEXT("academy"), TEXT("🔬"), TEXT("أكاديمية"), TEXT("military") },
		{ TEXT("monument"), TEXT("🗿"), TEXT("نصب"), TEXT("decor") },
		{ TEXT("tavern"), TEXT("🍺"), TEXT("حانة"), TEXT("decor") },
		{ TEXT("trading_post"), TEXT("⚖️"), TEXT("تجارة"), TEXT("decor") },
		{ TEXT("alliance_center"), TEXT("🤝"), TEXT("تحالف"), TEXT("decor") },
		{ TEXT("builders_hut"), TEXT("⛺"), TEXT("بنّائون"), TEXT("decor") },
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

	// عنوان
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Title->SetText(FText::FromString(TEXT("🔨 قائمة البناء")));
	Title->SetColorAndOpacity(FSlateColor(Rok2BuildStyle::Gold));
	Title->SetFont(BuildFont(Title, 18));
	Title->SetJustification(ETextJustify::Center);
	VBox->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 10, 0, 6));

	// تبويبات الفئات
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(Tabs)->SetHorizontalAlignment(HAlign_Center);

	auto MakeTab = [&](UTextBlock*& OutTxt, const FString& Label, const FName Handler) {
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		B->OnClicked.AddDynamic(this, Handler);
		OutTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		OutTxt->SetText(FText::FromString(Label));
		OutTxt->SetFont(BuildFont(OutTxt, 14));
		B->AddChild(OutTxt);
		Tabs->AddChildToHorizontalBox(B)->SetPadding(FMargin(12, 2, 12, 2));
	};

	MakeTab(TabEconText, TEXT("🌾 اقتصاد"), FName(TEXT("OnTabEcon")));
	MakeTab(TabMilText, TEXT("⚔️ عسكري"), FName(TEXT("OnTabMil")));
	MakeTab(TabDecorText, TEXT("🎨 زخرفة"), FName(TEXT("OnTabDecor")));

	// الشبكة
	Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	VBox->AddChildToVerticalBox(Grid)->SetPadding(FMargin(16, 8, 16, 16));

	FillGrid(CurrentCategory);
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
		Proxies.Add(Proxy);

		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Btn->AddChild(V);

		UTextBlock* Ico = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Ico->SetText(FText::FromString(E.Icon));
		Ico->SetFont(BuildFont(Ico, 24));
		Ico->SetJustification(ETextJustify::Center);
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
	RemoveFromParent();
}

void URok2BuildMenuWidget::OnTabEcon() { FillGrid(TEXT("economic")); }
void URok2BuildMenuWidget::OnTabMil() { FillGrid(TEXT("military")); }
void URok2BuildMenuWidget::OnTabDecor() { FillGrid(TEXT("decor")); }

void URok2BuildMenuWidget::OnCloseClicked()
{
	RemoveFromParent();
}
