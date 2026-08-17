// Copyright ROK2. شاشة الإعدادات (P18-T6) — implementation.
// P6-T3: تفتح بحركة `PlayScaleInCenter` وتُسرَّح بـ`PlayFadeOut`، وكل زر بضغطة
// محسوسة من `URok2MotionLibrary` — لا حركة محلية ولا إزالة مفاجئة.

#include "Rok2SettingsWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2ArtAssets.h"
#include "Rok2AudioManager.h"
#include "Rok2MotionLibrary.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	/** حدود مقياس الواجهة — تطابق حرفياً ما تقصّه `URok2Accessibility::SetUiScale`. */
	constexpr float UiScaleMin = 0.85f;
	constexpr float UiScaleMax = 1.6f;

	/**
	 * نمط شريح من نظام التصميم.
	 *
	 * `USlider` بنمطه الافتراضي يرسم مسار Slate الرمادي ومقبضاً دائرياً أبيض —
	 * غريبان تماماً وسط برونز وذهب. المسار والمقبض هنا من `Rok2Surface` نفسه
	 * الذي يرسم أشرطة تقدّم الطوابير، فيتماسك الشكل.
	 */
	FSliderStyle MakeSliderStyle()
	{
		FSliderStyle Style;
		Style.SetNormalBarImage(Rok2Surface::ProgressTrack());
		Style.SetHoveredBarImage(Rok2Surface::ProgressTrack());
		Style.SetDisabledBarImage(Rok2Surface::ProgressTrack());
		Style.SetNormalThumbImage(Rok2Surface::Circle(Rok2Visual::Gold()));
		Style.SetHoveredThumbImage(Rok2Surface::Circle(Rok2Visual::GoldText()));
		Style.SetDisabledThumbImage(Rok2Surface::Circle(Rok2Visual::Muted()));
		Style.SetBarThickness(6.f);
		return Style;
	}

	/**
	 * نمط مربع اختيار من نظام التصميم — بحالاته الأربع.
	 *
	 * الحالة المحدّدة تحمل حشواً ذهبياً لا علامة صحّ وحدها: قاعدة §8.3 تمنع
	 * الاعتماد على شكل صغير واحد للتمييز.
	 */
	FCheckBoxStyle MakeCheckBoxStyle()
	{
		FCheckBoxStyle Style;
		Style.SetCheckBoxType(ESlateCheckBoxType::ToggleButton);
		Style.SetUncheckedImage(Rok2Surface::Pill(Rok2Visual::Card()));
		Style.SetUncheckedHoveredImage(Rok2Surface::OutlinedPill(Rok2Visual::Card(), Rok2Visual::GoldText()));
		Style.SetUncheckedPressedImage(Rok2Surface::OutlinedPill(Rok2Visual::Card(), Rok2Visual::Edge()));
		Style.SetCheckedImage(Rok2Surface::OutlinedPill(Rok2Visual::PrimaryAction(), Rok2Visual::Gold(), 2.f));
		Style.SetCheckedHoveredImage(Rok2Surface::OutlinedPill(Rok2Visual::PrimaryAction(), Rok2Visual::GoldText(), 2.f));
		Style.SetCheckedPressedImage(Rok2Surface::OutlinedPill(Rok2Visual::PrimaryAction(), Rok2Visual::Edge(), 2.f));
		Style.SetPadding(FMargin(Rok2Space::M, Rok2Space::S));
		return Style;
	}
}

TSharedRef<SWidget> URok2SettingsWidget::RebuildWidget()
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

