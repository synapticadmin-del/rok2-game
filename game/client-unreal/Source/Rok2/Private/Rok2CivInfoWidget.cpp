// Copyright ROK2. Civilization identity screen (P6-T5) — implementation.

#include "Rok2CivInfoWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
#include "Rok2CivLore.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2CivInfo, Log, All);

// الألوان من Rok2Visual؛ الأبعاد وحدها تبقى محلية. كانت هنا نسخة خامسة من
// الذهب والعاج بنفس القيم، فتعديل الهوية كان يعني تعديل ستة ملفات.
namespace Rok2CivInfoStyle
{
	static const FLinearColor SheetBg = Rok2Visual::Panel();
	static const FLinearColor Gold = Rok2Visual::GoldText();
	static const FLinearColor Ivory = Rok2Visual::Ivory();
	static const FLinearColor Muted = Rok2Visual::Muted();
	static const FLinearColor Backdrop = Rok2Visual::Scrim();

	/** ارتفاع اللوحة: ترويسة + 4 أسطر حكاية + تحية + 3 تلميحات */
	static constexpr float SheetHeight = 400.f;

	/** سماكة مقبض السحب العلوي */
	static constexpr float HandleHeight = 4.f;
	static constexpr float HandleWidth = 60.f;
}


TSharedRef<SWidget> URok2CivInfoWidget::RebuildWidget()
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

void URok2CivInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CivInfoRoot"));
	WidgetTree->RootWidget = RootCanvas;

	BuildSheet(RootCanvas);
}

void URok2CivInfoWidget::BuildSheet(UCanvasPanel* RootCanvas)
{
	if (!RootCanvas) return;

	// خلفية معتمة تُلمس للإغلاق — نفس اصطلاح بطاقة المبنى، فالإغلاق يُتعلَّم مرة
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CivInfoBackdrop"));
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetSize(FVector2D(0.f, 0.f));
	Backdrop->OnClicked.AddDynamic(this, &URok2CivInfoWidget::OnCloseClicked);

	// اللوحة السفلية بعرض كامل (§1 Bottom Sheet)
	SheetBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CivInfoSheet"));
	SheetBorder->SetBrush(Rok2Surface::Sheet());
	SheetBorder->SetPadding(FMargin(20.f, 10.f, 20.f, 16.f));
	UCanvasPanelSlot* SheetSlot = RootCanvas->AddChildToCanvas(SheetBorder);
	SheetSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
	SheetSlot->SetAlignment(FVector2D(0.5f, 1.f));
	SheetSlot->SetPosition(FVector2D(0.f, 0.f));
	SheetSlot->SetSize(FVector2D(0.f, Rok2CivInfoStyle::SheetHeight));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CivInfoVBox"));
	SheetBorder->SetContent(VBox);

	// مقبض السحب — إشارة «هذه لوحة تُغلق» قبل أي نصّ.
	// الأبعاد من Spacer داخلي لا من الـBorder: الـBorder يقيس محتواه، فبلا
	// محتوى ذي حجم يصير المقبض شريطاً بلا ارتفاع أي غير مرئي.
	{
		UBorder* Handle = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CivInfoHandle"));
		Handle->SetBrush(Rok2Surface::SheetHandle());
		USpacer* HandleSize = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		HandleSize->SetSize(FVector2D(Rok2CivInfoStyle::HandleWidth, Rok2CivInfoStyle::HandleHeight));
		Handle->SetContent(HandleSize);

		UVerticalBoxSlot* HandleSlot = VBox->AddChildToVerticalBox(Handle);
		HandleSlot->SetHorizontalAlignment(HAlign_Center);
		HandleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		HandleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// --- الترويسة: تاج + اسم الحضارة + زر إغلاق ---
	{
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		VBox->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));

		UImage* Crown = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Crown->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("crown"), 30.f, Rok2CivInfoStyle::Gold));
		Crown->SetDesiredSizeOverride(FVector2D(30.f, 30.f));
		// P7-T7: نص بديل لأيقونة التاج في ترويسة الحضارة
		Crown->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("crown")));
		UHorizontalBoxSlot* CrownSlot = HeaderRow->AddChildToHorizontalBox(Crown);
		CrownSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
		CrownSlot->SetVerticalAlignment(VAlign_Center);
		CrownSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivInfoName"));
		NameText->SetColorAndOpacity(FSlateColor(Rok2CivInfoStyle::Gold));
		// Display: وثيقة Typography تخصّ هذا الدور بـ«اسم الحضارة» صراحةً
		URok2Typography::ApplyFont(NameText, ERok2TextRole::Display);
		HeaderRow->AddChildToHorizontalBox(NameText)->SetVerticalAlignment(VAlign_Center);

		HeaderRow->AddChildToHorizontalBox(
			WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass())
		)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// زر إغلاق صريح إلى جانب لمس الخلفية: اللوحة بعرض الشاشة كاملاً فقد
		// لا يجد اللاعب على الهاتف مساحة خلفية ظاهرة يلمسها.
		UButton* CloseBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CivInfoClose"));
		CloseBtn->OnClicked.AddDynamic(this, &URok2CivInfoWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(CloseBtn);
		UImage* CloseIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		CloseIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("close"), 18.f, Rok2CivInfoStyle::Muted));
		CloseIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
		// P7-T7: نص بديل لزر إغلاق لوحة الحضارة
		CloseIco->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		CloseBtn->SetToolTipText(FText::FromString(TEXT("إغلاق لوحة الحضارة")));
		CloseBtn->AddChild(CloseIco);
		UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(CloseBtn);
		CloseSlot->SetVerticalAlignment(VAlign_Center);
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// --- جملة الفانتازي ---
	FantasyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivInfoFantasy"));
	FantasyText->SetColorAndOpacity(FSlateColor(Rok2CivInfoStyle::Muted));
	URok2Typography::ApplyFont(FantasyText, ERok2TextRole::Subtitle);
	FantasyText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(FantasyText)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	// --- النبذة الأدبية: متن الشاشة ---
	StoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivInfoStory"));
	StoryText->SetColorAndOpacity(FSlateColor(Rok2CivInfoStyle::Ivory));
	URok2Typography::ApplyFont(StoryText, ERok2TextRole::Body);
	StoryText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(StoryText)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	// --- التحية: صوت الحضارة، بينها وبين التلميحات فاصل بصري ---
	GreetingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CivInfoGreeting"));
	GreetingText->SetColorAndOpacity(FSlateColor(Rok2CivInfoStyle::Gold));
	URok2Typography::ApplyFont(GreetingText, ERok2TextRole::BodySmall);
	GreetingText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(GreetingText)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));

	// --- التلميحات بنبرة الحضارة ---
	HintsBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CivInfoHints"));
	VBox->AddChildToVerticalBox(HintsBox);
}

