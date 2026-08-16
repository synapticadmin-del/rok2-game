// P6-T3: بطاقة الدخول تظهر بتلاشٍ + ضغطة محسوسة على أزرار الدخول والبدء.
// P6-T5: نبذة الحضارة الأدبية تظهر تحت القائمة وتتبدّل مع كل اختيار.

#include "Rok2BootWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivLore.h"
#include "Rok2MotionLibrary.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

// ألوان الهوية من ui-ux-design-system.md §1 — محلية للملف على اصطلاح
// Rok2FtueStyle/Rok2HudStyle (الألوان بقيت مسؤولية كل ودجة في P6-T2).
namespace Rok2BootLoreStyle
{
	// الألوان من Rok2Visual — كانت هنا نسخة ثالثة بنفس الأرقام تقريباً.
	static const FLinearColor PanelBg = Rok2Visual::Panel();
	static const FLinearColor Gold = Rok2Visual::GoldText();
	static const FLinearColor Ivory = Rok2Visual::Ivory();
	static const FLinearColor Muted = Rok2Visual::Muted();

	/** عرض النبذة داخل بطاقة الدخول (760px ناقص هامشَي 30) */
	static constexpr float StoryWidth = 680.f;
	static const FLinearColor ShowcaseFallback = Rok2Visual::Ink();
	static const FLinearColor ShowcaseVeil = Rok2Visual::Scrim();

	/** يحمل Texture2D مستورداً؛ يبقى التخطيط صالحاً مع لون احتياطي إن لم يُستورد المصدر بعد. */
	static UTexture2D* LoadImportedVisual(const FString& Folder, const FString& AssetName)
	{
		if (AssetName.IsEmpty()) return nullptr;
		const FString Path = FString::Printf(TEXT("/Game/Art/%s/%s.%s"), *Folder, *AssetName, *AssetName);
		return LoadObject<UTexture2D>(nullptr, *Path);
	}

	static FString JoinLoreHints(const FRok2CivLore& Lore)
	{
		return FString::Join(Lore.Hints, TEXT("\n"));
	}
}

void URok2BootWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnLoginComplete.AddDynamic(this, &URok2BootWidget::OnLoginComplete);
	Api->OnPlayerLoaded.AddDynamic(this, &URok2BootWidget::OnPlayerLoaded);
	Api->OnApiError.AddDynamic(this, &URok2BootWidget::OnApiError);
	Api->OnConnectionState.AddDynamic(this, &URok2BootWidget::OnConnectionState);
	Api->OnMetaLoaded.AddDynamic(this, &URok2BootWidget::OnMetaLoaded);

	if (EnterButton)
	{
		EnterButton->OnClicked.RemoveAll(this);
		EnterButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnEnterClicked);
		URok2MotionLibrary::BindPress(EnterButton);
	}
	if (StartButton)
	{
		StartButton->OnClicked.RemoveAll(this);
		StartButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnStartClicked);
		URok2MotionLibrary::BindPress(StartButton);
	}
	if (PreviousCivButton)
	{
		PreviousCivButton->OnClicked.RemoveAll(this);
		PreviousCivButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnPreviousCivClicked);
		URok2MotionLibrary::BindPress(PreviousCivButton);
	}
	if (NextCivButton)
	{
		NextCivButton->OnClicked.RemoveAll(this);
		NextCivButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnNextCivClicked);
		URok2MotionLibrary::BindPress(NextCivButton);
	}
	if (CivCombo)
	{
		CivCombo->OnSelectionChanged.RemoveAll(this);
		CivCombo->OnSelectionChanged.AddDynamic(this, &URok2BootWidget::OnCivSelectionChanged);
		PopulateCivCombo(TEXT("rome"));
	}

	ShowCivVisuals(TEXT("rome"));
	ShowLoreFor(TEXT("rome"));
}

TSharedRef<SWidget> URok2BootWidget::RebuildWidget()
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