void URok2SettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRoot"));
	WidgetTree->RootWidget = RootPanel;

	// حجاب يُلمس للإغلاق — نفس اصطلاح بطاقة المبنى وشاشة الحضارة.
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2SettingsWidget::OnCloseClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsCard"));
	Card->SetBrush(Rok2Surface::Panel());
	Card->SetPadding(FMargin(Rok2Space::XL));
	UCanvasPanelSlot* CardSlot = RootPanel->AddChildToCanvas(Card);
	CardSlot->SetAnchors(FAnchors(0.18f, 0.12f, 0.82f, 0.88f));
	CardSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsColumn"));
	Card->SetContent(Column);

	// ── الترويسة ──
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::L));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("wrench"), 24.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("wrench")));
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("الإعدادات")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
		Header->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

		USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Header->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsClose"));
		Close->SetStyle(Rok2Surface::PrimaryButton());
		Close->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		Close->OnClicked.AddDynamic(this, &URok2SettingsWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(Close);
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CloseText->SetText(FText::FromString(TEXT("تم")));
		CloseText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(CloseText, ERok2TextRole::Button);
		Close->AddChild(CloseText);
		Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);
	}

	// ── المحتوى (تمرير: الشاشة الصغيرة بمقياس 1.6 لا تتسع للصفوف الخمسة) ──
	UVerticalBox* Rows = nullptr;
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SettingsScroll"));
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRows"));
		Scroll->AddChild(Rows);
	}

	URok2AudioManager* Audio = URok2AudioManager::Get();
	URok2Accessibility* A11y = URok2Accessibility::Get();

	// ── الصوت ──
	MusicSlider = BuildSliderRow(Rows, TEXT("الموسيقى"), TEXT("bell"),
		Audio ? Audio->MusicVolume : 1.f, 0.f, 1.f, MusicValueText);
	if (MusicSlider)
	{
		MusicSlider->OnValueChanged.AddDynamic(this, &URok2SettingsWidget::OnMusicVolumeChanged);
	}

	SfxSlider = BuildSliderRow(Rows, TEXT("المؤثرات الصوتية"), TEXT("sparkle"),
		Audio ? Audio->SfxVolume : 1.f, 0.f, 1.f, SfxValueText);
	if (SfxSlider)
	{
		SfxSlider->OnValueChanged.AddDynamic(this, &URok2SettingsWidget::OnSfxVolumeChanged);
	}

	AudioEnabledCheck = BuildToggleRow(Rows, TEXT("تشغيل الصوت"), TEXT("bell"),
		TEXT("إيقافه يكتم الموسيقى والمؤثرات معاً."),
		Audio ? Audio->bAudioEnabled : true);
	if (AudioEnabledCheck)
	{
		AudioEnabledCheck->OnCheckStateChanged.AddDynamic(this, &URok2SettingsWidget::OnAudioEnabledChanged);
	}

	// ── العرض ──
	UiScaleSlider = BuildSliderRow(Rows, TEXT("مقياس الواجهة"), TEXT("stats"),
		A11y ? A11y->GetUiScale() : 1.f, UiScaleMin, UiScaleMax, UiScaleValueText);
	if (UiScaleSlider)
	{
		UiScaleSlider->OnValueChanged.AddDynamic(this, &URok2SettingsWidget::OnUiScaleChanged);
	}

	HighContrastCheck = BuildToggleRow(Rows, TEXT("تباين عالٍ"), TEXT("art"),
		TEXT("يثبّت ألوان النص على درجات عالية التباين (WCAG AA)."),
		A11y ? A11y->IsHighContrast() : false);
	if (HighContrastCheck)
	{
		HighContrastCheck->OnCheckStateChanged.AddDynamic(this, &URok2SettingsWidget::OnHighContrastChanged);
	}

	// ── إعادة الافتراضيات ──
	{
		UButton* Reset = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsReset"));
		Reset->SetStyle(Rok2Surface::SecondaryButton());
		Reset->OnClicked.AddDynamic(this, &URok2SettingsWidget::OnResetClicked);
		URok2MotionLibrary::BindPress(Reset);
		UTextBlock* ResetText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ResetText->SetText(FText::FromString(TEXT("إعادة الافتراضيات")));
		ResetText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(ResetText, ERok2TextRole::Button);
		Reset->AddChild(ResetText);
		Column->AddChildToVerticalBox(Reset)->SetPadding(FMargin(Rok2Space::None, Rok2Space::L, Rok2Space::None, Rok2Space::None));
	}

	UpdateValueTexts();
	URok2MotionLibrary::PlayScaleInCenter(Card);
	URok2MotionLibrary::PlayFadeIn(Backdrop);
}

