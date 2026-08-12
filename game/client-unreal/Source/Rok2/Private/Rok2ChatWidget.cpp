// Copyright ROK2. P6-T6: دردشة حية — قناتا المملكة والتحالف.
// الودجة تُبنى بالكامل في الكود (لا Blueprint assets) — نفس نمط HUD و AllianceRoster.

#include "Rok2ChatWidget.h"
#include "Rok2Api.h"
#include "Rok2Typography.h"
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
#include "Components/ScrollBox.h"
#include "Components/Image.h"
#include "Components/Spacer.h"

// ألوان الحضارات الست
FLinearColor URok2ChatWidget::GetCivColor(const FString& Civ)
{
	if (Civ == TEXT("rome"))       return FLinearColor(0.9f, 0.2f, 0.2f);
	if (Civ == TEXT("china"))      return FLinearColor(1.0f, 0.85f, 0.0f);
	if (Civ == TEXT("arabia"))     return FLinearColor(0.1f, 0.8f, 0.3f);
	if (Civ == TEXT("egypt"))      return FLinearColor(0.9f, 0.7f, 0.1f);
	if (Civ == TEXT("vikings"))    return FLinearColor(0.4f, 0.6f, 0.9f);
	if (Civ == TEXT("japan"))      return FLinearColor(0.9f, 0.3f, 0.5f);
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

void URok2ChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	if (!WidgetTree->RootWidget)
	{
		BuildWidgetTree();
	}

	// ربط أحداث الدردشة
	if (Api)
	{
		Api->OnChatMessage.AddDynamic(this, &URok2ChatWidget::OnChatReceived);
		// تحميل الرسائل القديمة من الذاكرة
		for (const auto& Msg : Api->GetChatHistory())
		{
			AddMessageBubble(Msg);
		}
	}
}

