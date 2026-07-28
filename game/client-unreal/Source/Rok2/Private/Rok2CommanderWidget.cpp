// Copyright ROK2. Commander screen widget (P5-T4) — implementation.

#include "Rok2CommanderWidget.h"
#include "Rok2Api.h"
#include "Rok2CivThemes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/Border.h"
#include "Components/Spacer.h"
#include "Styling/CoreStyle.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Texture2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Cmdr, Log, All);

// ---------------------------------------------------------------------------
// P4-T2: تحميل بورتريه قائد حقيقي من /Game/Art/Commanders/<id> (مستورد من PNG)
// يعيد nullptr إن لم يُستورد — فيبقى الـ placeholder الملوّن (لا يُكسر العرض).
// ---------------------------------------------------------------------------
static UTexture2D* LoadCommanderPortrait(const FString& CommanderId)
{
	if (CommanderId.IsEmpty()) return nullptr;
	const FString AssetPath = FString::Printf(TEXT("/Game/Art/Commanders/%s.%s"), *CommanderId, *CommanderId);
	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath);
	if (!Tex)
	{
		UE_LOG(LogRok2Cmdr, Verbose, TEXT("Portrait not imported (placeholder stays): %s"), *AssetPath);
	}
	return Tex;
}

/** يبني UImage من بورتريه حقيقي مع إطار ذهبي، أو يرجع placeholder عند غيابه. */
static UImage* MakePortraitImage(UWidgetTree* Tree, UTexture2D* Tex, float Size)
{
	UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	Img->SetBrushFromTexture(Tex);
	Img->SetDesiredSizeOverride(FVector2D(Size, Size));
	return Img;
}

// ---------------------------------------------------------------------------
// ألوان الندرة (RoK)
// ---------------------------------------------------------------------------
static const FLinearColor COLOR_ADVANCED(0.2f, 0.7f, 0.3f);    // أخضر
static const FLinearColor COLOR_ELITE(0.2f, 0.5f, 0.9f);       // أزرق
static const FLinearColor COLOR_EPIC(0.6f, 0.3f, 0.8f);        // بنفسجي
static const FLinearColor COLOR_LEGENDARY(0.95f, 0.6f, 0.1f);  // برتقالي
static const FLinearColor COLOR_GOLD(0.79f, 0.63f, 0.15f);     // ذهبي
static const FLinearColor COLOR_IVORY(0.96f, 0.91f, 0.82f);    // عاجي
static const FLinearColor COLOR_BRONZE_BG(0.1f, 0.07f, 0.04f); // برونز داكن

// ---------------------------------------------------------------------------
// NativeConstruct — يبني الهيكل الأساسي للشاشة
// ---------------------------------------------------------------------------
void URok2CommanderWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree->RootWidget)
	{
		BuildUI();
	}

	LoadCommanderDetailsFromJson();
}

