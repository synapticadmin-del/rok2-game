// Copyright ROK2. First-minute onboarding — guidance overlay (P6-T4).

#include "Rok2OnboardingWidget.h"
#include "Rok2Api.h"
#include "Rok2Typography.h"
#include "Rok2ArtAssets.h"
#include "Rok2MotionLibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2FtueUI, Log, All);

// ألوان الهوية من ui-ux-design-system.md §1. محلية للملف على اصطلاح
// Rok2HudStyle/Rok2CardStyle — الألوان بقيت مسؤولية كل ودجة عن قصد في P6-T2،
// فلا أُمركزها هنا استطراداً.
namespace Rok2FtueStyle
{
	static const FLinearColor PanelBg(0.10f, 0.07f, 0.04f, 0.94f);  // #1A120B
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f);             // #C9A227
	static const FLinearColor Ivory(0.96f, 0.91f, 0.81f);            // #F5E9D0
	static const FLinearColor Muted(0.72f, 0.68f, 0.60f, 0.95f);

	/** عرض البطاقة — تكفي لسطرَي حكاية عربية دون أن تزحم عرض الهاتف */
	static constexpr float CardWidth = 340.f;

	/** الإزاحة من أعلى اليسار: 16 هامش، و58 = 46 (الشريط العلوي) + 12 تنفّس */
	static constexpr float CardLeft = 16.f;
	static constexpr float CardTop = 58.f;

	/** مدة بقاء بطاقة التتويج قبل تلاشي الطبقة */
	static constexpr float CelebrationHoldSeconds = 4.2f;
}

void URok2OnboardingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FtueRoot"));
	WidgetTree->RootWidget = RootCanvas;

	// الطبقة كلها غير حاجبة: تُرسم فوق الشاشة ولا تلتقط لمسة. لو التقطت،
	// لصار الزر الذي تُبرزه غير قابل للضغط — أي درسٌ يمنع تنفيذ نفسه.
	SetVisibility(ESlateVisibility::HitTestInvisible);

	BuildCard();
}

void URok2OnboardingWidget::BuildCard()
{
	if (!RootCanvas) return;

	Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FtueCard"));
	Card->SetBrushColor(Rok2FtueStyle::PanelBg);
	Card->SetPadding(FMargin(12.f, 10.f, 12.f, 12.f));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(Card);
	// أعلى اليسار تحت الشريط — الجهة الحرة الوحيدة، وهي «اليسار — المهام» في §3.5.
	CardSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
	CardSlot->SetAlignment(FVector2D(0.f, 0.f));
	CardSlot->SetPosition(FVector2D(Rok2FtueStyle::CardLeft, Rok2FtueStyle::CardTop));
	// AutoSize مع SizeBox داخلي بدل SetSize: على شريحة الكانفس، AutoSize يُلغي
	// SetSize تماماً — فعرضٌ مضبوط بـSetSize مع AutoSize=true وهمٌ لا أثر له.
	// والعرض المحدود ليس تجميلاً: SetAutoWrapText لا يلتفّ بلا حدٍّ أفقي، فبدون
	// SizeBox كانت الحكاية العربية تمتدّ سطراً واحداً يخرج من الشاشة.
	CardSlot->SetAutoSize(true);

	USizeBox* Bounds = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FtueBounds"));
	Bounds->SetWidthOverride(Rok2FtueStyle::CardWidth);
	Card->SetContent(Bounds);

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bounds->SetContent(VBox);

	// --- الترويسة: أيقونة الخطوة + لافتة «1 من 3» ---
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	VBox->AddChildToVerticalBox(HeaderRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	StepIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FtueIcon"));
	StepIcon->SetDesiredSizeOverride(FVector2D(28.f, 28.f));
	HeaderRow->AddChildToHorizontalBox(StepIcon)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	OrdinalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FtueOrdinal"));
	OrdinalText->SetColorAndOpacity(FSlateColor(Rok2FtueStyle::Muted));
	// Caption لا Numeric: اللافتة تخلط رقماً بكلمة «من»، والوجه الرقمي (Cinzel)
	// بلا محارف عربية فكانت الكلمة ستسقط إلى وجه آخر بصمت.
	URok2Typography::ApplyFont(OrdinalText, ERok2TextRole::Caption);
	HeaderRow->AddChildToHorizontalBox(OrdinalText);

	HeaderRow->AddChildToHorizontalBox(
		WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass())
	)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// --- العنوان الأدبي ---
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FtueTitle"));
	TitleText->SetColorAndOpacity(FSlateColor(Rok2FtueStyle::Gold));
	URok2Typography::ApplyFont(TitleText, ERok2TextRole::Title);
	TitleText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	// --- سطر الحكاية ---
	StoryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FtueStory"));
	StoryText->SetColorAndOpacity(FSlateColor(Rok2FtueStyle::Ivory));
	URok2Typography::ApplyFont(StoryText, ERok2TextRole::Body);
	StoryText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(StoryText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	// --- الإجراء الملموس (وزن بصري أخفّ من الحكاية) ---
	ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FtueAction"));
	ActionText->SetColorAndOpacity(FSlateColor(Rok2FtueStyle::Gold));
	URok2Typography::ApplyFont(ActionText, ERok2TextRole::Micro);
	ActionText->SetAutoWrapText(true);
	VBox->AddChildToVerticalBox(ActionText);

	// مخفية حتى يقرّر النموذج أن هناك ما يُرشد إليه
	Card->SetRenderOpacity(0.f);
	Card->SetVisibility(ESlateVisibility::Collapsed);
}

void URok2OnboardingWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	// التقدّم يُلتقط من المفوَّضات القائمة لا من Tick: OnCityLoaded يحمل
	// المباني والطوابير، وOnWorldSnapshot يُبثّ من UpsertMarch لحظة إنشاء
	// المسيرة — فلا سباق مع انتهائها داخل الجلسة.
	Api->OnCityLoaded.AddDynamic(this, &URok2OnboardingWidget::OnCityLoadedHandler);
	Api->OnWorldSnapshot.AddDynamic(this, &URok2OnboardingWidget::OnWorldSnapshotHandler);

	Refresh();
}

