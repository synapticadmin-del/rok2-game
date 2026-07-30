// P6-T3: بطاقة الدخول تظهر بتلاشٍ + ضغطة محسوسة على أزرار الدخول والبدء.
// P6-T5: نبذة الحضارة الأدبية تظهر تحت القائمة وتتبدّل مع كل اختيار.

#include "Rok2BootWidget.h"
#include "Rok2Typography.h"
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
#include "Blueprint/WidgetTree.h"

// ألوان الهوية من ui-ux-design-system.md §1 — محلية للملف على اصطلاح
// Rok2FtueStyle/Rok2HudStyle (الألوان بقيت مسؤولية كل ودجة في P6-T2).
namespace Rok2BootLoreStyle
{
	static const FLinearColor PanelBg(0.10f, 0.07f, 0.04f, 0.92f);	// #1A120B
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f);				// #C9A227
	static const FLinearColor Ivory(0.96f, 0.91f, 0.81f);			// #F5E9D0
	static const FLinearColor Muted(0.72f, 0.68f, 0.60f, 0.95f);

	/** عرض النبذة داخل بطاقة الدخول (520px ناقص هامشَي 30) */
	static constexpr float StoryWidth = 460.f;
}

void URok2BootWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnLoginComplete.AddDynamic(this, &URok2BootWidget::OnLoginComplete);
	Api->OnPlayerLoaded.AddDynamic(this, &URok2BootWidget::OnPlayerLoaded);
	Api->OnApiError.AddDynamic(this, &URok2BootWidget::OnApiError);
	Api->OnConnectionState.AddDynamic(this, &URok2BootWidget::OnConnectionState);
	// P6-T5: حمولة /v1/meta/all تصل بعد Setup عادةً، وهي التي تحمل نصّ الخادم.
	Api->OnMetaLoaded.AddDynamic(this, &URok2BootWidget::OnMetaLoaded);

	if (EnterButton)
	{
		EnterButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnEnterClicked);
		URok2MotionLibrary::BindPress(EnterButton);	// P6-T3: ضغطة محسوسة
	}
	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnStartClicked);
		URok2MotionLibrary::BindPress(StartButton);	// P6-T3: ضغطة محسوسة
	}

	// Populate civ combo — P6-T5: القائمة من البيانات، والنبذة تتبع الاختيار
	if (CivCombo)
	{
		CivCombo->OnSelectionChanged.AddDynamic(this, &URok2BootWidget::OnCivSelectionChanged);
		PopulateCivCombo(FString());
	}

	if (Api->IsLoggedIn())
	{
		// try direct resume
		SetLoading(true, TEXT("جاري استعادة الجلسة"));
		if (Api->HasPlayer())
		{
			OnPlayerLoaded(Api->GetPlayer());
		}
		else
		{
			Api->LoadCity();
		}
	}
	else
	{
		// دخول تلقائي كضيف بدأ للتو من GameMode — أظهر التحميل
		SetLoading(true, TEXT("جاري الاتصال بالخادم"));
	}
}