// ---------------------------------------------------------------------------
// BuildUI — يبني الشاشة بالكود (بدون Blueprint)
// ---------------------------------------------------------------------------
void URok2CommanderWidget::BuildUI()
{
	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel"));
	WidgetTree->RootWidget = RootPanel;

	// خلفية معتمة خفيفة (تسمح برؤية المدينة خلفها)
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	// اللوحة الرئيسية (Bottom Sheet كبيرة)
	UBorder* MainSheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainSheet"));
	MainSheet->SetBrushColor(FLinearColor(COLOR_BRONZE_BG.R, COLOR_BRONZE_BG.G, COLOR_BRONZE_BG.B, 0.92f));
	UCanvasPanelSlot* SheetSlot = RootPanel->AddChildToCanvas(MainSheet);
	SheetSlot->SetAnchors(FAnchors(0.05f, 0.08f, 0.95f, 0.95f));
	SheetSlot->SetOffsets(FMargin(0.f));

	// إطار ذهبي
	MainSheet->SetPadding(FMargin(2.f));
	UBorder* GoldFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GoldFrame"));
	GoldFrame->SetBrushColor(FLinearColor(COLOR_GOLD.R, COLOR_GOLD.G, COLOR_GOLD.B, 0.6f));
	MainSheet->SetContent(GoldFrame);
	GoldFrame->SetPadding(FMargin(3.f));

	// المحتوى الداخلي
	UBorder* InnerBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InnerBg"));
	InnerBg->SetBrushColor(FLinearColor(COLOR_BRONZE_BG.R, COLOR_BRONZE_BG.G, COLOR_BRONZE_BG.B, 0.95f));
	GoldFrame->SetContent(InnerBg);

	// تقسيم أفقي: قائمة القادة (يمين) | تفاصيل القائد (يسار)
	UHorizontalBox* MainHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainHBox"));
	InnerBg->SetContent(MainHBox);
	InnerBg->SetPadding(FMargin(12.f));

	// ========================================================================
	// الجانب الأيمن (RTL): قائمة القادة (40%)
	// ========================================================================
	UVerticalBox* ListPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListPanel"));
	UHorizontalBoxSlot* ListPanelSlot = MainHBox->AddChildToHorizontalBox(ListPanel);
	ListPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ListPanelSlot->SizeParam = 0.4f;

	// عنوان القائمة
	UTextBlock* ListTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ListTitle"));
	ListTitle->SetText(FText::FromString(TEXT("القادة")));
	ListTitle->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 24);
	ListTitle->SetFont(TitleFont);
	ListTitle->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* ListTitleSlot = ListPanel->AddChildToVerticalBox(ListTitle);
	ListTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	// ScrollBox للقائمة
	CommanderListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CommanderListScroll"));
	UVerticalBoxSlot* ScrollSlot = ListPanel->AddChildToVerticalBox(CommanderListScroll);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	CommanderListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CommanderListBox"));
	CommanderListScroll->AddChild(CommanderListBox);

	// فاصل
	USpacer* VSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("VSpacer"));
	VSpacer->SetSize(FVector2D(8.f, 0.f));
	MainHBox->AddChildToHorizontalBox(VSpacer);

	// ========================================================================
	// الجانب الأيسر (RTL): تفاصيل القائد (60%)
	// ========================================================================
	UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DetailScroll"));
	UHorizontalBoxSlot* DetailScrollSlot = MainHBox->AddChildToHorizontalBox(DetailScroll);
	DetailScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DetailScrollSlot->SizeParam = 0.6f;

	DetailPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailPanel"));
	DetailScroll->AddChild(DetailPanel);

	// --- ترويسة التفاصيل: بورتريه + اسم + ندرة + مستوى ---
	UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderBox"));
	UVerticalBoxSlot* HeaderSlot = DetailPanel->AddChildToVerticalBox(HeaderBox);
	HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// بورتريه كبير (placeholder)
	DetailPortraitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DetailPortrait"));
	DetailPortraitImage->SetDesiredSizeOverride(FVector2D(120.f, 120.f));
	UHorizontalBoxSlot* PortraitSlot = HeaderBox->AddChildToHorizontalBox(DetailPortraitImage);
	PortraitSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	PortraitSlot->SetVerticalAlignment(VAlign_Top);

	// معلومات أساسية (عمودي)
	UVerticalBox* InfoBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoBox"));
	HeaderBox->AddChildToHorizontalBox(InfoBox);

	DetailNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailName"));
	DetailNameText->SetText(FText::FromString(TEXT("—")));
	DetailNameText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	DetailNameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 28));
	InfoBox->AddChildToVerticalBox(DetailNameText);

	DetailRarityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailRarity"));
	DetailRarityText->SetText(FText::FromString(TEXT("")));
	DetailRarityText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 16));
	InfoBox->AddChildToVerticalBox(DetailRarityText);

	DetailNationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailNation"));
	DetailNationText->SetText(FText::FromString(TEXT("")));
	DetailNationText->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	DetailNationText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	InfoBox->AddChildToVerticalBox(DetailNationText);

	DetailLevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailLevel"));
	DetailLevelText->SetText(FText::FromString(TEXT("")));
	DetailLevelText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	DetailLevelText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 18));
	InfoBox->AddChildToVerticalBox(DetailLevelText);

	DetailStarsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailStars"));
	DetailStarsText->SetText(FText::FromString(TEXT("")));
	DetailStarsText->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	DetailStarsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
	InfoBox->AddChildToVerticalBox(DetailStarsText);

	// شريط الخبرة
	DetailXpBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("DetailXpBar"));
	DetailXpBar->SetPercent(0.3f);
	DetailXpBar->SetFillColorAndOpacity(COLOR_GOLD);
	DetailXpBar->SetWidgetStyle(FProgressBarStyle()
		.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f)))
		.SetFillImage(FSlateColorBrush(COLOR_GOLD))
	);
	UVerticalBoxSlot* XpSlot = InfoBox->AddChildToVerticalBox(DetailXpBar);
	XpSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	// --- إحصائيات ---
	DetailStatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailStats"));
	DetailStatsText->SetText(FText::FromString(TEXT("")));
	DetailStatsText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	DetailStatsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	UVerticalBoxSlot* StatsSlot = DetailPanel->AddChildToVerticalBox(DetailStatsText);
	StatsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	// --- المهارات ---
	UTextBlock* SkillsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillsTitle"));
	SkillsTitle->SetText(FText::FromString(TEXT("المهارات")));
	SkillsTitle->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	SkillsTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	UVerticalBoxSlot* SkillsTitleSlot = DetailPanel->AddChildToVerticalBox(SkillsTitle);
	SkillsTitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	SkillsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SkillsBox"));
	DetailPanel->AddChildToVerticalBox(SkillsBox);

	// --- المواهب ---
	UTextBlock* TalentsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TalentsTitle"));
	TalentsTitle->SetText(FText::FromString(TEXT("المواهب")));
	TalentsTitle->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	TalentsTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	UVerticalBoxSlot* TalentsTitleSlot = DetailPanel->AddChildToVerticalBox(TalentsTitle);
	TalentsTitleSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));

	TalentsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TalentsBox"));
	DetailPanel->AddChildToVerticalBox(TalentsBox);

	// --- المعدات ---
	UTextBlock* EquipTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EquipTitle"));
	EquipTitle->SetText(FText::FromString(TEXT("المعدات")));
	EquipTitle->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	EquipTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 20));
	UVerticalBoxSlot* EquipTitleSlot = DetailPanel->AddChildToVerticalBox(EquipTitle);
	EquipTitleSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));

	EquipmentBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EquipmentBox"));
	DetailPanel->AddChildToVerticalBox(EquipmentBox);

	// --- أزرار الإجراءات ---
	UHorizontalBox* ActionBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionBox"));
	UVerticalBoxSlot* ActionSlot = DetailPanel->AddChildToVerticalBox(ActionBox);
	ActionSlot->SetPadding(FMargin(0.f, 20.f, 0.f, 0.f));
	ActionSlot->SetHorizontalAlignment(HAlign_Center);

	// زر تعيين في مسيرة
	UButton* AssignBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AssignBtn"));
	UTextBlock* AssignText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AssignText"));
	AssignText->SetText(FText::FromString(TEXT("⚔️ تعيين في مسيرة")));
	AssignText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	AssignText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	AssignBtn->AddChild(AssignText);
	AssignBtn->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnAssignClicked);
	UHorizontalBoxSlot* AssignSlot = ActionBox->AddChildToHorizontalBox(AssignBtn);
	AssignSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	// زر ترقية المستوى
	UButton* LevelUpBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LevelUpBtn"));
	UTextBlock* LevelUpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelUpText"));
	LevelUpText->SetText(FText::FromString(TEXT("⭐ ترقية المستوى")));
	LevelUpText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	LevelUpText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	LevelUpBtn->AddChild(LevelUpText);
	LevelUpBtn->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnLevelUpClicked);
	UHorizontalBoxSlot* LevelUpSlot = ActionBox->AddChildToHorizontalBox(LevelUpBtn);
	LevelUpSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	// زر ترقية المهارة
	UButton* SkillUpBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillUpBtn"));
	UTextBlock* SkillUpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillUpText"));
	SkillUpText->SetText(FText::FromString(TEXT("⚡ ترقية مهارة")));
	SkillUpText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	SkillUpText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	SkillUpBtn->AddChild(SkillUpText);
	SkillUpBtn->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnSkillUpgradeClicked);
	ActionBox->AddChildToHorizontalBox(SkillUpBtn);
}