void URok2BootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrush(Rok2Surface::Panel());

		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		URok2Accessibility* A11y = URok2Accessibility::Get();
		CardSlot->SetSize(FVector2D(A11y ? A11y->GetScaledPx(840.f) : 840.f, A11y ? A11y->GetScaledPx(680.f) : 680.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		CardBorder->SetContent(VBox);

		// Title
		{
			UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UVerticalBoxSlot* TitleRowSlot = VBox->AddChildToVerticalBox(TitleRow);
			TitleRowSlot->SetHorizontalAlignment(HAlign_Center);
			TitleRowSlot->SetPadding(FMargin(0, 10, 0, 4));
			UImage* CrownIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			CrownIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("crown"), 26.f, Rok2Visual::GoldText()));
			CrownIco->SetDesiredSizeOverride(FVector2D(26.f, 26.f));
			CrownIco->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("crown")));
			UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(CrownIco);
			IcoSlot->SetPadding(FMargin(0, 0, 8, 0));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
			TitleText->SetText(FText::FromString(TEXT("ROK2 : RISE OF KINGDOMS 2")));
			TitleText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
			URok2Typography::ApplyFont(TitleText, ERok2TextRole::Display);
			TitleRow->AddChildToHorizontalBox(TitleText)->SetVerticalAlignment(VAlign_Center);
		}

		// Subtitle
		UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		SubtitleText->SetText(FText::FromString(TEXT("مرحباً بك يا حاكم — اختر اسمك وحضارتك لإنشاء المملكة")));
		SubtitleText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(SubtitleText, ERok2TextRole::BodySmall);
		UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(SubtitleText);
		SubSlot->SetHorizontalAlignment(HAlign_Center);
		SubSlot->SetPadding(FMargin(0, 0, 0, 6));

		// Enter Button (Guest login)
		EnterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EnterButton"));
		EnterButton->SetStyle(Rok2Surface::PrimaryButton());
		{
			UHorizontalBox* EnterBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			EnterButton->AddChild(EnterBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("ap"), 16.f, FLinearColor::White));
			Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
			Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("ap")));
			EnterButton->SetToolTipText(FText::FromString(TEXT("دخول سريع كضيف")));
			UHorizontalBoxSlot* IcoSlot = EnterBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8, 2, 5, 2));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* EnterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnterText"));
			EnterText->SetText(FText::FromString(TEXT("دخول سريع كضيف (Quick Guest Login)")));
			EnterText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(EnterText, ERok2TextRole::Button);
			EnterBox->AddChildToHorizontalBox(EnterText)->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* EnterSlot = VBox->AddChildToVerticalBox(EnterButton);
		EnterSlot->SetPadding(FMargin(30, 4, 30, 8));

		// Name Input
		NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInput"));
		NameInput->SetHintText(FText::FromString(TEXT("اسم الحاكم (Governor Name)...")));
		NameInput->SetText(FText::FromString(TEXT("Governor")));
		UVerticalBoxSlot* NameSlot = VBox->AddChildToVerticalBox(NameInput);
		NameSlot->SetPadding(FMargin(30, 2, 30, 6));

		CivCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CivCombo"));
		UVerticalBoxSlot* CivSlot = VBox->AddChildToVerticalBox(CivCombo);
		CivSlot->SetPadding(FMargin(0.f));

		BuildCivShowcase(VBox);
		BuildLorePanel(VBox);

		// Start Journey Button
		StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		StartButton->SetStyle(Rok2Surface::PrimaryButton());
		{
			UHorizontalBox* StartBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			StartButton->AddChild(StartBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("sword"), 16.f, FLinearColor::White));
			Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
			Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("sword")));
			StartButton->SetToolTipText(FText::FromString(TEXT("ابدأ رحلة التوسع والمجد")));
			UHorizontalBoxSlot* IcoSlot = StartBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8, 2, 5, 2));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* StartText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartText"));
			StartText->SetText(FText::FromString(TEXT("ابدأ رحلة التوسع والمجد (Start Journey)")));
			StartText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(StartText, ERok2TextRole::Button);
			StartBox->AddChildToHorizontalBox(StartText)->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* StartBtnSlot = VBox->AddChildToVerticalBox(StartButton);
		StartBtnSlot->SetPadding(FMargin(30, 4, 30, 10));

		// Loading panel
		LoadingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingPanel"));
		LoadingPanel->SetBrush(Rok2Surface::Pill(Rok2Visual::Card()));
		LoadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingText"));
		LoadingText->SetColorAndOpacity(FSlateColor(Rok2Visual::InformationText()));
		URok2Typography::ApplyFont(LoadingText, ERok2TextRole::BodySmall);
		LoadingText->SetJustification(ETextJustify::Center);
		LoadingPanel->SetContent(LoadingText);
		LoadingPanel->SetPadding(FMargin(0, 8, 0, 8));
		UVerticalBoxSlot* LoadSlot = VBox->AddChildToVerticalBox(LoadingPanel);
		LoadSlot->SetPadding(FMargin(30, 2, 30, 4));

		// Status text
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		StatusText->SetColorAndOpacity(FSlateColor(Rok2Visual::DangerText()));
		URok2Typography::ApplyFont(StatusText, ERok2TextRole::Caption);
		StatusText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* StatusSlot = VBox->AddChildToVerticalBox(StatusText);
		StatusSlot->SetPadding(FMargin(30, 2, 30, 8));

		// Bind buttons immediately upon creation
		if (EnterButton)
		{
			EnterButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnEnterClicked);
			URok2MotionLibrary::BindPress(EnterButton);
		}
		if (StartButton)
		{
			StartButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnStartClicked);
			URok2MotionLibrary::BindPress(StartButton);
		}
		if (PreviousCivButton)
		{
			PreviousCivButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnPreviousCivClicked);
			URok2MotionLibrary::BindPress(PreviousCivButton);
		}
		if (NextCivButton)
		{
			NextCivButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnNextCivClicked);
			URok2MotionLibrary::BindPress(NextCivButton);
		}
		if (CivCombo)
		{
			CivCombo->OnSelectionChanged.AddDynamic(this, &URok2BootWidget::OnCivSelectionChanged);
		}

		// Initial visibility
		EnterButton->SetVisibility(ESlateVisibility::Collapsed);
		NameInput->SetVisibility(ESlateVisibility::Visible);
		CivCombo->SetVisibility(ESlateVisibility::Collapsed);
		StartButton->SetVisibility(ESlateVisibility::Visible);
		LoadingPanel->SetVisibility(ESlateVisibility::Collapsed);
		StatusText->SetText(FText::GetEmpty());
		if (CivShowcasePanel) CivShowcasePanel->SetVisibility(ESlateVisibility::Visible);
		if (LorePanel) LorePanel->SetVisibility(ESlateVisibility::Visible);

		PopulateCivCombo(TEXT("rome"));
		ShowCivVisuals(TEXT("rome"));
		ShowLoreFor(TEXT("rome"));

		URok2MotionLibrary::PlayFadeIn(CardBorder);
	}
}