void URok2OnboardingWidget::OnCityLoadedHandler(const FRok2City& City)
{
	Refresh();
}

void URok2OnboardingWidget::OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot)
{
	Refresh();
}

void URok2OnboardingWidget::ApplyStepInfo(const FRok2FtueStepInfo& Info)
{
	if (StepIcon && URok2ArtAssets::HasIcon(Info.IconId))
	{
		StepIcon->SetBrush(URok2ArtAssets::GetIconBrush(Info.IconId, 28.f, Rok2FtueStyle::Gold));
	}

	if (OrdinalText)
	{
		// أرقام غربية كبقية أرقام اللعبة (الموارد والمؤقّتات) — رقمٌ هندي
		// عربي وحده هنا كان سيصير الاستثناء الوحيد في الواجهة كلها.
		OrdinalText->SetText(FText::FromString(FString::Printf(
			TEXT("%d من %d"), Info.Ordinal, Rok2FtueSpec::GuidedStepCount)));
	}

	if (TitleText)  TitleText->SetText(FText::FromString(Info.Title));
	if (StoryText)  StoryText->SetText(FText::FromString(Info.Story));
	if (ActionText) ActionText->SetText(FText::FromString(Info.Action));
}

void URok2OnboardingWidget::Refresh()
{
	URok2Onboarding* Model = URok2Onboarding::Get();
	if (!Model || !Card) return;

	const ERok2FtueStep Step = Model->Evaluate(Api);

	// لاعب عائد: لا شيء يُبنى ولا يُعرض أبداً.
	if (!Model->IsArmed())
	{
		Card->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// لا شيء تغيّر — نخرج بلا إعادة رسم ولا إعادة حركة. المفوَّضان يُبثّان
	// مع كل نبضة شبكة، وبطاقة تُعيد الانزلاق كل نبضة تصير وميضاً مزعجاً.
	if (Step == LastRenderedStep)
	{
		return;
	}

	const bool bFirstShow = (LastRenderedStep == ERok2FtueStep::None);
	LastRenderedStep = Step;

	// --- التتويج: الدقيقة الأولى تمّت ---
	if (Step == ERok2FtueStep::Done)
	{
		if (bCelebrated)
		{
			Card->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		bCelebrated = true;

		ApplyStepInfo(URok2Onboarding::CompletionInfo());
		Card->SetVisibility(ESlateVisibility::HitTestInvisible);
		Card->SetRenderOpacity(1.f);

		// theme-and-values §5: لحظة تتويج لا إخفاء صامت — ووثيقة UI §1
		// «كل تأكيد له وميض ذهبي».
		URok2MotionLibrary::PlayGoldFlash(Card, Rok2FtueStyle::PanelBg);

		if (UWorld* W = GetWorld())
		{
			FTimerHandle Handle;
			TWeakObjectPtr<URok2OnboardingWidget> WeakThis(this);
			W->GetTimerManager().SetTimer(Handle, [WeakThis]()
			{
				if (URok2OnboardingWidget* Self = WeakThis.Get())
				{
					if (Self->Card)
					{
						URok2MotionLibrary::PlayFadeOut(Self->Card);
					}
				}
			}, Rok2FtueStyle::CelebrationHoldSeconds, false);
		}

		UE_LOG(LogRok2FtueUI, Log, TEXT("FTUE completed — celebration shown"));
		return;
	}

	// --- خطوة مرشدة ---
	if (Step == ERok2FtueStep::None)
	{
		Card->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ApplyStepInfo(URok2Onboarding::StepInfo(Step));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (bFirstShow)
	{
		// أول ظهور: انزلاق من الأسفل كبقية اللوحات (§1 «لا قفزات جامدة»).
		Card->SetRenderOpacity(0.f);
		URok2MotionLibrary::PlaySlideInBottom(Card);
	}
	else
	{
		// خطوة تقدّمت: وميض ذهبي يقول «تمّ» ثم النص الجديد — أخفّ من إعادة
		// انزلاق البطاقة كاملة، فالبطاقة لم تذهب ولم تعد.
		Card->SetRenderOpacity(1.f);
		URok2MotionLibrary::PlayGoldFlash(Card, Rok2FtueStyle::PanelBg);
	}

	UE_LOG(LogRok2FtueUI, Verbose, TEXT("FTUE step rendered: %d"), (int32)Step);
}