// ---------------------------------------------------------------------------
// SetupWithApi — يربط الشاشة بالـ API
// ---------------------------------------------------------------------------
void URok2CommanderWidget::SetupWithApi(URok2Api* InApi)
{
	Api = InApi;
	RefreshCommanderList();
}

// ---------------------------------------------------------------------------
// RefreshCommanderList — يملأ قائمة القادة من Api->GetCommanders()
// ---------------------------------------------------------------------------
void URok2CommanderWidget::RefreshCommanderList()
{
	if (!Api || !CommanderListBox) return;

	CommanderListBox->ClearChildren();

	const TArray<FRok2Commander>& Commanders = Api->GetCommanders();
	if (Commanders.Num() == 0)
	{
		UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
		EmptyText->SetText(FText::FromString(TEXT("لا يوجد قادة بعد — استدعِ من الحانة")));
		EmptyText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
		EmptyText->SetJustification(ETextJustify::Center);
		CommanderListBox->AddChildToVerticalBox(EmptyText);
		return;
	}

	for (const FRok2Commander& Cmd : Commanders)
	{
		UWidget* Card = BuildCommanderCard(Cmd);
		if (Card)
		{
			UVerticalBoxSlot* CardSlot = CommanderListBox->AddChildToVerticalBox(Card);
			CardSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}
	}
}