void URok2BootWidget::BuildCivShowcase(UVerticalBox* VBox)
{
	if (!VBox || !WidgetTree) return;

	CivShowcasePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CivShowcasePanel"));
	CivShowcasePanel->SetBrush(Rok2Surface::Card());
	CivShowcasePanel->SetPadding(FMargin(16.f));
	VBox->AddChildToVerticalBox(CivShowcasePanel)->SetPadding(FMargin(30.f, 4.f, 30.f, 10.f));

	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CivShowcaseLayers"));
	CivShowcasePanel->SetContent(Layers);

	CivBackdropImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CivBackdropImage"));
	CivBackdropImage->SetColorAndOpacity(FLinearColor::White);
	Layers->AddChildToOverlay(CivBackdropImage);

	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CivShowcaseVeil"));
	Veil->SetBrush(Rok2Surface::Scrim());
	Layers->AddChildToOverlay(Veil);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CivShowcaseContent"));
	Layers->AddChildToOverlay(Content);

	CivEmblemImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CivEmblemImage"));
	CivEmblemImage->SetDesiredSizeOverride(FVector2D(76.f, 76.f));
	CivEmblemImage->SetColorAndOpacity(Rok2BootLoreStyle::Gold);
	UVerticalBoxSlot* EmblemSlot = Content->AddChildToVerticalBox(CivEmblemImage);
	EmblemSlot->SetHorizontalAlignment(HAlign_Center);
	EmblemSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));

	CivNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivNameText"));
	CivNameText->SetJustification(ETextJustify::Center);
	CivNameText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Gold));
	URok2Typography::ApplyFont(CivNameText, ERok2TextRole::Display);
	Content->AddChildToVerticalBox(CivNameText)->SetHorizontalAlignment(HAlign_Center);

	CivFantasyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivFantasyText"));
	CivFantasyText->SetJustification(ETextJustify::Center);
	CivFantasyText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Ivory));
	URok2Typography::ApplyFont(CivFantasyText, ERok2TextRole::BodySmall);
	Content->AddChildToVerticalBox(CivFantasyText)->SetHorizontalAlignment(HAlign_Center);

	UHorizontalBox* Details = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CivShowcaseDetails"));
	UVerticalBoxSlot* DetailsSlot = Content->AddChildToVerticalBox(Details);
	DetailsSlot->SetPadding(FMargin(12.f, 6.f, 12.f, 2.f));
	DetailsSlot->SetHorizontalAlignment(HAlign_Center);

	CivCommanderImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CivCommanderImage"));
	CivCommanderImage->SetDesiredSizeOverride(FVector2D(116.f, 116.f));
	Details->AddChildToHorizontalBox(CivCommanderImage)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CivShowcaseTextColumn"));
	Details->AddChildToHorizontalBox(TextColumn)->SetVerticalAlignment(VAlign_Center);
	CivPerksText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivPerksText"));
	CivPerksText->SetAutoWrapText(true);
	CivPerksText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Ivory));
	URok2Typography::ApplyFont(CivPerksText, ERok2TextRole::Caption);
	TextColumn->AddChildToVerticalBox(CivPerksText);
	CivUnitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivUnitText"));
	CivUnitText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Gold));
	URok2Typography::ApplyFont(CivUnitText, ERok2TextRole::Caption);
	TextColumn->AddChildToVerticalBox(CivUnitText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	UHorizontalBox* CivNav = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CivNavigation"));
	UVerticalBoxSlot* NavSlot = Content->AddChildToVerticalBox(CivNav);
	NavSlot->SetHorizontalAlignment(HAlign_Center);
	NavSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 10.f));
	PreviousCivButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PreviousCivButton"));
	PreviousCivButton->SetStyle(Rok2Surface::SecondaryButton());
	UTextBlock* PreviousLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PreviousCivLabel"));
	PreviousLabel->SetText(FText::FromString(TEXT("الحضارة السابقة")));
	URok2Typography::ApplyFont(PreviousLabel, ERok2TextRole::Button);
	PreviousCivButton->AddChild(PreviousLabel);
	CivNav->AddChildToHorizontalBox(PreviousCivButton)->SetPadding(FMargin(4.f));

	CivCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivCounterText"));
	CivCounterText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Muted));
	URok2Typography::ApplyFont(CivCounterText, ERok2TextRole::Caption);
	CivNav->AddChildToHorizontalBox(CivCounterText)->SetPadding(FMargin(12.f, 6.f));

	NextCivButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextCivButton"));
	NextCivButton->SetStyle(Rok2Surface::SecondaryButton());
	UTextBlock* NextLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextCivLabel"));
	NextLabel->SetText(FText::FromString(TEXT("الحضارة التالية")));
	URok2Typography::ApplyFont(NextLabel, ERok2TextRole::Button);
	NextCivButton->AddChild(NextLabel);
	CivNav->AddChildToHorizontalBox(NextCivButton)->SetPadding(FMargin(4.f));
}