void URok2BootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBorder"));
		CardBorder->SetBrushColor(FLinearColor(0.04f, 0.07f, 0.14f, 0.94f));

		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(CardBorder);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(520.f, 480.f));

		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		CardBorder->SetContent(VBox);

		// Title — P6-T1: تاج إجرائي + عنوان اللعبة
		{
			UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			UVerticalBoxSlot* TitleRowSlot = VBox->AddChildToVerticalBox(TitleRow);
			TitleRowSlot->SetHorizontalAlignment(HAlign_Center);
			TitleRowSlot->SetPadding(FMargin(0, 15, 0, 5));
			UImage* CrownIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			CrownIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("crown"), 26.f, FLinearColor(1.0f, 0.84f, 0.2f)));
			CrownIco->SetDesiredSizeOverride(FVector2D(26.f, 26.f));
			UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(CrownIco);
			IcoSlot->SetPadding(FMargin(0, 0, 8, 0));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
			TitleText->SetText(FText::FromString(TEXT("ROK2 : RISE OF KINGDOMS 2")));
			TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
			URok2Typography::ApplyFont(TitleText, ERok2TextRole::Display);
			TitleText->SetFont(TitleFont);
			TitleRow->AddChildToHorizontalBox(TitleText)->SetVerticalAlignment(VAlign_Center);
		}

		// Subtitle
		UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		SubtitleText->SetText(FText::FromString(TEXT("مرحباً بك يا حاكم — اختر اسمك وحضارتك لإنشاء المملكة")));
		SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.85f, 0.9f)));
		UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(SubtitleText);
		SubSlot->SetHorizontalAlignment(HAlign_Center);
		SubSlot->SetPadding(FMargin(0, 0, 0, 20));

		// Enter Button (Guest login) — P6-T1: أيقونة برق إجرائية + نص
		EnterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EnterButton"));
		{
			UHorizontalBox* EnterBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			EnterButton->AddChild(EnterBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("ap"), 16.f, FLinearColor::White));
			Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
			UHorizontalBoxSlot* IcoSlot = EnterBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8, 2, 5, 2));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* EnterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnterText"));
			EnterText->SetText(FText::FromString(TEXT("دخول سريع كضيف (Quick Guest Login)")));
			EnterText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(EnterText, ERok2TextRole::Button);
			EnterText->SetFont(BtnFont);
			EnterBox->AddChildToHorizontalBox(EnterText)->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* EnterSlot = VBox->AddChildToVerticalBox(EnterButton);
		EnterSlot->SetPadding(FMargin(30, 10, 30, 15));

		// Name Input
		NameInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("NameInput"));
		NameInput->SetHintText(FText::FromString(TEXT("اسم الحاكم (Governor Name)...")));
		NameInput->SetText(FText::FromString(TEXT("Governor")));
		UVerticalBoxSlot* NameSlot = VBox->AddChildToVerticalBox(NameInput);
		NameSlot->SetPadding(FMargin(30, 5, 30, 10));

		// Civ Dropdown Combo
		CivCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("CivCombo"));
		UVerticalBoxSlot* CivSlot = VBox->AddChildToVerticalBox(CivCombo);
		CivSlot->SetPadding(FMargin(30, 5, 30, 8));

		// P6-T5: نبذة الحضارة — تحت القائمة مباشرة، فالعين تقرأ ما اختارته
		BuildLorePanel(VBox);

		// Start Journey Button — P6-T1: أيقونة سيف إجرائية + نص
		StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		{
			UHorizontalBox* StartBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			StartButton->AddChild(StartBox);
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("sword"), 16.f, FLinearColor::White));
			Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
			UHorizontalBoxSlot* IcoSlot = StartBox->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(8, 2, 5, 2));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* StartText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartText"));
			StartText->SetText(FText::FromString(TEXT("ابدأ رحلة التوسع والمجد (Start Journey)")));
			StartText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			URok2Typography::ApplyFont(StartText, ERok2TextRole::Button);
			StartText->SetFont(StartBtnFont);
			StartBox->AddChildToHorizontalBox(StartText)->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* StartBtnSlot = VBox->AddChildToVerticalBox(StartButton);
		StartBtnSlot->SetPadding(FMargin(30, 5, 30, 15));

		// --- شاشة التحميل: لوحة سفلية بنص متحرك ---
		LoadingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingPanel"));
		LoadingPanel->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
		LoadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingText"));
		LoadingText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
		URok2Typography::ApplyFont(LoadingText, ERok2TextRole::BodySmall);
		LoadingText->SetFont(LoadFont);
		LoadingText->SetJustification(ETextJustify::Center);
		LoadingPanel->SetContent(LoadingText);
		LoadingPanel->SetPadding(FMargin(0, 10, 0, 10));
		UVerticalBoxSlot* LoadSlot = VBox->AddChildToVerticalBox(LoadingPanel);
		LoadSlot->SetPadding(FMargin(30, 5, 30, 5));

		// --- نص حالة الاتصال (أخطاء/إعادة محاولة) ---
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.55f, 0.4f)));
		URok2Typography::ApplyFont(StatusText, ERok2TextRole::Caption);
		StatusText->SetFont(StatusFont);
		StatusText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* StatusSlot = VBox->AddChildToVerticalBox(StatusText);
		StatusSlot->SetPadding(FMargin(30, 2, 30, 12));

		// Initial visibility
		NameInput->SetVisibility(ESlateVisibility::Collapsed);
		CivCombo->SetVisibility(ESlateVisibility::Collapsed);
		StartButton->SetVisibility(ESlateVisibility::Collapsed);
		LoadingPanel->SetVisibility(ESlateVisibility::Collapsed);
		StatusText->SetText(FText::GetEmpty());
		// النبذة تُطوى مع القائمة: قبل تسجيل الدخول لا اختيار فلا نبذة
		if (LorePanel) LorePanel->SetVisibility(ESlateVisibility::Collapsed);

		// P6-T3: أول شاشة يراها اللاعب تظهر بتلاشٍ هادئ لا ظهور مفاجئ
		URok2MotionLibrary::PlayFadeIn(CardBorder);
	}
}

// ---------------------------------------------------------------------------
// P6-T5: لوحة النبذة الأدبية
// ---------------------------------------------------------------------------

void URok2BootWidget::BuildLorePanel(UVerticalBox* VBox)
{
	if (!VBox || !WidgetTree) return;

	LorePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LorePanel"));
	LorePanel->SetBrushColor(Rok2BootLoreStyle::PanelBg);
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
	}
}

void URok2BootWidget::OnCivSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	ShowLoreFor(SelectedCivId());
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

	URok2CivLore* Lore = URok2CivLore::Get();
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
		if (CivCombo) CivCombo->SetVisibility(ESlateVisibility::Visible);
		// P6-T5: النبذة تظهر مع القائمة — الاختيار الحالي هو أول ما يُقرأ
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