// ---------------------------------------------------------------------------
// BuildCommanderCard — بطاقة قائد في القائمة
// ---------------------------------------------------------------------------
UWidget* URok2CommanderWidget::BuildCommanderCard(const FRok2Commander& Cmd)
{
	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CardBorder->SetBrushColor(FLinearColor(0.15f, 0.12f, 0.08f, 0.8f));
	CardBorder->SetPadding(FMargin(8.f));

	UHorizontalBox* CardBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CardBorder->SetContent(CardBox);

	// بورتريه: حقيقي من /Game/Art/Commanders/<id> إن استُورد، وإلا placeholder ملوّن
	UWidget* Portrait = nullptr;
	if (UTexture2D* Tex = LoadCommanderPortrait(Cmd.Id))
	{
		Portrait = MakePortraitImage(WidgetTree, Tex, 56.f);
	}
	else
	{
		Portrait = BuildPortraitPlaceholder(Cmd.Name, Cmd.Nation, 56.f);
	}
	CardBox->AddChildToHorizontalBox(Portrait);

	// معلومات (عمودي)
	UVerticalBox* InfoBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* InfoSlot = CardBox->AddChildToHorizontalBox(InfoBox);
	InfoSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	InfoSlot->SetVerticalAlignment(VAlign_Center);

	// اسم
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(Cmd.Name));
	NameText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	InfoBox->AddChildToVerticalBox(NameText);

	// ندرة + حضارة
	FString RarityNation = FString::Printf(TEXT("%s · %s"), *Cmd.Rarity, *Cmd.Nation);
	UTextBlock* RarityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RarityText->SetText(FText::FromString(RarityNation));
	RarityText->SetColorAndOpacity(FSlateColor(RarityColor(Cmd.Rarity)));
	RarityText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	InfoBox->AddChildToVerticalBox(RarityText);

	// نجوم
	int32 Stars = StarsForRarity(Cmd.Rarity);
	FString StarsStr;
	for (int32 i = 0; i < 6; ++i)
	{
		StarsStr += (i < Stars) ? TEXT("★") : TEXT("☆");
	}
	UTextBlock* StarsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StarsText->SetText(FText::FromString(StarsStr));
	StarsText->SetColorAndOpacity(FSlateColor(COLOR_GOLD));
	StarsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
	InfoBox->AddChildToVerticalBox(StarsText);

	// زر البطاقة (يغطيها كلها)
	UButton* CardBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CardBtn->SetVisibility(ESlateVisibility::Visible);
	CardBtn->OnClicked.AddDynamic(this, &URok2CommanderWidget::OnCardClicked);
	// ملاحظة: في UE5 حقيقي، نحتاج لتمرير معرف القائد مع الزر — نستخدم Tag أو WidgetTree

	return CardBorder;
}