// ---------------------------------------------------------------------------
// P6-T5: لوحة النبذة الأدبية
// ---------------------------------------------------------------------------

void URok2BootWidget::BuildLorePanel(UVerticalBox* VBox)
{
	if (!VBox || !WidgetTree) return;

	LorePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LorePanel"));
	LorePanel->SetBrush(Rok2Surface::Card());
	LorePanel->SetPadding(FMargin(12.f, 10.f, 12.f, 10.f));

	UVerticalBoxSlot* PanelSlot = VBox->AddChildToVerticalBox(LorePanel);
	PanelSlot->SetPadding(FMargin(30, 0, 30, 12));

	// SizeBox بعرض محدود: SetAutoWrapText لا يلتفّ بلا حدٍّ أفقي، وأسطر النبذة
	// العربية أطول من عرض البطاقة — بلا هذا الحدّ يخرج السطر من الشاشة.
	// (نفس العطل الذي أُصلح في بطاقة الإرشاد P6-T4.)
	USizeBox* Bounds = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LoreBounds"));
	Bounds->SetWidthOverride(Rok2BootLoreStyle::StoryWidth);
	LorePanel->SetContent(Bounds);

	UVerticalBox* Inner = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoreInner"));
	Bounds->SetContent(Inner);

	// الترويسة: «روما — نظام وجيوش ثقيلة». دور Display لأن وثيقة Typography
	// تخصّه صراحةً لـ«اسم الحضارة عند الاختيار» (ERok2TextRole::Display).
	LoreHeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoreHeading"));
	LoreHeadingText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Gold));
	URok2Typography::ApplyFont(LoreHeadingText, ERok2TextRole::Display);
	LoreHeadingText->SetAutoWrapText(true);
	Inner->AddChildToVerticalBox(LoreHeadingText)->SetPadding(FMargin(0, 0, 0, 6));

	// النبذة: أسطرها مؤلَّفة، فـSetAutoWrapText يلتفّ عند الحاجة فقط ولا يمسّ
	// فواصل الأسطر الموجودة في النصّ.
	LoreStoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoreStory"));
	LoreStoryText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Ivory));
	URok2Typography::ApplyFont(LoreStoryText, ERok2TextRole::Body);
	LoreStoryText->SetAutoWrapText(true);
	Inner->AddChildToVerticalBox(LoreStoryText)->SetPadding(FMargin(0, 0, 0, 8));

	// التحية بوزن بصري أخفّ — صوت الحضارة لا متن الحكاية
	LoreGreetingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoreGreeting"));
	LoreGreetingText->SetColorAndOpacity(FSlateColor(Rok2BootLoreStyle::Muted));
	URok2Typography::ApplyFont(LoreGreetingText, ERok2TextRole::Micro);
	LoreGreetingText->SetAutoWrapText(true);
	Inner->AddChildToVerticalBox(LoreGreetingText);
}