void URok2ChatWidget::BuildWidgetTree()
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	// اللوحة الرئيسية — شبه شفافة في أسفل اليسار
	UBorder* MainBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MainBorder"));
	MainBorder->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.08f, 0.92f));
	UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(MainBorder);
	BorderSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
	BorderSlot->SetAlignment(FVector2D(0.f, 1.f));
	BorderSlot->SetPosition(FVector2D(10.f, -320.f));
	BorderSlot->SetSize(FVector2D(320.f, 310.f));

	ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
	MainBorder->AddChild(ContentVBox);

	// ---- الشريط العلوي: تبويبات + badge + تصغير ----
	UHorizontalBox* HeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderBox"));
	UVerticalBoxSlot* HeaderSlot = ContentVBox->AddChildToVerticalBox(HeaderBox);
	HeaderSlot->SetPadding(FMargin(6.f, 4.f, 6.f, 2.f));

	KingdomTab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("KingdomTab"));
	KingdomTab->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.2f, 0.6f, 0.9f));
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("المملكة")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		URok2Typography::ApplyFont(T, ERok2TextRole::Button);
		KingdomTab->AddChild(T);
	}
	KingdomTab->OnClicked.AddDynamic(this, &URok2ChatWidget::OnKingdomTabClicked);
	UHorizontalBoxSlot* KTabSlot = HeaderBox->AddChildToHorizontalBox(KingdomTab);
	KTabSlot->SetPadding(FMargin(2.f));
	KTabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	AllianceTab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("AllianceTab"));
	AllianceTab->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f));
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("التحالف")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		URok2Typography::ApplyFont(T, ERok2TextRole::Button);
		AllianceTab->AddChild(T);
	}
	AllianceTab->OnClicked.AddDynamic(this, &URok2ChatWidget::OnAllianceTabClicked);
	UHorizontalBoxSlot* ATabSlot = HeaderBox->AddChildToHorizontalBox(AllianceTab);
	ATabSlot->SetPadding(FMargin(2.f));
	ATabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UnreadBadge = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnreadBadge"));
	UnreadBadge->SetText(FText::FromString(TEXT("")));
	UnreadBadge->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.3f, 0.3f)));
	URok2Typography::ApplyFont(UnreadBadge, ERok2TextRole::Button);
	UHorizontalBoxSlot* BadgeSlot = HeaderBox->AddChildToHorizontalBox(UnreadBadge);
	BadgeSlot->SetPadding(FMargin(4.f, 0.f));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	MinimizeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MinimizeButton"));
	MinimizeButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f));
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("_")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		MinimizeButton->AddChild(T);
	}
	MinimizeButton->OnClicked.AddDynamic(this, &URok2ChatWidget::OnMinimizeClicked);
	URok2MotionLibrary::BindPress(MinimizeButton);
	HeaderBox->AddChildToHorizontalBox(MinimizeButton);

	// ---- صندوق الرسائل ----
	MessageScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MessageScroll"));
	MessageScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
	UVerticalBoxSlot* ScrollSlot = ContentVBox->AddChildToVerticalBox(MessageScroll);
	ScrollSlot->SetPadding(FMargin(4.f, 2.f));
	ScrollSlot->Size.SizeRule = ESlateSizeRule::Fill;

	MessageVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MessageVBox"));
	MessageScroll->AddChild(MessageVBox);

	// ---- شريط الإدخال ----
	InputBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InputBar"));
	UVerticalBoxSlot* InputSlot = ContentVBox->AddChildToVerticalBox(InputBar);
	InputSlot->SetPadding(FMargin(6.f, 4.f, 6.f, 6.f));

	InputField = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InputField"));
	InputField->SetHintText(FText::FromString(TEXT("اكتب رسالة...")));
	InputField->WidgetStyle.BackgroundColor = FSlateColor(FLinearColor(0.15f, 0.15f, 0.2f));
	InputField->OnTextCommitted.AddDynamic(this, &URok2ChatWidget::OnInputTextCommitted);
	UHorizontalBoxSlot* FieldSlot = InputBar->AddChildToHorizontalBox(InputField);
	FieldSlot->SetPadding(FMargin(2.f));
	FieldSlot->Size.SizeRule = ESlateSizeRule::Fill;

	SendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendButton"));
	SendButton->WidgetStyle.Normal.TintColor = FSlateColor(FLinearColor(0.2f, 0.7f, 0.3f));
	{
		// أيقونة سهم إجرائية
		UImage* Arrow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Arrow->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("speedup"), 18.f, FLinearColor::White));
		Arrow->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
		SendButton->AddChild(Arrow);
	}
	SendButton->OnClicked.AddDynamic(this, &URok2ChatWidget::OnSendClicked);
	URok2MotionLibrary::BindPress(SendButton);
	InputBar->AddChildToHorizontalBox(SendButton);
}

// ---- أحداث ----

void URok2ChatWidget::OnSendClicked()
{
	OnInputTextCommitted(FText::GetEmpty(), ETextCommit::OnEnter);
}

void URok2ChatWidget::OnInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter && CommitMethod != ETextCommit::OnUserMovedFocus) return;
	if (!Api) return;

	FString MsgText = Text.ToString().TrimStartAndEnd();
	if (MsgText.IsEmpty()) return;

	Api->SendChat(ActiveChannel, MsgText);
	if (InputField) InputField->SetText(FText::GetEmpty());
}

void URok2ChatWidget::OnKingdomTabClicked()
{
	SwitchChannel(TEXT("kingdom"));
}

void URok2ChatWidget::OnAllianceTabClicked()
{
	SwitchChannel(TEXT("alliance"));
}

