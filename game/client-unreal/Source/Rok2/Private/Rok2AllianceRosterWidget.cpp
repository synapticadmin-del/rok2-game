// Copyright ROK2.
// P6-T1: زر المساعدة بأيقونة مصافحة إجرائية (بدل 🤝).
// P6-T3: اللوحة تفتح من المركز + ضغطة محسوسة على كل أزرار الكشف.

#include "Rok2AllianceRosterWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2AllianceRallyWidget.h"
#include "Rok2BattleReportWidget.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
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
#include "Components/EditableTextBox.h"
#include "Components/Spacer.h"
#include "Components/Image.h"


TSharedRef<SWidget> URok2AllianceRosterWidget::RebuildWidget()
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

void URok2AllianceRosterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	if (!WidgetTree->RootWidget)
	{
		URok2Accessibility* A11y = URok2Accessibility::Get();
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
		// اللوح من مصنع الأسطح المشترك (زوايا + حافة ذهبية)، والحافة تحمل لون
		// الحضارة بدل صبغ الخلفية كلها — فيبقى النص مقروءاً على كل الحضارات.
		if (Api)
		{
			MainBorder->SetBrush(Rok2Surface::AccentCard(Rok2Visual::CivilizationAccent(Api->GetPlayer().Civ)));
		}
		else
		{
			MainBorder->SetBrush(Rok2Surface::Panel());
		}
		
		UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
		BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BorderSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(480.f) : 480.f, A11y ? A11y->GetScaledPx(540.f) : 540.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		MainBorder->AddChild(VBox);
		URok2MotionLibrary::PlayScaleInCenter(MainBorder);

		// صف الرأس مع زر الإغلاق
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		VBox->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(16.f, 12.f, 16.f, 6.f));

		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("سجل التحالف (Alliance)")));
		TitleText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(TitleText, ERok2TextRole::TitleCompact);
		HeaderRow->AddChildToHorizontalBox(TitleText)->SetVerticalAlignment(VAlign_Center);

		USpacer* Sp = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		HeaderRow->AddChildToHorizontalBox(Sp)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseBtn"));
		CloseBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::RemoveFromParent);
		UTextBlock* XTxt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		XTxt->SetText(FText::FromString(TEXT("✕")));
		XTxt->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(XTxt, ERok2TextRole::Caption);
		CloseBtn->AddChild(XTxt);
		HeaderRow->AddChildToHorizontalBox(CloseBtn)->SetVerticalAlignment(VAlign_Center);

		UTextBlock* RallyTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RallyTitle"));
		RallyTitle->SetText(FText::FromString(TEXT("الراليات النشطة")));
		RallyTitle->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(RallyTitle, ERok2TextRole::Subtitle);
		VBox->AddChildToVerticalBox(RallyTitle)->SetPadding(FMargin(20.f, 8.f, 20.f, 2.f));

		// ── قسم إنشاء التحالف (P24-T1) ──
		// انتقل من لوح `URok2CityWidget` المطوي حيث لم يكن يُرى. يُظهره
		// RefreshMembershipState للاعب بلا تحالف فقط.
		{
			CreateBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreateBox"));
			VBox->AddChildToVerticalBox(CreateBox)->SetPadding(FMargin(20.f, 4.f, 20.f, 8.f));

			CreateHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreateHintText"));
			CreateHintText->SetText(FText::FromString(TEXT("لا تحالف لك بعد — أنشئ واحداً وابدأ بجمع الحكّام.")));
			CreateHintText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
			CreateHintText->SetAutoWrapText(true);
			URok2Typography::ApplyFont(CreateHintText, ERok2TextRole::BodySmall);
			CreateBox->AddChildToVerticalBox(CreateHintText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

			UHorizontalBox* CreateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreateRow"));
			CreateBox->AddChildToVerticalBox(CreateRow);

			AllianceNameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AllianceNameInput"));
			AllianceNameInput->SetHintText(FText::FromString(TEXT("اسم التحالف")));
			AllianceNameInput->WidgetStyle.SetBackgroundImageNormal(Rok2Surface::Card());
			UHorizontalBoxSlot* NameSlot = CreateRow->AddChildToHorizontalBox(AllianceNameInput);
			NameSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

			AllianceTagInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("AllianceTagInput"));
			AllianceTagInput->SetHintText(FText::FromString(TEXT("TAG")));
			AllianceTagInput->WidgetStyle.SetBackgroundImageNormal(Rok2Surface::Card());
			CreateRow->AddChildToHorizontalBox(AllianceTagInput)->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));

			CreateAllianceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreateAllianceButton"));
			CreateAllianceButton->SetStyle(Rok2Surface::PrimaryButton());
			CreateAllianceButton->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnCreateAllianceClicked);
			URok2MotionLibrary::BindPress(CreateAllianceButton);
			UTextBlock* CreateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			CreateText->SetText(FText::FromString(TEXT("إنشاء")));
			CreateText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
			URok2Typography::ApplyFont(CreateText, ERok2TextRole::Button);
			CreateAllianceButton->AddChild(CreateText);
			CreateRow->AddChildToHorizontalBox(CreateAllianceButton)->SetVerticalAlignment(VAlign_Center);
		}

		RallyVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RallyVBox"));
		UVerticalBoxSlot* RallySlot = VBox->AddChildToVerticalBox(RallyVBox);
		RallySlot->SetPadding(FMargin(20.f, 0.f, 20.f, 4.f));

		RallyReportsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RallyReportsButton"));
		RallyReportsButton->SetStyle(Rok2Surface::PrimaryButton());
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
		HelpButton->SetStyle(Rok2Surface::SuccessButton());
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
		URok2MotionLibrary::BindPress(HelpButton);

		PopulateRoster();
	}

	if (Api)
	{
		Api->OnAllianceRalliesUpdated.RemoveDynamic(this, &URok2AllianceRosterWidget::OnRalliesUpdated);
		Api->OnAllianceRalliesUpdated.AddDynamic(this, &URok2AllianceRosterWidget::OnRalliesUpdated);
		// حالة العضوية تتغيّر بعد الإنشاء أو الانضمام، والخادم هو من يقرّها —
		// فنستمع للملف بدل إخفاء القسم تخميناً.
		Api->OnPlayerLoaded.RemoveDynamic(this, &URok2AllianceRosterWidget::OnPlayerUpdated);
		Api->OnPlayerLoaded.AddDynamic(this, &URok2AllianceRosterWidget::OnPlayerUpdated);
		PopulateRallies(Api->GetAllianceRallies());
		Api->FetchAllianceRallies();
	}
	RefreshMembershipState();
}

