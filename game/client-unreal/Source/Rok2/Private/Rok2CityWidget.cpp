#include "Rok2CityWidget.h"
#include "Rok2Api.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"

void URok2CityWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnCityLoaded.AddDynamic(this, &URok2CityWidget::OnCityLoaded);
	Api->OnToast.AddDynamic(this, &URok2CityWidget::OnToast);

	if (TrainButton) TrainButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnTrainClicked);
	if (CreateAllianceButton) CreateAllianceButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnCreateAllianceClicked);
	if (MapButton) MapButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnMapClicked);
	if (RefreshButton) RefreshButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnRefreshClicked);

	if (TrainUnitCombo)
	{
		TrainUnitCombo->ClearOptions();
		TrainUnitCombo->AddOption(TEXT("infantry_t1|مشاة T1"));
		TrainUnitCombo->AddOption(TEXT("cavalry_t1|فرسان T1"));
		TrainUnitCombo->AddOption(TEXT("archer_t1|رماة T1"));
		TrainUnitCombo->SetSelectedIndex(0);
	}
	if (TrainCountSpin) TrainCountSpin->SetValue(50.f);

	Refresh();
}

void URok2CityWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// 1. Top Bar Background (Full width across top)
		UBorder* TopBarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopBarBorder"));
		TopBarBorder->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.12f, 0.92f));

		UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(TopBarBorder);
		TopSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		TopSlot->SetAlignment(FVector2D(0.f, 0.f));
		TopSlot->SetPosition(FVector2D(0.f, 0.f));
		TopSlot->SetSize(FVector2D(0.f, 54.f));

		UHorizontalBox* TopHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopHBox"));
		TopBarBorder->SetContent(TopHBox);

		// Player Info
		PlayerInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerInfoText"));
		PlayerInfoText->SetText(FText::FromString(TEXT("👑 Governor | Power: 0")));
		PlayerInfoText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		FSlateFontInfo InfoFont = PlayerInfoText->GetFont();
		InfoFont.Size = 15;
		PlayerInfoText->SetFont(InfoFont);
		UHorizontalBoxSlot* InfoSlot = TopHBox->AddChildToHorizontalBox(PlayerInfoText);
		InfoSlot->SetVerticalAlignment(VAlign_Center);
		InfoSlot->SetPadding(FMargin(20, 0, 30, 0));

		// Resources
		ResourcesText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResourcesText"));
		ResourcesText->SetText(FText::FromString(TEXT("🍲 0   🪵 0   🪨 0   🪙 0")));
		ResourcesText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.0f, 0.6f)));
		ResourcesText->SetFont(InfoFont);
		UHorizontalBoxSlot* ResSlot = TopHBox->AddChildToHorizontalBox(ResourcesText);
		ResSlot->SetVerticalAlignment(VAlign_Center);
		ResSlot->SetPadding(FMargin(20, 0, 20, 0));

		// Refresh Button
		RefreshButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RefreshButton"));
		UTextBlock* RefText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RefText"));
		RefText->SetText(FText::FromString(TEXT("🔄 تحديث")));
		RefText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		RefreshButton->AddChild(RefText);
		UHorizontalBoxSlot* RefSlot = TopHBox->AddChildToHorizontalBox(RefreshButton);
		RefSlot->SetVerticalAlignment(VAlign_Center);
		RefSlot->SetPadding(FMargin(10, 5, 10, 5));

		// Map Button
		MapButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MapButton"));
		UTextBlock* MapText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MapText"));
		MapText->SetText(FText::FromString(TEXT("🗺️ الخريطة")));
		MapText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		MapButton->AddChild(MapText);
		UHorizontalBoxSlot* MapSlot = TopHBox->AddChildToHorizontalBox(MapButton);
		MapSlot->SetVerticalAlignment(VAlign_Center);
		MapSlot->SetPadding(FMargin(10, 5, 20, 5));

		// 2. Bottom Left Panel (Buildings)
		UBorder* LeftPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanel"));
		LeftPanel->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.14f, 0.88f));

		UCanvasPanelSlot* LeftSlot = RootCanvas->AddChildToCanvas(LeftPanel);
		LeftSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		LeftSlot->SetAlignment(FVector2D(0.f, 1.f));
		LeftSlot->SetPosition(FVector2D(15.f, -15.f));
		LeftSlot->SetSize(FVector2D(320.f, 260.f));

		UVerticalBox* LeftVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVBox"));
		LeftPanel->SetContent(LeftVBox);

		UTextBlock* BldTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BldTitle"));
		BldTitle->SetText(FText::FromString(TEXT("🏰 مباني المدينة (City Buildings)")));
		BldTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		LeftVBox->AddChildToVerticalBox(BldTitle)->SetPadding(FMargin(10, 10, 10, 5));

		UScrollBox* BldScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BldScroll"));
		LeftVBox->AddChildToVerticalBox(BldScroll)->SetPadding(FMargin(10, 0, 10, 10));

		UTextBlock* QueueTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QueueTitle"));
		QueueTitle->SetText(FText::FromString(TEXT("⏳ الطوابير النشطة (Active Queues)")));
		QueueTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 1.0f)));
		BldScroll->AddChild(QueueTitle);

		ActiveQueuesList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActiveQueuesList"));
		BldScroll->AddChild(ActiveQueuesList);

		UTextBlock* BldDivider = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BldDivider"));
		BldDivider->SetText(FText::FromString(TEXT(" ")));
		BldScroll->AddChild(BldDivider);

		BuildingsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildingsList"));
		BldScroll->AddChild(BuildingsList);

		// 3. Bottom Right Panel (Troops & Alliance)
		UBorder* RightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanel"));
		RightPanel->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.14f, 0.88f));

		UCanvasPanelSlot* RightSlot = RootCanvas->AddChildToCanvas(RightPanel);
		RightSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		RightSlot->SetAlignment(FVector2D(1.f, 1.f));
		RightSlot->SetPosition(FVector2D(-15.f, -15.f));
		RightSlot->SetSize(FVector2D(360.f, 300.f));

		UVerticalBox* RightVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightVBox"));
		RightPanel->SetContent(RightVBox);

		// Troop Section
		UTextBlock* TrpTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TrpTitle"));
		TrpTitle->SetText(FText::FromString(TEXT("⚔️ الجيش والتدريب (Troops & Training)")));
		TrpTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		RightVBox->AddChildToVerticalBox(TrpTitle)->SetPadding(FMargin(10, 10, 10, 5));

		UScrollBox* TrpScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TrpScroll"));
		RightVBox->AddChildToVerticalBox(TrpScroll)->SetPadding(FMargin(10, 0, 10, 5));

		TroopsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TroopsList"));
		TrpScroll->AddChild(TroopsList);

		UHorizontalBox* TrainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TrainHBox"));
		RightVBox->AddChildToVerticalBox(TrainHBox)->SetPadding(FMargin(10, 5, 10, 5));

		TrainUnitCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("TrainUnitCombo"));
		TrainHBox->AddChildToHorizontalBox(TrainUnitCombo)->SetPadding(FMargin(0, 0, 5, 0));

		TrainCountSpin = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("TrainCountSpin"));
		TrainCountSpin->SetMinValue(10.f);
		TrainCountSpin->SetMaxValue(10000.f);
		TrainCountSpin->SetValue(50.f);
		TrainHBox->AddChildToHorizontalBox(TrainCountSpin)->SetPadding(FMargin(0, 0, 5, 0));

		TrainButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TrainButton"));
		UTextBlock* TrnBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TrnBtnText"));
		TrnBtnText->SetText(FText::FromString(TEXT("تدريب")));
		TrainButton->AddChild(TrnBtnText);
		TrainHBox->AddChildToHorizontalBox(TrainButton);

		// Alliance Section
		UTextBlock* AllTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AllTitle"));
		AllTitle->SetText(FText::FromString(TEXT("🛡️ التحالف (Alliance)")));
		AllTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		RightVBox->AddChildToVerticalBox(AllTitle)->SetPadding(FMargin(10, 10, 10, 5));

		UHorizontalBox* AllHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AllHBox"));
		RightVBox->AddChildToVerticalBox(AllHBox)->SetPadding(FMargin(10, 0, 10, 10));

		AllianceNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AllianceNameInput"));
		AllianceNameInput->SetHintText(FText::FromString(TEXT("اسم التحالف")));
		AllHBox->AddChildToHorizontalBox(AllianceNameInput)->SetPadding(FMargin(0, 0, 5, 0));

		AllianceTagInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AllianceTagInput"));
		AllianceTagInput->SetHintText(FText::FromString(TEXT("TAG")));
		AllHBox->AddChildToHorizontalBox(AllianceTagInput)->SetPadding(FMargin(0, 0, 5, 0));

		CreateAllianceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateAllianceButton"));
		UTextBlock* AllBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AllBtnText"));
		AllBtnText->SetText(FText::FromString(TEXT("إنشاء")));
		CreateAllianceButton->AddChild(AllBtnText);
		AllHBox->AddChildToHorizontalBox(CreateAllianceButton);
	}
}

