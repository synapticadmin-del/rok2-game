// Copyright Rok2. Building detail popup impl.

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
#include "Blueprint/WidgetTree.h"

void URok2BuildingDetailWidget::SetupBuilding(URok2Api* InApi, const FString& InBuildingId, int32 InLevel)
{
	Api = InApi;
	BuildingId = InBuildingId;
	CurrentLevel = InLevel;

	FString DisplayName = BuildingId;
	if (BuildingId == TEXT("city_hall")) DisplayName = TEXT("🏰 القلعة الرئيسية (City Hall)");
	else if (BuildingId == TEXT("farm")) DisplayName = TEXT("🌾 المزرعة (Farm)");
	else if (BuildingId == TEXT("lumber_mill")) DisplayName = TEXT("🪵 طاحونة الخشب (Lumber Mill)");
	else if (BuildingId == TEXT("quarry")) DisplayName = TEXT("🪨 المحجر (Quarry)");
	else if (BuildingId == TEXT("goldmine")) DisplayName = TEXT("🪙 منجم الذهب (Gold Mine)");
	else if (BuildingId == TEXT("barracks")) DisplayName = TEXT("⚔️ ثكنة المشاة (Barracks)");
	else if (BuildingId == TEXT("stable")) DisplayName = TEXT("🐎 اسطبل الفرسان (Stable)");
	else if (BuildingId == TEXT("archery_range")) DisplayName = TEXT("🏹 ميدان الرماية (Archery Range)");
	else if (BuildingId == TEXT("hospital")) DisplayName = TEXT("🏥 المستشفى (Hospital)");
	else if (BuildingId == TEXT("wall")) DisplayName = TEXT("🧱 السور الدفاعي (City Wall)");
	else if (BuildingId == TEXT("storehouse")) DisplayName = TEXT("📦 المخزن (Storehouse)");

	if (TitleText) TitleText->SetText(FText::FromString(DisplayName));
	if (LevelText) LevelText->SetText(FText::FromString(FString::Printf(TEXT("المستوى الحالي: %d ➔ المستوى التالي: %d"), CurrentLevel, CurrentLevel + 1)));

	int32 FoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	int32 WoodCost = FMath::RoundToInt(200.f * FMath::Pow(1.45f, (float)CurrentLevel));
	if (CostText)
	{
		CostText->SetText(FText::FromString(FString::Printf(TEXT("التكلفة: 🍲 %d   🪵 %d"), FoodCost, WoodCost)));
	}
}

void URok2BuildingDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.03f, 0.06f, 0.12f, 0.95f));

		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(460.f, 320.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		CardBorder->SetContent(VBox);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("🏰 تفاصيل المبنى")));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 20;
		TitleText->SetFont(TitleFont);
		UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0, 15, 0, 5));

		LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
		LevelText->SetText(FText::FromString(TEXT("المستوى: 1")));
		LevelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UVerticalBoxSlot* LvlSlot = VBox->AddChildToVerticalBox(LevelText);
		LvlSlot->SetHorizontalAlignment(HAlign_Center);
		LvlSlot->SetPadding(FMargin(0, 5, 0, 10));

		CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
		CostText->SetText(FText::FromString(TEXT("التكلفة: 🍲 200   🪵 200")));
		CostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.0f, 0.6f)));
		UVerticalBoxSlot* CostSlot = VBox->AddChildToVerticalBox(CostText);
		CostSlot->SetHorizontalAlignment(HAlign_Center);
		CostSlot->SetPadding(FMargin(0, 5, 0, 20));

		UHorizontalBox* BtnHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BtnHBox"));
		VBox->AddChildToVerticalBox(BtnHBox)->SetPadding(FMargin(20, 10, 20, 15));

		UpgradeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpgradeButton"));
		UTextBlock* UpgTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UpgTxt"));
		UpgTxt->SetText(FText::FromString(TEXT("🔨 ترقية الآن")));
		UpgTxt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		UpgradeButton->AddChild(UpgTxt);
		UHorizontalBoxSlot* UpgSlot = BtnHBox->AddChildToHorizontalBox(UpgradeButton);
		UpgSlot->SetPadding(FMargin(5, 0, 5, 0));

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* ClsTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClsTxt"));
		ClsTxt->SetText(FText::FromString(TEXT("❌ إغلاق")));
		ClsTxt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CloseButton->AddChild(ClsTxt);
		UHorizontalBoxSlot* ClsSlot = BtnHBox->AddChildToHorizontalBox(CloseButton);
		ClsSlot->SetPadding(FMargin(5, 0, 5, 0));
	}

	if (UpgradeButton) UpgradeButton->OnClicked.AddDynamic(this, &URok2BuildingDetailWidget::OnUpgradeClicked);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &URok2BuildingDetailWidget::OnCloseClicked);
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
	RemoveFromParent();
}

void URok2BuildingDetailWidget::OnCloseClicked()
{
	RemoveFromParent();
}