void URok2AllianceRosterWidget::RefreshMembershipState()
{
	if (!CreateBox) return;

	const bool bHasAlliance = Api && Api->HasPlayer() && !Api->GetPlayer().AllianceId.IsEmpty();
	CreateBox->SetVisibility(bHasAlliance ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void URok2AllianceRosterWidget::OnPlayerUpdated(const FRok2Player& Player)
{
	RefreshMembershipState();
}

void URok2AllianceRosterWidget::OnCreateAllianceClicked()
{
	if (!Api || !AllianceNameInput || !AllianceTagInput) return;

	const FString Name = AllianceNameInput->GetText().ToString().TrimStartAndEnd();
	const FString Tag = AllianceTagInput->GetText().ToString().TrimStartAndEnd();

	// حدود الطول من عقد الخادم؛ نُبلّغ اللاعب بدل إرسال طلب سيُرفض بصمت.
	if (Name.Len() < 3)
	{
		Api->EmitToast(TEXT("اسم التحالف: ثلاثة أحرف على الأقل"));
		return;
	}
	if (Tag.Len() < 2)
	{
		Api->EmitToast(TEXT("وسم التحالف: حرفان على الأقل"));
		return;
	}

	Api->CreateAlliance(Name, Tag);
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
		URok2MotionLibrary::BindPress(PromoteBtn);
		HBox->AddChildToHorizontalBox(PromoteBtn);

		UButton* KickBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* KickText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KickText->SetText(FText::FromString(TEXT("Kick")));
		KickBtn->AddChild(KickText);
		KickBtn->OnClicked.AddDynamic(this, &URok2AllianceRosterWidget::OnKickClicked);
		URok2MotionLibrary::BindPress(KickBtn);
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
	URok2MotionLibrary::BindPress(InviteBtn);
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
		EmptyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
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
}

void URok2AllianceRosterWidget::OnKickClicked()
{
}

void URok2AllianceRosterWidget::OnInviteClicked()
{
}