void URok2CityWidget::Refresh()
{
	if (!Api) return;
	const FRok2Player& P = Api->GetPlayer();
	const FRok2City& C = Api->GetCity();

	if (PlayerInfoText)
	{
		PlayerInfoText->SetText(FText::FromString(FString::Printf(
			TEXT("%s · %s · قوة %d · %s"),
			*P.Name, *P.Civ, P.Power, *P.RegionId)));
	}
	if (ResourcesText)
	{
		ResourcesText->SetText(FText::FromString(FString::Printf(
			TEXT("🍲 %d   🪵 %d   🪨 %d   🪙 %d"),
			(int32)C.Resources.Food, (int32)C.Resources.Wood,
			(int32)C.Resources.Stone, (int32)C.Resources.Gold)));
	}

	if (BuildingsList)
	{
		BuildingsList->ClearChildren();
		for (const auto& KV : Api->GetBuildings())
		{
			UTextBlock* Txt = NewObject<UTextBlock>(this);
			if (Txt)
			{
				Txt->SetText(FText::FromString(FString::Printf(TEXT("%s: Lv %d"), *KV.Key, KV.Value)));
				BuildingsList->AddChildToVerticalBox(Txt);
			}
		}
	}

	if (ActiveQueuesList)
	{
		ActiveQueuesList->ClearChildren();
		QueueHandlers.Empty();

		for (const FRok2QueueEntry& Q : C.ActiveQueues)
		{
			UHorizontalBox* QHBox = NewObject<UHorizontalBox>(this);
			
			UTextBlock* QTxt = NewObject<UTextBlock>(this);
			FString QName = Q.Type == TEXT("building") ? FString::Printf(TEXT("🏗️ ترقية %s"), *Q.RefId) : FString::Printf(TEXT("⚔️ تدريب %s"), *Q.RefId);
			QTxt->SetText(FText::FromString(FString::Printf(TEXT("%s إلى %d"), *QName, Q.Level)));
			
			UHorizontalBoxSlot* TxtSlot = QHBox->AddChildToHorizontalBox(QTxt);
			TxtSlot->SetPadding(FMargin(0, 0, 10, 0));
			TxtSlot->SetVerticalAlignment(VAlign_Center);

			UButton* SpeedupBtn = NewObject<UButton>(this);
			UTextBlock* BtnTxt = NewObject<UTextBlock>(this);
			BtnTxt->SetText(FText::FromString(TEXT("⏩ تسريع")));
			SpeedupBtn->AddChild(BtnTxt);
			
			URok2QueueBtnHandler* Handler = NewObject<URok2QueueBtnHandler>(this);
			Handler->QueueId = Q.Id;
			Handler->Api = Api;
			QueueHandlers.Add(Handler);
			SpeedupBtn->OnClicked.AddDynamic(Handler, &URok2QueueBtnHandler::OnClick);
			
			QHBox->AddChildToHorizontalBox(SpeedupBtn);
			ActiveQueuesList->AddChildToVerticalBox(QHBox);
		}
	}

	if (TroopsList)
	{
		TroopsList->ClearChildren();
		for (const auto& Troop : Api->GetTroops())
		{
			UTextBlock* Txt = NewObject<UTextBlock>(this);
			if (Txt)
			{
				Txt->SetText(FText::FromString(FString::Printf(TEXT("%s: %d"), *Troop.UnitId, Troop.Count)));
				TroopsList->AddChildToVerticalBox(Txt);
			}
		}
	}
}