USlider* URok2SettingsWidget::BuildSliderRow(UVerticalBox* Column, const FString& Label,
	const FString& IconId, float Value, float Min, float Max, UTextBlock*& OutValueText)
{
	if (!Column || !WidgetTree) return nullptr;

	UBorder* RowCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowCard->SetBrush(Rok2Surface::Card());
	RowCard->SetPadding(FMargin(Rok2Space::M));
	Column->AddChildToVerticalBox(RowCard)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RowCard->SetContent(Stack);

	// السطر الأول: أيقونة + عنوان + القيمة الرقمية.
	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Stack->AddChildToVerticalBox(TitleRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::XS));

	UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 16.f, Rok2Visual::GoldText()));
	Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
	Ico->SetToolTipText(URok2Accessibility::LabelForIcon(IconId));
	UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(Ico);
	IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
	IcoSlot->SetVerticalAlignment(VAlign_Center);
	IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(LabelText, ERok2TextRole::CardTitle);
	UHorizontalBoxSlot* LabelSlot = TitleRow->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	// القيمة نصاً: شريح بلا رقم لا يُضبط بدقة، ولا يُقرأ بلا بصر لوني.
	OutValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutValueText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
	URok2Typography::ApplyFont(OutValueText, ERok2TextRole::Numeric);
	UHorizontalBoxSlot* ValueSlot = TitleRow->AddChildToHorizontalBox(OutValueText);
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	// السطر الثاني: الشريح.
	USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetWidgetStyle(MakeSliderStyle());
	Slider->SetMinValue(Min);
	Slider->SetMaxValue(Max);
	Slider->SetStepSize((Max - Min) / 20.f);
	Slider->SetValue(FMath::Clamp(Value, Min, Max));
	Slider->SetToolTipText(FText::FromString(Label));
	Stack->AddChildToVerticalBox(Slider);

	return Slider;
}

UCheckBox* URok2SettingsWidget::BuildToggleRow(UVerticalBox* Column, const FString& Label,
	const FString& IconId, const FString& Hint, bool bChecked)
{
	if (!Column || !WidgetTree) return nullptr;

	UBorder* RowCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowCard->SetBrush(Rok2Surface::Card());
	RowCard->SetPadding(FMargin(Rok2Space::M));
	Column->AddChildToVerticalBox(RowCard)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RowCard->SetContent(Stack);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Stack->AddChildToVerticalBox(Row);

	UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 16.f, Rok2Visual::GoldText()));
	Ico->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
	Ico->SetToolTipText(URok2Accessibility::LabelForIcon(IconId));
	UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
	IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
	IcoSlot->SetVerticalAlignment(VAlign_Center);
	IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(LabelText, ERok2TextRole::CardTitle);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
	LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	UCheckBox* Check = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	Check->SetWidgetStyle(MakeCheckBoxStyle());
	Check->SetIsChecked(bChecked);
	Check->SetToolTipText(FText::FromString(Label));
	// نص داخل المربع لا علامة صحّ وحدها — §8.3 «لا اعتماد على اللون/الشكل فقط».
	UTextBlock* StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StateText->SetText(FText::FromString(TEXT("تشغيل")));
	StateText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(StateText, ERok2TextRole::Micro);
	Check->AddChild(StateText);
	UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(Check);
	CheckSlot->SetVerticalAlignment(VAlign_Center);
	CheckSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HintText->SetText(FText::FromString(Hint));
	HintText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
	HintText->SetAutoWrapText(true);
	URok2Typography::ApplyFont(HintText, ERok2TextRole::Micro);
	Stack->AddChildToVerticalBox(HintText)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XS, Rok2Space::None, Rok2Space::None));

	return Check;
}

// ---------------------------------------------------------------------------
// المعالجات — كل تغيير يسري فوراً ويُحفظ لحظتها
//
// لا زر «تطبيق» ولا «إلغاء»: ضبط الصوت والحجم يحتاج سماعاً ورؤيةً فورية، وأي
// حالة معلّقة تعني أن اللاعب يضبط رقماً لا يسمعه. والحفظ في كل تغيير مقبول
// لأنه أربعة حقول على القرص، لا كتابة ثقيلة.
// ---------------------------------------------------------------------------

