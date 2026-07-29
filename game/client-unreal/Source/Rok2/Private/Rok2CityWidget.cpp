// P6-T3: ضغطة محسوسة على أزرار المدينة والتسريع + تلاشي دخول الشاشة.

#include "Rok2CityWidget.h"
#include "Rok2Api.h"
#include "Rok2BattleReportWidget.h"
#include "Rok2BlueprintLibrary.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"
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
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

void URok2CityWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnCityLoaded.AddDynamic(this, &URok2CityWidget::OnCityLoaded);
	Api->OnToast.AddDynamic(this, &URok2CityWidget::OnToast);
	Api->OnConnectionState.AddDynamic(this, &URok2CityWidget::OnConnectionState);

	// P6-T3: كل زر رئيسي يحصل على ضغطة محسوسة (تصغير + نقرة)
	if (TrainButton)
	{
		TrainButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnTrainClicked);
		URok2MotionLibrary::BindPress(TrainButton);
	}
	if (CreateAllianceButton)
	{
		CreateAllianceButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnCreateAllianceClicked);
		URok2MotionLibrary::BindPress(CreateAllianceButton);
	}
	if (MapButton)
	{
		MapButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnMapClicked);
		URok2MotionLibrary::BindPress(MapButton);
	}
	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnRefreshClicked);
		URok2MotionLibrary::BindPress(RefreshButton);
	}
	if (ReportsButton)
	{
		ReportsButton->OnClicked.AddDynamic(this, &URok2CityWidget::OnReportsClicked);
		URok2MotionLibrary::BindPress(ReportsButton);
	}

	if (TrainUnitCombo)
	{
		TrainUnitCombo->ClearOptions();
		// قائمة الوحدات من بيانات الخادم الموحدة /v1/meta/all (P1-T6) — مع fallback محلي
		const FRok2GameMeta& Meta = Api->GetMeta();
		if (Meta.TrainableUnits.Num() > 0)
		{
			for (const FRok2TrainableUnit& U : Meta.TrainableUnits)
			{
				TrainUnitCombo->AddOption(FString::Printf(TEXT("%s|%s"), *U.Id, *U.Name));
			}
		}
		else
		{
			TrainUnitCombo->AddOption(TEXT("infantry_t1|مشاة T1"));
			TrainUnitCombo->AddOption(TEXT("cavalry_t1|فرسان T1"));
			TrainUnitCombo->AddOption(TEXT("archer_t1|رماة T1"));
		}
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
		URok2MotionLibrary::PlayFadeIn(RootCanvas);	// P6-T3: انتقال دخول الشاشة

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

		// Player Info — P6-T1: أيقونة حاكم إجرائية + نص
		{
			UImage* GovIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("GovIcon"));
			GovIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("governor"), 18.f, FLinearColor(1.0f, 0.84f, 0.2f)));
			GovIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
			UHorizontalBoxSlot* IcoSlot = TopHBox->AddChildToHorizontalBox(GovIco);
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetPadding(FMargin(20, 0, 4, 0));
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		PlayerInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerInfoText"));
		PlayerInfoText->SetText(FText::FromString(TEXT("Governor | Power: 0")));
		PlayerInfoText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		FSlateFontInfo InfoFont = PlayerInfoText->GetFont();
		InfoFont.Size = 15;
		PlayerInfoText->SetFont(InfoFont);
		UHorizontalBoxSlot* InfoSlot = TopHBox->AddChildToHorizontalBox(PlayerInfoText);
		InfoSlot->SetVerticalAlignment(VAlign_Center);
		InfoSlot->SetPadding(FMargin(0, 0, 30, 0));

		// Resources — P6-T1: 4 أزواج (أيقونة + رقم) بدل سطر إيموجي واحد
		{
			auto AddResPair = [&](const TCHAR* IconId, FLinearColor Color) -> UTextBlock* {
				UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 16.f, Color));
				Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
				UHorizontalBoxSlot* IcoSlot = TopHBox->AddChildToHorizontalBox(Ico);
				IcoSlot->SetVerticalAlignment(VAlign_Center);
				IcoSlot->SetPadding(FMargin(6, 0, 3, 0));
				IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				Txt->SetColorAndOpacity(FSlateColor(Color));
				Txt->SetFont(InfoFont);
				UHorizontalBoxSlot* TxtSlot = TopHBox->AddChildToHorizontalBox(Txt);
				TxtSlot->SetVerticalAlignment(VAlign_Center);
				TxtSlot->SetPadding(FMargin(0, 0, 8, 0));
				return Txt;
			};
			const FLinearColor ResColor(0.4f, 1.0f, 0.6f);
			ResFoodVal = AddResPair(TEXT("food"), ResColor);
			ResWoodVal = AddResPair(TEXT("wood"), ResColor);
			ResStoneVal = AddResPair(TEXT("stone"), ResColor);
			ResGoldVal = AddResPair(TEXT("gold"), ResColor);
			ResourcesText = ResFoodVal; // توافق خلفي — لم يعد سطراً واحداً
		}

		// Connection status badge (P1-T2) — P6-T1: أيقونة دائرة إجرائية + نص
		ConnIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ConnIcon"));
		ConnIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("conn"), 14.f, FLinearColor(0.4f, 1.0f, 0.5f)));
		ConnIcon->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		{
			UHorizontalBoxSlot* IcoSlot = TopHBox->AddChildToHorizontalBox(ConnIcon);
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetPadding(FMargin(10, 0, 4, 0));
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		ConnectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConnectionText"));
		ConnectionText->SetText(FText::FromString(TEXT("متصل")));
		ConnectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.0f, 0.5f)));
		FSlateFontInfo ConnFont = ConnectionText->GetFont();
		ConnFont.Size = 12;
		ConnectionText->SetFont(ConnFont);
		UHorizontalBoxSlot* ConnSlot = TopHBox->AddChildToHorizontalBox(ConnectionText);
		ConnSlot->SetVerticalAlignment(VAlign_Center);
		ConnSlot->SetPadding(FMargin(0, 0, 10, 0));

		// P6-T1: منشئ زر بأيقونة إجرائية + نص
		auto MakeIconBtn = [&](UButton*& OutBtn, const TCHAR* IconId, const FString& Label, FMargin IcoPad, FMargin BtnPad) {
			OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			UHorizontalBox* BtnBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			OutBtn->AddChild(BtnBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 15.f, FLinearColor::White));
			Ico->SetDesiredSizeOverride(FVector2D(15.f, 15.f));
			UHorizontalBoxSlot* IcoSlot = BtnBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(IcoPad);
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* Txt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Txt->SetText(FText::FromString(Label));
			Txt->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			BtnBox->AddChildToHorizontalBox(Txt)->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* S = TopHBox->AddChildToHorizontalBox(OutBtn);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(BtnPad);
		};

		MakeIconBtn(RefreshButton, TEXT("refresh"), TEXT("تحديث"), FMargin(5, 0, 3, 0), FMargin(10, 5, 10, 5));
		MakeIconBtn(MapButton, TEXT("map"), TEXT("الخريطة"), FMargin(5, 0, 3, 0), FMargin(10, 5, 20, 5));
		MakeIconBtn(ReportsButton, TEXT("scroll"), TEXT("التقارير"), FMargin(5, 0, 3, 0), FMargin(0, 5, 10, 5));

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

		// P6-T1: منشئ عنوان لوحة = أيقونة إجرائية + نص
		auto MakePanelTitle = [&](UVerticalBox* Parent, const TCHAR* IconId, const FString& Label, FLinearColor Color, const FMargin& Pad) {
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Parent->AddChildToVerticalBox(Row)->SetPadding(Pad);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 15.f, Color));
			Ico->SetDesiredSizeOverride(FVector2D(15.f, 15.f));
			UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			T->SetText(FText::FromString(Label));
			T->SetColorAndOpacity(FSlateColor(Color));
			Row->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		};

		MakePanelTitle(LeftVBox, TEXT("castle"), TEXT("مباني المدينة (City Buildings)"), FLinearColor(1.0f, 0.84f, 0.2f), FMargin(10, 10, 10, 5));

		UScrollBox* BldScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BldScroll"));
		LeftVBox->AddChildToVerticalBox(BldScroll)->SetPadding(FMargin(10, 0, 10, 10));

		// عنوان الطوابير (أيقونة ساعة رملية + نص)
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			BldScroll->AddChild(Row);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("hourglass"), 15.f, FLinearColor(0.2f, 0.8f, 1.0f)));
			Ico->SetDesiredSizeOverride(FVector2D(15.f, 15.f));
			UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* QueueTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QueueTitle"));
			QueueTitle->SetText(FText::FromString(TEXT("الطوابير النشطة (Active Queues)")));
			QueueTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.8f, 1.0f)));
			Row->AddChildToHorizontalBox(QueueTitle)->SetVerticalAlignment(VAlign_Center);
		}

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

		// Troop Section — P6-T1: عنوان بأيقونة سيف
		MakePanelTitle(RightVBox, TEXT("sword"), TEXT("الجيش والتدريب (Troops & Training)"), FLinearColor(1.0f, 0.84f, 0.2f), FMargin(10, 10, 10, 5));

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

		// Alliance Section — P6-T1: عنوان بأيقونة درع
		MakePanelTitle(RightVBox, TEXT("shield"), TEXT("التحالف (Alliance)"), FLinearColor(1.0f, 0.84f, 0.2f), FMargin(10, 10, 10, 5));

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
	UpdateResourceText();

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

			// P6-T1: أيقونة نوع الطابور إجرائية (بناء/تدريب) بدل الإيموجي
			const TCHAR* IconId = Q.Type == TEXT("building") ? TEXT("build") : TEXT("sword");
			UImage* QIco = NewObject<UImage>(this);
			QIco->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 14.f, FLinearColor::White));
			QIco->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
			UHorizontalBoxSlot* IcoSlot = QHBox->AddChildToHorizontalBox(QIco);
			IcoSlot->SetPadding(FMargin(0, 0, 4, 0));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			UTextBlock* QTxt = NewObject<UTextBlock>(this);
			FString QName = Q.Type == TEXT("building") ? FString::Printf(TEXT("ترقية %s"), *Q.RefId) : FString::Printf(TEXT("تدريب %s"), *Q.RefId);
			QTxt->SetText(FText::FromString(FString::Printf(TEXT("%s إلى %d"), *QName, Q.Level)));

			UHorizontalBoxSlot* TxtSlot = QHBox->AddChildToHorizontalBox(QTxt);
			TxtSlot->SetPadding(FMargin(0, 0, 10, 0));
			TxtSlot->SetVerticalAlignment(VAlign_Center);

			UButton* SpeedupBtn = NewObject<UButton>(this);
			// P6-T1: زر تسريع بأيقونة سهم مزدوج إجرائية + نص
			UHorizontalBox* SpdBox = NewObject<UHorizontalBox>(this);
			SpeedupBtn->AddChild(SpdBox);
			UImage* SpdIco = NewObject<UImage>(this);
			SpdIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("speedup"), 13.f, FLinearColor::White));
			SpdIco->SetDesiredSizeOverride(FVector2D(13.f, 13.f));
			UHorizontalBoxSlot* SpdIcoSlot = SpdBox->AddChildToHorizontalBox(SpdIco);
			SpdIcoSlot->SetPadding(FMargin(3, 0, 3, 0));
			SpdIcoSlot->SetVerticalAlignment(VAlign_Center);
			SpdIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* BtnTxt = NewObject<UTextBlock>(this);
			BtnTxt->SetText(FText::FromString(TEXT("تسريع")));
			SpdBox->AddChildToHorizontalBox(BtnTxt)->SetVerticalAlignment(VAlign_Center);

			URok2QueueBtnHandler* Handler = NewObject<URok2QueueBtnHandler>(this);
			Handler->QueueId = Q.Id;
			Handler->Api = Api;
			QueueHandlers.Add(Handler);
			SpeedupBtn->OnClicked.AddDynamic(Handler, &URok2QueueBtnHandler::OnClick);
			URok2MotionLibrary::BindPress(SpeedupBtn);	// P6-T3: ضغطة محسوسة

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