FString URok2BootWidget::SelectedCivId() const
{
	if (!CivCombo) return FString();
	const FString Sel = CivCombo->GetSelectedOption();
	FString Left, Right;
	// الخيار مُرمَّز «الاسم|المعرّف» — المعرّف هو ما يُرسَل للخادم
	return Sel.Split(TEXT("|"), &Left, &Right) ? Right : Sel;
}

void URok2BootWidget::PopulateCivCombo(const FString& PreferCivId)
{
	if (!CivCombo || !Api) return;

	// المعرّف المطلوب حفظه: المفضَّل إن مُرِّر، وإلا المختار حالياً
	const FString Keep = PreferCivId.IsEmpty() ? SelectedCivId() : PreferCivId;

	CivCombo->ClearOptions();
	int32 KeepIndex = INDEX_NONE;
	int32 Index = 0;
	for (const FRok2Civilization& C : Api->GetCivilizations())
	{
		// DisplayName: العربي إن وُجد — القائمة يقرأها لاعب عربي
		CivCombo->AddOption(FString::Printf(TEXT("%s|%s"), *C.DisplayName(), *C.Id));
		if (!Keep.IsEmpty() && C.Id == Keep) KeepIndex = Index;
		Index++;
	}

	if (Index > 0)
	{
		// SetSelectedIndex يبثّ OnSelectionChanged، فالنبذة تُحدَّث من هناك ولا
		// تُستدعى مرتين. وعند غياب المحفوظ نعود لأول خيار لا لفراغ.
		CivCombo->SetSelectedIndex(KeepIndex != INDEX_NONE ? KeepIndex : 0);
	}
	else
	{
		// لا حضارات: لا نبذة تُعرض ولا اختيار يُرسَل
		ShowLoreFor(FString());
		ShowCivVisuals(FString());
	}
}

