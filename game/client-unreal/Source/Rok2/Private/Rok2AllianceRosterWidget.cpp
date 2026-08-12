// Copyright ROK2.
// P6-T1: زر المساعدة بأيقونة مصافحة إجرائية (بدل 🤝).
// P6-T3: اللوحة تفتح من المركز + ضغطة محسوسة على كل أزرار الكشف.

#include "Rok2AllianceRosterWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2AllianceRallyWidget.h"
#include "Rok2BattleReportWidget.h"
#include "Rok2Typography.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Rok2MotionLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Spacer.h"
#include "Components/Image.h"

void URok2AllianceRosterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
		// P6-T7: خلفية اللوحة بلون الحضارة
		FLinearColor PanelColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.9f);
		if (Api)
		{
			const FRok2CivTheme& Theme = URok2CivThemes::Get()->GetTheme(Api->GetPlayer().Civ);
			PanelColor = Theme.PanelBg;
		}
		MainBorder->SetBrushColor(PanelColor);
		
		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BorderSlot->SetSize(FVector2D(600.f, 800.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		MainBorder->AddChild(VBox);
		URok2MotionLibrary::PlayScaleInCenter(MainBorder);	// P6-T3: تفتح من المركز

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("Alliance Roster")));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.2f)));
		URok2Typography::ApplyFont(TitleText, ERok2TextRole::Display);
		UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText);
		TitleSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 10.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);

			UTextBlock* RallyTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RallyTitle"));
			RallyTitle->SetText(FText::FromString(TEXT("الراليات النشطة")));
			RallyTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.78f, 0.24f)));
			URok2Typography::ApplyFont(RallyTitle, ERok2TextRole::Subtitle);
			VBox->AddChildToVerticalBox(RallyTitle)->SetPadding(FMargin(20.f, 8.f, 20.f, 2.f));

			RallyVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RallyVBox"));
			UVerticalBoxSlot* RallySlot = VBox->AddChildToVerticalBox(RallyVBox);
			RallySlot->SetPadding(FMargin(20.f, 0.f, 20.f, 4.f));

			RallyReportsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RallyReportsButton"));
			RallyReportsButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.72f, 0.52f, 0.18f));
			UTextBlock* RallyReportsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RallyReportsText"));
			RallyReportsText->SetText(FText::FromString(TEXT("تقارير الراليات والقتال")));
			RallyReportsText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(RallyReportsText, ERok2TextRole::Button);
			RallyReportsButton->AddChild(RallyReportsText);
			RallyReportsButton->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnRallyReportsClicked);
			URok2MotionLibrary::BindPress(RallyReportsButton);
			VBox->AddChildToVerticalBox(RallyReportsButton)->SetPadding(FMargin(20.f, 2.f, 20.f, 8.f));

			RosterVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RosterVBox"));
			UVerticalBoxSlot* RosterSlot = VBox->AddChildToVerticalBox(RosterVBox);
			RosterSlot->SetPadding(FMargin(20.f, 10.f, 20.f, 10.f));
			RosterSlot->Size.SizeRule = ESlateSizeRule::Fill;

		// Help Button
		HelpButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HelpButton"));
		HelpButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f));
		UVerticalBoxSlot* BtnSlot = VBox->AddChildToVerticalBox(HelpButton);
		BtnSlot->SetPadding(FMargin(20.f, 10.f, 20.f, 20.f));
		BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		// P6-T1: أيقونة مصافحة إجرائية + نص
		{
			UHorizontalBox* BtnBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			HelpButton->AddChild(BtnBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("handshake"), 20.f, FLinearColor::White));
				Ico->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
				// P7-T7: نص بديل لزر مساعدة التحالف
				Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("handshake")));
				HelpButton->SetToolTipText(FText::FromString(TEXT("مساعدة التحالف")));
			UHorizontalBoxSlot* IcoSlot = BtnBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8.f, 2.f, 5.f, 2.f));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BtnText"));
			BtnText->SetText(FText::FromString(TEXT("مساعدة التحالف (Alliance Help)")));
			BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(BtnText, ERok2TextRole::Button);
			BtnBox->AddChildToHorizontalBox(BtnText)->SetVerticalAlignment(VAlign_Center);
		}

		HelpButton->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnHelpClicked);
		URok2MotionLibrary::BindPress(HelpButton);	// P6-T3: ضغطة محسوسة

		PopulateRoster();
	}

	if (Api)
	{
		Api->OnAllianceRalliesUpdated.RemoveDynamic(this, &URok2AllianceRosterWidget::OnRalliesUpdated);
		Api->OnAllianceRalliesUpdated.AddDynamic(this, &URok2AllianceRosterWidget::OnRalliesUpdated);
		PopulateRallies(Api->GetAllianceRallies());
		Api->FetchAllianceRallies();
	}
}

