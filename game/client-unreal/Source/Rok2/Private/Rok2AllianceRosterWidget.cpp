// Copyright ROK2.
// P6-T1: زر المساعدة بأيقونة مصافحة إجرائية (بدل 🤝).

#include "Rok2AllianceRosterWidget.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
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
		MainBorder->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.9f));
		
		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BorderSlot->SetSize(FVector2D(600.f, 800.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		MainBorder->AddChild(VBox);

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("Alliance Roster")));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.8f, 0.2f)));
		TitleText->Font.Size = 24;
		UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText);
		TitleSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 10.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);

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
			UHorizontalBoxSlot* IcoSlot = BtnBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8.f, 2.f, 5.f, 2.f));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BtnText"));
			BtnText->SetText(FText::FromString(TEXT("مساعدة التحالف (Alliance Help)")));
			BtnText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			BtnText->Font.Size = 18;
			BtnBox->AddChildToHorizontalBox(BtnText)->SetVerticalAlignment(VAlign_Center);
		}

		HelpButton->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnHelpClicked);

		PopulateRoster();
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
		HBox->AddChildToHorizontalBox(PromoteBtn);

		UButton* KickBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* KickText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KickText->SetText(FText::FromString(TEXT("Kick")));
		KickBtn->AddChild(KickText);
		KickBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnKickClicked);
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
	UVerticalBoxSlot* InviteSlot = RosterVBox->AddChildToVerticalBox(InviteBtn);
	InviteSlot->SetPadding(FMargin(0, 10, 0, 0));
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