void URok2BootWidget::OnCivSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	ShowCivVisuals(SelectedCivId());
	ShowLoreFor(SelectedCivId());
}

void URok2BootWidget::SelectCivIndex(int32 RequestedIndex)
{
	if (!CivCombo || !Api) return;
	const int32 Count = Api->GetCivilizations().Num();
	if (Count <= 0) return;

	const int32 WrappedIndex = ((RequestedIndex % Count) + Count) % Count;
	CivCombo->SetSelectedIndex(WrappedIndex);
}

void URok2BootWidget::OnPreviousCivClicked()
{
	SelectCivIndex(CivCombo ? CivCombo->GetSelectedIndex() - 1 : -1);
}

void URok2BootWidget::OnNextCivClicked()
{
	SelectCivIndex(CivCombo ? CivCombo->GetSelectedIndex() + 1 : 0);
}

void URok2BootWidget::ShowCivVisuals(const FString& CivId)
{
	if (!CivShowcasePanel) return;
	URok2CivLoreRegistry* Lore = URok2CivLoreRegistry::Get();
	const bool bHasLore = Lore && Lore->HasLore(CivId);
	if (!bHasLore)
	{
		CivShowcasePanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FRok2CivLore& Entry = Lore->GetLore(CivId);
	const FString AssetId = CivId.ToLower();
	const auto ApplyTexture = [](UImage* Image, UTexture2D* Texture, const FLinearColor& Fallback)
	{
		if (!Image) return;
		if (Texture)
		{
			Image->SetBrushFromTexture(Texture, true);
			Image->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			Image->SetBrush(FSlateBrush());
			Image->SetColorAndOpacity(Fallback);
		}
	};

	ApplyTexture(CivBackdropImage, Rok2BootLoreStyle::LoadImportedVisual(TEXT("CivBackgrounds"), FString::Printf(TEXT("bg_%s"), *AssetId)), Rok2BootLoreStyle::ShowcaseFallback);
	ApplyTexture(CivEmblemImage, Rok2BootLoreStyle::LoadImportedVisual(TEXT("CivIcons"), FString::Printf(TEXT("icon_%s_runtime"), *AssetId)), Rok2BootLoreStyle::Gold);
	ApplyTexture(CivCommanderImage, Rok2BootLoreStyle::LoadImportedVisual(TEXT("Commanders"), FString::Printf(TEXT("cmd_%s_starter"), *AssetId)), Rok2BootLoreStyle::Muted);

	if (CivNameText) CivNameText->SetText(FText::FromString(Entry.NameAr.IsEmpty() ? Entry.NameLatin : Entry.NameAr));
	if (CivFantasyText) CivFantasyText->SetText(FText::FromString(Entry.FantasyAr));
	if (CivPerksText) CivPerksText->SetText(FText::FromString(Rok2BootLoreStyle::JoinLoreHints(Entry)));
	if (CivUnitText)
	{
		CivUnitText->SetText(FText::FromString(Entry.SpecialUnitId.IsEmpty()
			? TEXT("")
			: FString::Printf(TEXT("الوحدة الخاصة عند T4: %s"), *Entry.SpecialUnitId)));
	}
	if (CivCounterText && Api)
	{
		const TArray<FRok2Civilization>& Civilizations = Api->GetCivilizations();
		int32 Index = 0;
		for (int32 I = 0; I < Civilizations.Num(); ++I)
		{
			if (Civilizations[I].Id == CivId) { Index = I + 1; break; }
		}
		CivCounterText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Index, Civilizations.Num())));
	}

	CivShowcasePanel->SetVisibility(ESlateVisibility::Visible);
}