void URok2CivInfoWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	RefreshFromPlayer();
}

void URok2CivInfoWidget::FillHints(const TArray<FString>& Hints)
{
	if (!HintsBox || !WidgetTree) return;
	HintsBox->ClearChildren();

	for (const FString& Hint : Hints)
	{
		if (Hint.IsEmpty()) continue;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		// أيقونة إجرائية لا نقطة نصّية — قاعدة P6-T1: «لا إيموجي»، ووثيقة UI §8.2
		UImage* Bullet = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Bullet->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("sparkle"), 14.f, Rok2CivInfoStyle::Gold));
		Bullet->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		UHorizontalBoxSlot* BulletSlot = Row->AddChildToHorizontalBox(Bullet);
		BulletSlot->SetPadding(FMargin(0.f, 3.f, 8.f, 0.f));
		BulletSlot->SetVerticalAlignment(VAlign_Top);
		BulletSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Hint));
		Text->SetColorAndOpacity(FSlateColor(Rok2CivInfoStyle::Ivory));
		URok2Typography::ApplyFont(Text, ERok2TextRole::Micro);
		Text->SetAutoWrapText(true);
		// Fill لا Automatic: بلا حدٍّ أفقي لا يلتفّ AutoWrapText، فالتلميح العربي
		// كان يمتدّ سطراً واحداً يخرج من اللوحة.
		Row->AddChildToHorizontalBox(Text)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		HintsBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 0.f, 0.f, 5.f));
	}
}

void URok2CivInfoWidget::RefreshFromPlayer()
{
	if (!Api) return;

	// حضارة اللاعب من حمولة الخادم — لا من اختيار محلي قد لا يكون جرى أصلاً
	const FString CivId = Api->GetPlayer().Civ;

	URok2CivLoreRegistry* Lore = URok2CivLoreRegistry::Get();
	const bool bHasLore = Lore && Lore->HasLore(CivId);

	if (NameText)
	{
		// اسم عربي إن وُجد، وإلا المعرّف نفسه: شاشة بلا عنوان أسوأ من شاشة
		// بعنوانٍ تقني، وحالة «معرّف من خادم أحدث» واقعية لا فرضية.
		const FString Name = bHasLore ? Lore->GetLore(CivId).NameAr : CivId;
		NameText->SetText(FText::FromString(Name));
	}

	if (bHasLore)
	{
		const FRok2CivLore& L = Lore->GetLore(CivId);
		if (FantasyText)  FantasyText->SetText(FText::FromString(L.FantasyAr));
		if (StoryText)    StoryText->SetText(FText::FromString(Lore->StoryText(CivId)));
		if (GreetingText) GreetingText->SetText(FText::FromString(L.Greeting));

		// صفوف التلميحات تُبنى من جديد، فلا نعيد بناءها بلا تغيّر حضارة
		if (RenderedCivId != CivId)
		{
			FillHints(L.Hints);
		}
	}
	else
	{
		// لا نصّ يُخترع مكان نصّ مفقود — تُفرَّغ الأقسام وتبقى الشاشة صادقة
		if (FantasyText)  FantasyText->SetText(FText::GetEmpty());
		if (StoryText)    StoryText->SetText(FText::GetEmpty());
		if (GreetingText) GreetingText->SetText(FText::GetEmpty());
		if (RenderedCivId != CivId)
		{
			FillHints(TArray<FString>());
		}
		UE_LOG(LogRok2CivInfo, Warning, TEXT("No lore for civ '%s' — showing identity without narrative"), *CivId);
	}

	RenderedCivId = CivId;

	// §1 «كل لوحة تنزلق من أسفل» — تُشغَّل عند كل فتح لا عند البناء وحده،
	// فاللوحة تُنشأ مرة وتُعاد للعرض مراراً.
	if (SheetBorder)
	{
		URok2MotionLibrary::PlaySlideInBottom(SheetBorder);
	}
}

void URok2CivInfoWidget::OnCloseClicked()
{
	// PlayFadeOut على الودجة نفسها: تخرج اللوحة وخلفيتها المعتمة معاً ثم تُزال
	// من الشجرة — الطريقة المعيارية للإغلاق (§1 «لا قفزات جامدة»).
	URok2MotionLibrary::PlayFadeOut(this);
}