void URok2ChatWidget::OnMinimizeClicked()
{
	bMinimized = !bMinimized;
	if (MessageScroll) MessageScroll->SetVisibility(bMinimized ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (InputBar) InputBar->SetVisibility(bMinimized ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (KingdomTab) KingdomTab->SetVisibility(bMinimized ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (AllianceTab) AllianceTab->SetVisibility(bMinimized ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void URok2ChatWidget::SwitchChannel(const FString& NewChannel)
{
	ActiveChannel = NewChannel;
	if (KingdomTab) KingdomTab->WidgetStyle.Normal.TintColor = FSlateColor(
		NewChannel == TEXT("kingdom") ? FLinearColor(0.2f, 0.6f, 0.9f) : FLinearColor(0.4f, 0.4f, 0.4f));
	if (AllianceTab) AllianceTab->WidgetStyle.Normal.TintColor = FSlateColor(
		NewChannel == TEXT("alliance") ? FLinearColor(0.2f, 0.6f, 0.9f) : FLinearColor(0.4f, 0.4f, 0.4f));

	// إعادة بناء القائمة للقناة الجديدة
	if (MessageVBox) MessageVBox->ClearChildren();
	if (Api)
	{
		for (const auto& Msg : Api->GetChatHistory())
		{
			if (Msg.Channel == ActiveChannel)
			{
				AddMessageBubble(Msg);
			}
		}
	}
	UpdateUnreadBadge();
}

// ---- إضافة فقاعة رسالة ----

void URok2ChatWidget::AddMessageBubble(const FRok2ChatMessage& Msg)
{
	if (!MessageVBox || !WidgetTree) return;

	UBorder* Bubble = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Bubble->SetBrushColor(FLinearColor(0.12f, 0.12f, 0.18f, 0.8f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bubble->AddChild(VBox);

	// اسم المرسل بلون الحضارة
	UTextBlock* SenderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SenderText->SetText(FText::FromString(Msg.PlayerName));
	SenderText->SetColorAndOpacity(FSlateColor(GetCivColor(Msg.Civ)));
	URok2Typography::ApplyFont(SenderText, ERok2TextRole::Button);
	UVerticalBoxSlot* SenderSlot = VBox->AddChildToVerticalBox(SenderText);
	SenderSlot->SetPadding(FMargin(8.f, 4.f, 8.f, 0.f));

	// نص الرسالة
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(FText::FromString(Msg.Text));
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f)));
	BodyText->SetAutoWrapText(true);
	URok2Typography::ApplyFont(BodyText, ERok2TextRole::Body);
	UVerticalBoxSlot* BodySlot = VBox->AddChildToVerticalBox(BodyText);
	BodySlot->SetPadding(FMargin(8.f, 2.f, 8.f, 2.f));

	// الطابع الزمني
	UTextBlock* TimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FDateTime MsgTime = FDateTime::FromUnixTimestamp(Msg.TimestampMs / 1000);
	TimeText->SetText(FText::FromString(MsgTime.ToString(TEXT("%H:%M"))));
	TimeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)));
	URok2Typography::ApplyFont(TimeText, ERok2TextRole::Caption);
	UVerticalBoxSlot* TimeSlot = VBox->AddChildToVerticalBox(TimeText);
	TimeSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 4.f));
	TimeSlot->SetHorizontalAlignment(HAlign_Right);

	UVerticalBoxSlot* BubbleSlot = MessageVBox->AddChildToVerticalBox(Bubble);
	BubbleSlot->SetPadding(FMargin(4.f, 3.f, 4.f, 3.f));

	// التمرير للأسفل
	if (MessageScroll) MessageScroll->ScrollToEnd();
}

// ---- استقبال رسالة جديدة ----

void URok2ChatWidget::OnChatReceived(const FRok2ChatMessage& Msg)
{
	if (Msg.Channel == ActiveChannel)
	{
		AddMessageBubble(Msg);
	}
	UpdateUnreadBadge();
}

void URok2ChatWidget::UpdateUnreadBadge()
{
	if (!UnreadBadge || !Api) return;
	int32 Count = Api->GetUnreadChatCount();
	if (Count > 0)
	{
		UnreadBadge->SetText(FText::FromString(FString::Printf(TEXT("%d"), FMath::Min(Count, 99))));
	}
	else
	{
		UnreadBadge->SetText(FText::GetEmpty());
	}
}