void URok2BootWidget::OnMetaLoaded(bool bFromServer)
{
	// نصّ الخادم وصل بعد بناء القائمة — نعيد الملء **محافظين على الاختيار**،
	// فلاعب كان قد اختار حضارته لا يُعاد إلى روما بسبب استجابة شبكة.
	if (!CivCombo) return;
	const FString Keep = SelectedCivId();
	PopulateCivCombo(Keep);
	// الحضارة نفسها قد تحمل الآن نصّاً مختلفاً (من الخادم) — نُبطل ذاكرة العرض
	// حتى يُعاد الرسم فعلاً بدل أن يُحسَب «لا تغيير».
	if (LastLoreCivId == Keep)
	{
		LastLoreCivId.Reset();
		ShowLoreFor(Keep);
	}
}

void URok2BootWidget::ShowLoreFor(const FString& CivId)
{
	if (!LorePanel) return;

	URok2CivLoreRegistry* Lore = URok2CivLoreRegistry::Get();
	const bool bHas = Lore && Lore->HasLore(CivId);

	// معرّف بلا نبذة: تُطوى اللوحة بلا رسالة خطأ — شاشة الدخول ليست موضع
	// تشخيص، ولا نصّ بديل يُخترع مكان نصّ مفقود.
	if (!bHas)
	{
		LorePanel->SetVisibility(ESlateVisibility::Collapsed);
		LastLoreCivId.Reset();
		return;
	}

	const FRok2CivLore& L = Lore->GetLore(CivId);

	// اختيارٌ لم يتغيّر: لا إعادة رسم ولا إعادة حركة. UComboBoxString يبثّ
	// OnSelectionChanged عند إعادة الملء أيضاً، وبطاقة تُعيد الظهور بلا سبب
	// وميضٌ مزعج لا انتقال (نفس حرس LastRenderedStep في بطاقة الإرشاد).
	if (LastLoreCivId == CivId && LorePanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		return;
	}
	const bool bFirstShow = LastLoreCivId.IsEmpty();
	LastLoreCivId = CivId;

	if (LoreHeadingText)
	{
		// الفانتازي قد يغيب في بيانات ناقصة — لا نطبع فاصلةً معلّقة بعد الاسم
		const FString Heading = L.FantasyAr.IsEmpty()
			? L.NameAr
			: FString::Printf(TEXT("%s — %s"), *L.NameAr, *L.FantasyAr);
		LoreHeadingText->SetText(FText::FromString(Heading));
	}
	if (LoreStoryText)
	{
		LoreStoryText->SetText(FText::FromString(Lore->StoryText(CivId)));
	}
	if (LoreGreetingText)
	{
		LoreGreetingText->SetText(FText::FromString(L.Greeting));
	}

	LorePanel->SetVisibility(ESlateVisibility::Visible);

	// أول ظهور ينزلق، والتبديل بين حضارتين يومض ذهباً: البطاقة لم تذهب ولم
	// تعد، فإعادة الانزلاق كانت ستقول «لوحة جديدة» والحقيقة «نصّ جديد».
	if (bFirstShow)
	{
		URok2MotionLibrary::PlayFadeIn(LorePanel);
	}
	else
	{
		URok2MotionLibrary::PlayGoldFlash(LorePanel, Rok2BootLoreStyle::PanelBg);
	}
}

void URok2BootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// نقاط متحركة لمؤشر التحميل
	if (bLoadingVisible && LoadingText)
	{
		LoadingDotsTimer += InDeltaTime;
		if (LoadingDotsTimer >= 0.4f)
		{
			LoadingDotsTimer = 0.f;
			FString Current = LoadingText->GetText().ToString();
			int32 Dots = 0;
			for (int32 i = Current.Len() - 1; i >= 0 && Current[i] == TEXT('.'); --i) Dots++;
			int32 Next = (Dots % 3) + 1;
			FString DotsStr;
			for (int32 i = 0; i < Next; ++i) DotsStr += TEXT(".");
			LoadingText->SetText(FText::FromString(LoadingBaseMessage + DotsStr));
		}
	}
}