void URok2AllianceRosterWidget::PopulateRoster()
{
	if (!RosterVBox) return;
	RosterVBox->ClearChildren();

	for (const auto& Member : Members)
	{
		UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(FText::FromString(Member.Name));
		UHorizontalBoxSlot* NameSlot = HBox->AddChildToHorizontalBox(NameText);
		NameSlot->Size.SizeRule = ESlateSizeRule::Fill;

		UTextBlock* PowerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PowerText->SetText(FText::FromString(FString::Printf(TEXT("Power: %d"), Member.Power)));
		UHorizontalBoxSlot* PowerSlot = HBox->AddChildToHorizontalBox(PowerText);
		PowerSlot->Size.SizeRule = ESlateSizeRule::Fill;

		UTextBlock* RankText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		RankText->SetText(FText::FromString(Member.Rank));
		UHorizontalBoxSlot* RankSlot = HBox->AddChildToHorizontalBox(RankText);
		RankSlot->Size.SizeRule = ESlateSizeRule::Fill;

		UButton* PromoteBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* PromoteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PromoteText->SetText(FText::FromString(TEXT("Promote")));
		PromoteBtn->AddChild(PromoteText);
		PromoteBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnPromoteClicked);
		URok2MotionLibrary::BindPress(PromoteBtn);	// P6-T3: ضغطة محسوسة
		HBox->AddChildToHorizontalBox(PromoteBtn);

		UButton* KickBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* KickText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KickText->SetText(FText::FromString(TEXT("Kick")));
		KickBtn->AddChild(KickText);
		KickBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnKickClicked);
		URok2MotionLibrary::BindPress(KickBtn);	// P6-T3: ضغطة محسوسة
		HBox->AddChildToHorizontalBox(KickBtn);

		UVerticalBoxSlot* RowSlot = RosterVBox->AddChildToVerticalBox(HBox);
		RowSlot->SetPadding(FMargin(0, 5, 0, 5));
	}

	// Invite
	UButton* InviteBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* InviteText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	InviteText->SetText(FText::FromString(TEXT("Invite")));
	InviteBtn->AddChild(InviteText);
	InviteBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnInviteClicked);
	URok2MotionLibrary::BindPress(InviteBtn);	// P6-T3: ضغطة محسوسة
	UVerticalBoxSlot* InviteSlot = RosterVBox->AddChildToVerticalBox(InviteBtn);
	InviteSlot->SetPadding(FMargin(0, 10, 0, 0));
}

void URok2AllianceRosterWidget::PopulateRallies(const TArray<FRok2AllianceRally>& Rallies)
{
	if (!RallyVBox || !WidgetTree) return;
	RallyVBox->ClearChildren();
	if (Rallies.IsEmpty())
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyRallies"));
		EmptyText->SetText(FText::FromString(TEXT("لا توجد راليات قيد التجميع حالياً.")));
		EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.75f, 0.82f)));
		URok2Typography::ApplyFont(EmptyText, ERok2TextRole::Body);
		RallyVBox->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(2.f, 2.f, 2.f, 8.f));
		return;
	}

	for (const FRok2AllianceRally& Rally : Rallies)
	{
		URok2AllianceRallyWidget* Card = CreateWidget<URok2AllianceRallyWidget>(GetWorld(), URok2AllianceRallyWidget::StaticClass());
		if (!Card) continue;
		Card->Setup(Api, Rally);
		RallyVBox->AddChildToVerticalBox(Card)->SetPadding(FMargin(0.f, 2.f, 0.f, 4.f));
	}
}

void URok2AllianceRosterWidget::OnRalliesUpdated(const TArray<FRok2AllianceRally>& Rallies)
{
	PopulateRallies(Rallies);
}

void URok2AllianceRosterWidget::OnRallyReportsClicked()
{
	if (!Api || !GetWorld()) return;
	if (!RallyReportsWidget)
	{
		RallyReportsWidget = CreateWidget<URok2BattleReportWidget>(GetWorld(), URok2BattleReportWidget::StaticClass());
		if (RallyReportsWidget) RallyReportsWidget->Setup(Api);
	}
	if (RallyReportsWidget && !RallyReportsWidget->IsInViewport())
	{
		RallyReportsWidget->AddToViewport(55);
	}
	Api->FetchBattleReports();
}

void URok2AllianceRosterWidget::OnHelpClicked()
{
	if (Api)
	{
		Api->AllianceHelp();
	}
}

void URok2AllianceRosterWidget::OnPromoteClicked()
{
	// stub
}

void URok2AllianceRosterWidget::OnKickClicked()
{
	// stub
}

void URok2AllianceRosterWidget::OnInviteClicked()
{
	// stub
}