void URok2CityWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// عدّاد حي سلس — المزامنة الحقيقية تتم في URok2Api كل 30 ثانية (P1-T5)
	UpdateResourceText();
}

void URok2CityWidget::UpdateResourceText()
{
	if (!Api) return;
	const FRok2City& C = Api->GetCity();

	// accrue محلي منذ آخر مزامنة خادم
	double ElapsedSec = 0.0;
	if (C.UpdatedAt > 0)
	{
		const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
		ElapsedSec = FMath::Max(0.0, (double)(NowMs - C.UpdatedAt) / 1000.0);
	}
	const double H = ElapsedSec / 3600.0;

	const int32 Food = (int32)(C.Resources.Food + C.Rates.Food * H);
	const int32 Wood = (int32)(C.Resources.Wood + C.Rates.Wood * H);
	const int32 Stone = (int32)(C.Resources.Stone + C.Rates.Stone * H);
	const int32 Gold = (int32)(C.Resources.Gold + C.Rates.Gold * H);

	// P6-T1: كل مورد نص منفصل بجانب أيقونته الإجرائية
	if (ResFoodVal) ResFoodVal->SetText(FText::FromString(FString::Printf(TEXT("%d (+%d/س)"), Food, (int32)C.Rates.Food)));
	if (ResWoodVal) ResWoodVal->SetText(FText::FromString(FString::Printf(TEXT("%d (+%d/س)"), Wood, (int32)C.Rates.Wood)));
	if (ResStoneVal) ResStoneVal->SetText(FText::FromString(FString::Printf(TEXT("%d (+%d/س)"), Stone, (int32)C.Rates.Stone)));
	if (ResGoldVal) ResGoldVal->SetText(FText::FromString(FString::Printf(TEXT("%d (+%d/س)"), Gold, (int32)C.Rates.Gold)));
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

void URok2CityWidget::OnReportsClicked()
{
	if (!Api) return;
	URok2BattleReportWidget* W = Cast<URok2BattleReportWidget>(
		URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2BattleReportWidget::StaticClass()));
	if (W)
	{
		W->Setup(Api);
		W->AddToViewport(50);
	}
}

void URok2CityWidget::OnToast(const FString& Message)
{
	// could show a toast text block
}

void URok2CityWidget::OnConnectionState(bool bOnline, const FString& StatusMessage)
{
	// P6-T1: شارة الاتصال أيقونة إجرائية تُصبغ حسب الحالة + نص
	if (ConnIcon)
	{
		ConnIcon->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("conn"), 14.f,
			bOnline ? FLinearColor(0.4f, 1.0f, 0.5f) : FLinearColor(1.0f, 0.5f, 0.4f)));
	}
	if (!ConnectionText) return;
	if (bOnline)
	{
		ConnectionText->SetText(FText::FromString(TEXT("متصل")));
		ConnectionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 1.0f, 0.5f)));
	}
	else
	{
		ConnectionText->SetText(FText::FromString(StatusMessage));
		ConnectionText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.5f, 0.4f)));
	}
}

void URok2QueueBtnHandler::OnClick()
{
	if (Api) Api->SpeedupQueue(QueueId);
}