void URok2CityWidget::OnCityLoaded(const FRok2City& City)
{
	Refresh();
}

void URok2CityWidget::OnTrainClicked()
{
	if (!Api || !TrainUnitCombo || !TrainCountSpin) return;
	FString Sel = TrainUnitCombo->GetSelectedOption();
	FString Unit = TEXT("infantry_t1");
	FString Left, Right;
	if (Sel.Split(TEXT("|"), &Left, &Right)) Unit = Left;
	else Unit = Sel;
	int32 Count = FMath::RoundToInt(TrainCountSpin->GetValue());
	if (Count > 0) Api->Train(Unit, Count);
}

void URok2CityWidget::OnCreateAllianceClicked()
{
	if (!Api || !AllianceNameInput || !AllianceTagInput) return;
	FString Name = AllianceNameInput->GetText().ToString();
	FString Tag = AllianceTagInput->GetText().ToString();
	if (Name.Len() >= 3 && Tag.Len() >= 2)
	{
		Api->CreateAlliance(Name, Tag);
	}
}

void URok2CityWidget::OnMapClicked()
{
	// handled by blueprint (open map level or widget)
}

void URok2CityWidget::OnRefreshClicked()
{
	if (Api) Api->LoadCity();
}

void URok2CityWidget::OnToast(const FString& Message)
{
	// could show a toast text block
}

void URok2QueueBtnHandler::OnClick()
{
	if (Api) Api->SpeedupQueue(QueueId);
}