// ---------------------------------------------------------------------------
// BuildPortraitPlaceholder — مربع ملوّن بحرف الاسم الأول بلون الحضارة
// ---------------------------------------------------------------------------
UWidget* URok2CommanderWidget::BuildPortraitPlaceholder(const FString& CommanderName, const FString& Nation, float Size)
{
	// لون الحضارة من URok2CivThemes
	FLinearColor CivColor = FLinearColor(0.5f, 0.5f, 0.5f);
	if (URok2CivThemes* Themes = URok2CivThemes::Get())
	{
		const FRok2CivTheme& Theme = Themes->GetTheme(Nation);
		CivColor = Theme.Primary;
	}

	UBorder* PortraitBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	PortraitBorder->SetBrushColor(CivColor);
	PortraitBorder->SetPadding(FMargin(2.f));

	// إطار ذهبي داخلي
	UBorder* GoldInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	GoldInner->SetBrushColor(FLinearColor(COLOR_GOLD.R, COLOR_GOLD.G, COLOR_GOLD.B, 0.4f));
	PortraitBorder->SetContent(GoldInner);
	GoldInner->SetPadding(FMargin(2.f));

	// خلفية داكنة للنص
	UBorder* DarkBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	DarkBg->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));
	GoldInner->SetContent(DarkBg);
	DarkBg->SetPadding(FMargin(0.f));

	// حرف الاسم الأول
	FString Initial = CommanderName.Len() > 0 ? CommanderName.Left(1) : TEXT("?");
	UTextBlock* InitialText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	InitialText->SetText(FText::FromString(Initial));
	InitialText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	InitialText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(Size * 0.5f)));
	InitialText->SetJustification(ETextJustify::Center);
	DarkBg->SetContent(InitialText);

	// ضبط الحجم
	PortraitBorder->SetDesiredSizeOverride(FVector2D(Size, Size));

	return PortraitBorder;
}

// ---------------------------------------------------------------------------
// SelectCommander — يختار قائداً ويملأ لوحة التفاصيل
// ---------------------------------------------------------------------------
void URok2CommanderWidget::SelectCommander(const FString& CommanderId)
{
	SelectedCommanderId = CommanderId;

	// جلب البيانات المفصلة
	FRok2CommanderDetailData Detail = LoadCommanderDetail(CommanderId);
	PopulateDetailPanel(Detail);

	OnCommanderSelected.Broadcast(CommanderId);
}

// ---------------------------------------------------------------------------
// PopulateDetailPanel — يملأ لوحة التفاصيل
// ---------------------------------------------------------------------------
void URok2CommanderWidget::PopulateDetailPanel(const FRok2CommanderDetailData& Detail)
{
	// بورتريه التفاصيل الكبير: حقيقي إن استُورد
	if (DetailPortraitImage)
	{
		if (UTexture2D* Tex = LoadCommanderPortrait(Detail.Id))
		{
			DetailPortraitImage->SetBrushFromTexture(Tex);
			DetailPortraitImage->SetDesiredSizeOverride(FVector2D(160.f, 160.f));
		}
	}

	if (DetailNameText) DetailNameText->SetText(FText::FromString(Detail.Name));
	if (DetailRarityText)
	{
		DetailRarityText->SetText(FText::FromString(Detail.Rarity));
		DetailRarityText->SetColorAndOpacity(FSlateColor(RarityColor(Detail.Rarity)));
	}
	if (DetailNationText) DetailNationText->SetText(FText::FromString(Detail.Nation));
	if (DetailLevelText) DetailLevelText->SetText(FText::FromString(FString::Printf(TEXT("المستوى %d"), Detail.Level)));

	// نجوم
	FString StarsStr;
	for (int32 i = 0; i < 6; ++i)
	{
		StarsStr += (i < Detail.Stars) ? TEXT("★") : TEXT("☆");
	}
	if (DetailStarsText) DetailStarsText->SetText(FText::FromString(StarsStr));

	// شريط الخبرة
	if (DetailXpBar && Detail.XpToNext > 0)
	{
		DetailXpBar->SetPercent(FMath::Clamp((float)Detail.Xp / (float)Detail.XpToNext, 0.f, 1.f));
	}

	// إحصائيات
	if (DetailStatsText)
	{
		FString StatsStr = FString::Printf(TEXT("هجوم %d · دفاع %d · دعم %d"), Detail.Attack, Detail.Defense, Detail.Utility);
		DetailStatsText->SetText(FText::FromString(StatsStr));
	}

	// المهارات
	if (SkillsBox)
	{
		SkillsBox->ClearChildren();
		for (int32 i = 0; i < Detail.Skills.Num(); ++i)
		{
			UWidget* SkillRow = BuildSkillRow(Detail.Skills[i], i + 1);
			if (SkillRow)
			{
				UVerticalBoxSlot* RowSlot = SkillsBox->AddChildToVerticalBox(SkillRow);
				RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			}
		}
	}

	// المواهب (stub)
	if (TalentsBox)
	{
		TalentsBox->ClearChildren();
		UWidget* TalentStub = BuildTalentTreeStub();
		if (TalentStub) TalentsBox->AddChildToVerticalBox(TalentStub);
	}

	// المعدات (stub)
	if (EquipmentBox)
	{
		EquipmentBox->ClearChildren();
		UWidget* EquipStub = BuildEquipmentSlots();
		if (EquipStub) EquipmentBox->AddChildToHorizontalBox(EquipStub);
	}
}