void URok2BootWidget::SetLoading(bool bShow, const FString& Message)
{
	bLoadingVisible = bShow;
	if (LoadingPanel)
	{
		LoadingPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (bShow && LoadingText)
	{
		LoadingBaseMessage = Message.IsEmpty() ? TEXT("جاري التحميل") : Message;
		LoadingText->SetText(FText::FromString(LoadingBaseMessage));
		LoadingDotsTimer = 0.f;
	}
}

void URok2BootWidget::OnEnterClicked()
{
	if (!Api) return;
	if (Api->IsLoggedIn())
	{
		if (!Api->HasPlayer())
		{
			SetLoading(true, TEXT("جاري تحميل المدينة"));
			Api->LoadCity();
		}
	}
	else
	{
		SetLoading(true, TEXT("جاري تسجيل الدخول"));
		if (StatusText) StatusText->SetText(FText::GetEmpty());
		Api->LoginAsGuest();
	}
}

void URok2BootWidget::OnLoginComplete(const FString& Token)
{
	SetLoading(false);
	if (!Api) return;
	if (!Api->HasPlayer())
	{
		// Reveal civ selection
		if (StartButton) StartButton->SetVisibility(ESlateVisibility::Visible);
		if (NameInput) NameInput->SetVisibility(ESlateVisibility::Visible);
					// القائمة باقية كمخزن اختيار فقط؛ البطاقة هي مسار اللاعب المرئي.
			if (CivCombo) CivCombo->SetVisibility(ESlateVisibility::Collapsed);
			if (CivShowcasePanel) CivShowcasePanel->SetVisibility(ESlateVisibility::Visible);
			ShowCivVisuals(SelectedCivId());
			ShowLoreFor(SelectedCivId());

	}
}

void URok2BootWidget::OnStartClicked()
{
	if (!Api) return;
	// المعرّف من المصدر الواحد لا من فكّ ترميز مكرَّر — الفكّ المحلي السابق كان
	// نسخة ثانية من نفس المنطق تنحرف عن الأولى بصمت لو تغيّر الترميز.
	FString Civ = SelectedCivId();
	if (Civ.IsEmpty())
	{
		// قائمة فارغة (لا بيانات ولا خادم): أول حضارة من السجلّ بدل معرّف
		// مكتوب هنا — فلو حُذفت روما من الملف يوماً لا يُرسَل معرّف مجهول.
		const TArray<FRok2Civilization>& Civs = Api->GetCivilizations();
		if (Civs.Num() > 0) Civ = Civs[0].Id;
	}
	if (Civ.IsEmpty())
	{
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("لا توجد حضارات متاحة — تعذّر قراءة بيانات اللعبة")));
		return;
	}
	FString Name = NameInput ? NameInput->GetText().ToString() : TEXT("");
	if (Name.IsEmpty()) Name = TEXT("Governor");
	SetLoading(true, TEXT("جاري تأسيس المملكة"));
	if (StatusText) StatusText->SetText(FText::GetEmpty());
	Api->InitCity(Civ, Name);
}

void URok2BootWidget::OnPlayerLoaded(const FRok2Player& Player)
{
	SetLoading(false);
	// hide boot widget - game UI takes over
	RemoveFromParent();
}

void URok2BootWidget::OnApiError(const FString& Message)
{
	SetLoading(false);
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
	UE_LOG(LogTemp, Warning, TEXT("Rok2 API error: %s"), *Message);
}

void URok2BootWidget::OnConnectionState(bool bOnline, const FString& StatusMessage)
{
	if (bOnline)
	{
		// اتصال مستعاد — امسح رسالة الخطأ
		if (StatusText) StatusText->SetText(FText::GetEmpty());
	}
	else
	{
		// أظهر حالة إعادة الاتصال — ابقِ مؤشر التحميل ظاهراً ليعلم اللاعب أننا نحاول
		SetLoading(true, StatusMessage);
		if (StatusText) StatusText->SetText(FText::FromString(StatusMessage));
	}
}