void URok2SettingsWidget::OnMusicVolumeChanged(float Value)
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		// `SetMusicVolume` يطبّق على الموسيقى العاملة الآن عبر
		// `SetVolumeMultiplier` — فيسمع اللاعب الأثر وهو يحرّك الشريح.
		Audio->SetMusicVolume(Value);
	}
	UpdateValueTexts();
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		A11y->SaveSettings();
	}
}

void URok2SettingsWidget::OnSfxVolumeChanged(float Value)
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->SetSfxVolume(Value);
		// نموذج مسموع بالمستوى الجديد — وإلا كان ضبط المؤثرات صامتاً بلا مرجع.
		Audio->PlaySfx(ERok2AudioType::UiButtonClick);
	}
	UpdateValueTexts();
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		A11y->SaveSettings();
	}
}

void URok2SettingsWidget::OnUiScaleChanged(float Value)
{
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		// `SetUiScale` يضبط مقياس Slate كذلك، فتكبر الشاشة كلها فوراً — بما فيها
		// هذه اللوحة. الودجات المبنيّة سابقاً لا تُعاد، لكن Slate يكبّر ما هو
		// معروض، والجديد يُبنى على الرمز نفسه فيتسق.
		A11y->SetUiScale(Value);
		A11y->SaveSettings();
	}
	UpdateValueTexts();
}

void URok2SettingsWidget::OnHighContrastChanged(bool bChecked)
{
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		A11y->SetHighContrast(bChecked);
		A11y->SaveSettings();
	}
}

void URok2SettingsWidget::OnAudioEnabledChanged(bool bChecked)
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->bAudioEnabled = bChecked;
		// الإيقاف يجب أن يُسكت الموسيقى العاملة، لا أن ينتظر إعادة تشغيلها.
		if (!bChecked)
		{
			Audio->StopMusic();
		}
		else
		{
			Audio->PlayMusic();
		}
	}
}

void URok2SettingsWidget::OnResetClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->SetMusicVolume(1.f);
		Audio->SetSfxVolume(1.f);
		Audio->bAudioEnabled = true;
	}
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		A11y->SetUiScale(1.f);
		A11y->SetHighContrast(false);
		A11y->SaveSettings();
	}
	SyncFromState();
}

void URok2SettingsWidget::OnCloseClicked()
{
	if (URok2Accessibility* A11y = URok2Accessibility::Get())
	{
		// حفظ أخير: أي إعداد لا يمرّ بمعالج (لو أُضيف واحد لاحقاً) يُثبَّت هنا.
		A11y->SaveSettings();
	}
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2SettingsWidget::SyncFromState()
{
	URok2AudioManager* Audio = URok2AudioManager::Get();
	URok2Accessibility* A11y = URok2Accessibility::Get();

	if (MusicSlider && Audio) MusicSlider->SetValue(Audio->MusicVolume);
	if (SfxSlider && Audio) SfxSlider->SetValue(Audio->SfxVolume);
	if (AudioEnabledCheck && Audio) AudioEnabledCheck->SetIsChecked(Audio->bAudioEnabled);
	if (UiScaleSlider && A11y) UiScaleSlider->SetValue(A11y->GetUiScale());
	if (HighContrastCheck && A11y) HighContrastCheck->SetIsChecked(A11y->IsHighContrast());

	UpdateValueTexts();
}

void URok2SettingsWidget::UpdateValueTexts()
{
	URok2AudioManager* Audio = URok2AudioManager::Get();
	URok2Accessibility* A11y = URok2Accessibility::Get();

	if (MusicValueText && Audio)
	{
		MusicValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Audio->MusicVolume * 100.f))));
	}
	if (SfxValueText && Audio)
	{
		SfxValueText->SetText(FText::FromString(
			FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Audio->SfxVolume * 100.f))));
	}
	if (UiScaleValueText && A11y)
	{
		UiScaleValueText->SetText(FText::FromString(
			FString::Printf(TEXT("×%.2f"), A11y->GetUiScale())));
	}
}