// ---------------------------------------------------------------------------
// BuildSkillRow — صف مهارة في لوحة التفاصيل
// ---------------------------------------------------------------------------
UWidget* URok2CommanderWidget::BuildSkillRow(const FRok2CommanderSkillData& Skill, int32 SlotIndex)
{
	UBorder* SkillBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	SkillBorder->SetBrushColor(FLinearColor(0.12f, 0.1f, 0.06f, 0.7f));
	SkillBorder->SetPadding(FMargin(8.f));

	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	SkillBorder->SetContent(RowBox);

	// أيقونة النوع
	FString TypeIcon;
	FLinearColor TypeColor;
	if (Skill.Type == TEXT("attack"))
	{
		TypeIcon = TEXT("⚔️");
		TypeColor = FLinearColor(0.9f, 0.3f, 0.2f);
	}
	else if (Skill.Type == TEXT("defense"))
	{
		TypeIcon = TEXT("🛡️");
		TypeColor = FLinearColor(0.2f, 0.5f, 0.9f);
	}
	else
	{
		TypeIcon = TEXT("✨");
		TypeColor = FLinearColor(0.9f, 0.7f, 0.2f);
	}

	UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	IconText->SetText(FText::FromString(TypeIcon));
	IconText->SetColorAndOpacity(FSlateColor(TypeColor));
	IconText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
	UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(IconText);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	IconSlot->SetVerticalAlignment(VAlign_Center);

	// معلومات المهارة
	UVerticalBox* SkillInfo = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RowBox->AddChildToHorizontalBox(SkillInfo);

	// اسم + مستوى
	FString NameLevel = FString::Printf(TEXT("%s  L%d/%d"), *Skill.Name, Skill.CurrentLevel, Skill.MaxLevel);
	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(NameLevel));
	NameText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
	NameText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
	SkillInfo->AddChildToVerticalBox(NameText);

	// وصف
	UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DescText->SetText(FText::FromString(Skill.Description));
	DescText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)));
	DescText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
	SkillInfo->AddChildToVerticalBox(DescText);

	return SkillBorder;
}

// ---------------------------------------------------------------------------
// BuildTalentTreeStub — شجرة مواهب stub (3 فروع ملونة)
// ---------------------------------------------------------------------------
UWidget* URok2CommanderWidget::BuildTalentTreeStub()
{
	UHorizontalBox* TreeBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// 3 فروع: أحمر (قتال)، أصفر (دعم)، أزرق (حركة)
	const TArray<TPair<FString, FLinearColor>> Branches = {
		{TEXT("قتال"), FLinearColor(0.8f, 0.2f, 0.2f)},
		{TEXT("دعم"), FLinearColor(0.9f, 0.7f, 0.1f)},
		{TEXT("حركة"), FLinearColor(0.2f, 0.5f, 0.9f)}
	};

	for (const auto& Branch : Branches)
	{
		UBorder* BranchBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		BranchBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.6f));
		BranchBorder->SetPadding(FMargin(8.f));
		UHorizontalBoxSlot* BranchSlot = TreeBox->AddChildToHorizontalBox(BranchBorder);
		BranchSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

		UVerticalBox* BranchBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		BranchBorder->SetContent(BranchBox);

		UTextBlock* BranchTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		BranchTitle->SetText(FText::FromString(Branch.Key));
		BranchTitle->SetColorAndOpacity(FSlateColor(Branch.Value));
		BranchTitle->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
		BranchTitle->SetJustification(ETextJustify::Center);
		BranchBox->AddChildToVerticalBox(BranchTitle);

		UTextBlock* PointsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		PointsText->SetText(FText::FromString(TEXT("0/20 نقطة")));
		PointsText->SetColorAndOpacity(FSlateColor(COLOR_IVORY));
		PointsText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
		PointsText->SetJustification(ETextJustify::Center);
		BranchBox->AddChildToVerticalBox(PointsText);
	}

	return TreeBox;
}

