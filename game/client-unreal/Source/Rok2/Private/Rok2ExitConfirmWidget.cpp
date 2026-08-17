// Copyright ROK2. لوحة تأكيد الخروج (P18-T5) — implementation.

#include "Rok2ExitConfirmWidget.h"
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
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

TSharedRef<SWidget> URok2ExitConfirmWidget::RebuildWidget()
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

void URok2ExitConfirmWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ExitConfirmRoot"));
	WidgetTree->RootWidget = RootPanel;

	// حجاب يمنع لمس ما تحته. لمسه = إلغاء، كبقية اللوحات في المشروع — فلا
	// يُحتجز اللاعب داخل سؤال.
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitConfirmBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2ExitConfirmWidget::OnCancelClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	// نافذة مركزية لا ورقة سفلية: هذا سؤال يوقف كل شيء، لا لوحة محتوى.
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ExitConfirmCard"));
	Card->SetBrush(Rok2Surface::Panel());
	Card->SetPadding(FMargin(Rok2Space::XL));
	UCanvasPanelSlot* CardSlot = RootPanel->AddChildToCanvas(Card);
	CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CardSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Card->SetContent(Column);

	{
		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(TitleRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("crown"), 22.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(22.f, 22.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("crown")));
		UHorizontalBoxSlot* IcoSlot = TitleRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("مغادرة المملكة؟")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
		TitleRow->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);
	}

	{
		UTextBlock* Body = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		// الطوابير والمسيرات تعمل على الخادم لا في العميل — والصدق هنا يمنع
		// خوفاً لا سبب له من إغلاق التطبيق.
		Body->SetText(FText::FromString(TEXT("مملكتك تبقى قائمة على الخادم: البناء والتدريب والمسيرات تكمل في غيابك.")));
		Body->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		Body->SetAutoWrapText(true);
		URok2Typography::ApplyFont(Body, ERok2TextRole::Body);
		Column->AddChildToVerticalBox(Body)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::L, Rok2Space::None));
	}

	{
		UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Actions);

		// «البقاء» أولاً: في صفٍّ عربي (RTL) يقع في اليمين — موضع الفعل الآمن.
		UButton* Stay = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitConfirmStay"));
		Stay->SetStyle(Rok2Surface::PrimaryButton());
		Stay->OnClicked.AddDynamic(this, &URok2ExitConfirmWidget::OnCancelClicked);
		URok2MotionLibrary::BindPress(Stay);
		UTextBlock* StayText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		StayText->SetText(FText::FromString(TEXT("البقاء")));
		StayText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(StayText, ERok2TextRole::Button);
		Stay->AddChild(StayText);
		UHorizontalBoxSlot* StaySlot = Actions->AddChildToHorizontalBox(Stay);
		StaySlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
		StaySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UButton* Quit = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ExitConfirmQuit"));
		Quit->SetStyle(Rok2Surface::DangerButton());
		Quit->OnClicked.AddDynamic(this, &URok2ExitConfirmWidget::OnConfirmClicked);
		URok2MotionLibrary::BindPress(Quit);
		UTextBlock* QuitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		QuitText->SetText(FText::FromString(TEXT("خروج")));
		QuitText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(QuitText, ERok2TextRole::Button);
		Quit->AddChild(QuitText);
		UHorizontalBoxSlot* QuitSlot = Actions->AddChildToHorizontalBox(Quit);
		QuitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	URok2MotionLibrary::PlayScaleInCenter(Card);
	URok2MotionLibrary::PlayFadeIn(Backdrop);
}

void URok2ExitConfirmWidget::OnConfirmClicked()
{
	// `Quit` بدل `RequestExit`: يمرّ بمسار المحرك المعتاد فيُنهي PIE في المحرر
	// ويغلق التطبيق على الجهاز، بلا سلوك مختلف بين البيئتين.
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(GetWorld(), 0),
		EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
}

void URok2ExitConfirmWidget::OnCancelClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	URok2MotionLibrary::PlayFadeOut(this);
}