// ---------------------------------------------------------------------------
// BuildEquipmentSlots — خانات المعدات stub (4 خانات + إكسسوار)
// ---------------------------------------------------------------------------
UWidget* URok2CommanderWidget::BuildEquipmentSlots()
{
	UHorizontalBox* SlotsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	const TArray<FString> SlotNames = {
		TEXT("⚔️ سلاح"),
		TEXT("🪖 خوذة"),
		TEXT("🛡️ درع"),
		TEXT("👢 حذاء"),
		TEXT("💍 إكسسوار")
	};

	for (const FString& SlotName : SlotNames)
	{
		UBorder* SlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		SlotBorder->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 0.7f));
		SlotBorder->SetPadding(FMargin(6.f));
		UHorizontalBoxSlot* SlotSlot = SlotsBox->AddChildToHorizontalBox(SlotBorder);
		SlotSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));

		UTextBlock* SlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SlotText->SetText(FText::FromString(SlotName));
		SlotText->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)));
		SlotText->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
		SlotText->SetJustification(ETextJustify::Center);
		SlotBorder->SetContent(SlotText);
	}

	return SlotsBox;
}

// ---------------------------------------------------------------------------
// RarityColor / StarsForRarity
// ---------------------------------------------------------------------------
FLinearColor URok2CommanderWidget::RarityColor(const FString& Rarity)
{
	if (Rarity == TEXT("legendary")) return COLOR_LEGENDARY;
	if (Rarity == TEXT("epic")) return COLOR_EPIC;
	if (Rarity == TEXT("elite")) return COLOR_ELITE;
	return COLOR_ADVANCED;
}

int32 URok2CommanderWidget::StarsForRarity(const FString& Rarity)
{
	if (Rarity == TEXT("legendary")) return 5;
	if (Rarity == TEXT("epic")) return 4;
	if (Rarity == TEXT("elite")) return 3;
	return 2;
}

// ---------------------------------------------------------------------------
// LoadCommanderDetail — يقرأ بيانات قائد مفصلة من commanders.json
// ---------------------------------------------------------------------------
FRok2CommanderDetailData URok2CommanderWidget::LoadCommanderDetail(const FString& CommanderId)
{
	// أولاً: من الذاكرة المحملة مسبقاً
	if (FRok2CommanderDetailData* Found = CommanderDetails.Find(CommanderId))
	{
		return *Found;
	}

	// fallback: بيانات افتراضية
	FRok2CommanderDetailData D;
	D.Id = CommanderId;
	D.Name = TEXT("قائد غير معروف");
	D.Rarity = TEXT("elite");
	D.Nation = TEXT("rome");
	D.Level = 1;
	D.Stars = 1;
	D.Xp = 0;
	D.XpToNext = 1000;
	return D;
}

// ---------------------------------------------------------------------------
// LoadCommanderDetailsFromJson — يقرأ commanders.json من القرص
// ---------------------------------------------------------------------------
void URok2CommanderWidget::LoadCommanderDetailsFromJson()
{
	if (bDetailsLoaded) return;
	bDetailsLoaded = true;

	const FString JsonPath = FPaths::ProjectContentDir() / TEXT("../../data/commanders.json");
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		UE_LOG(LogRok2Cmdr, Log, TEXT("commanders.json not found at %s — using fallback"), *JsonPath);
		return;
	}

	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		UE_LOG(LogRok2Cmdr, Warning, TEXT("Failed to parse commanders.json"));
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* CmdrsArray = nullptr;
	if (!RootObj->TryGetArrayField(TEXT("commanders"), CmdrsArray) || !CmdrsArray) return;

	for (const TSharedPtr<FJsonValue>& V : *CmdrsArray)
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (!V->TryGetObject(ObjPtr) || !ObjPtr || !ObjPtr->IsValid()) continue;

		FRok2CommanderDetailData D;
		(*ObjPtr)->TryGetStringField(TEXT("id"), D.Id);
		(*ObjPtr)->TryGetStringField(TEXT("name"), D.Name);
		(*ObjPtr)->TryGetStringField(TEXT("rarity"), D.Rarity);
		(*ObjPtr)->TryGetStringField(TEXT("nation"), D.Nation);

		// tags
		const TArray<TSharedPtr<FJsonValue>>* TagsArray = nullptr;
		if ((*ObjPtr)->TryGetArrayField(TEXT("tags"), TagsArray) && TagsArray)
		{
			for (const TSharedPtr<FJsonValue>& TV : *TagsArray)
			{
				FString Tag;
				if (TV->TryGetString(Tag)) D.Tags.Add(Tag);
			}
		}

		// base_stats
		const TSharedPtr<FJsonObject>* StatsObj = nullptr;
		if ((*ObjPtr)->TryGetObjectField(TEXT("base_stats"), StatsObj) && StatsObj && StatsObj->IsValid())
		{
			double Atk = 50, Def = 50, Util = 50;
			(*StatsObj)->TryGetNumberField(TEXT("attack"), Atk);
			(*StatsObj)->TryGetNumberField(TEXT("defense"), Def);
			(*StatsObj)->TryGetNumberField(TEXT("utility"), Util);
			D.Attack = FMath::RoundToInt(Atk);
			D.Defense = FMath::RoundToInt(Def);
			D.Utility = FMath::RoundToInt(Util);
		}

		// skills
		const TArray<TSharedPtr<FJsonValue>>* SkillsArray = nullptr;
		if ((*ObjPtr)->TryGetArrayField(TEXT("skills"), SkillsArray) && SkillsArray)
		{
			for (const TSharedPtr<FJsonValue>& SV : *SkillsArray)
			{
				const TSharedPtr<FJsonObject>* SObjPtr = nullptr;
				if (!SV->TryGetObject(SObjPtr) || !SObjPtr || !SObjPtr->IsValid()) continue;

				FRok2CommanderSkillData S;
				(*SObjPtr)->TryGetStringField(TEXT("id"), S.Id);
				(*SObjPtr)->TryGetStringField(TEXT("name"), S.Name);
				(*SObjPtr)->TryGetStringField(TEXT("type"), S.Type);
				(*SObjPtr)->TryGetStringField(TEXT("description"), S.Description);

				double MaxLvl = 5;
				(*SObjPtr)->TryGetNumberField(TEXT("max_level"), MaxLvl);
				S.MaxLevel = FMath::RoundToInt(MaxLvl);
				S.CurrentLevel = 1; // يُقرأ من بيانات اللاعب لاحقاً

				// effects
				const TArray<TSharedPtr<FJsonValue>>* EffectsArray = nullptr;
				if ((*SObjPtr)->TryGetArrayField(TEXT("effects"), EffectsArray) && EffectsArray && EffectsArray->Num() > 0)
				{
					const TSharedPtr<FJsonObject>* EffObj = nullptr;
					if ((*EffectsArray)[0]->TryGetObject(EffObj) && EffObj && EffObj->IsValid())
					{
						(*EffObj)->TryGetStringField(TEXT("stat"), S.StatName);
						double PerLvl = 0;
						(*EffObj)->TryGetNumberField(TEXT("per_level"), PerLvl);
						S.PerLevel = (float)PerLvl;
					}
				}

				D.Skills.Add(S);
			}
		}

		// قيم افتراضية للمستوى والنجوم
		D.Level = 1;
		D.Stars = StarsForRarity(D.Rarity);
		D.Xp = 0;
		D.XpToNext = 1000;

		CommanderDetails.Add(D.Id, D);
	}

	UE_LOG(LogRok2Cmdr, Log, TEXT("Loaded %d commander details from JSON"), CommanderDetails.Num());
}

// ---------------------------------------------------------------------------
// Event Handlers
// ---------------------------------------------------------------------------
void URok2CommanderWidget::OnCardClicked()
{
	// في UE5 حقيقي، نحتاج لمعرفة أي بطاقة ضُغطت — نستخدم SelectedCommanderId
	// كـ workaround، نختار أول قائد في القائمة (للاختبار)
	if (Api && Api->GetCommanders().Num() > 0)
	{
		SelectCommander(Api->GetCommanders()[0].Id);
	}
}

void URok2CommanderWidget::OnAssignClicked()
{
	if (!SelectedCommanderId.IsEmpty())
	{
		OnAssignCommander.Broadcast(SelectedCommanderId);
	}
}

void URok2CommanderWidget::OnLevelUpClicked()
{
	// يُربط بـ Api->LevelUpCommander(SelectedCommanderId) لاحقاً
	UE_LOG(LogRok2Cmdr, Log, TEXT("Level up clicked for %s"), *SelectedCommanderId);
}

void URok2CommanderWidget::OnSkillUpgradeClicked()
{
	// يُربط بـ Api->UpgradeCommanderSkill(SelectedCommanderId, SkillId) لاحقاً
	UE_LOG(LogRok2Cmdr, Log, TEXT("Skill upgrade clicked for %s"), *SelectedCommanderId);
}
