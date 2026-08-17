// Copyright ROK2. P6-T6: دردشة حية — قناتا المملكة والتحالف.
// الودجة تُبنى بالكامل في الكود (لا Blueprint assets) — نفس نمط HUD و AllianceRoster.

#include "Rok2ChatWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
#include "Rok2AudioManager.h"
#include "Rok2Surface.h"
#include "Rok2Typography.h"
#include "Rok2VisualTheme.h"
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

// لون الحضارة من رمز المشروع المشترك. كانت هنا لوحة سابعة بقيم لا تطابق
// Rok2Visual::CivilizationAccent، فيظهر اسم اللاعب في الدردشة بلون مختلف عن
// لون حضارته في كل شاشة أخرى.
FLinearColor URok2ChatWidget::GetCivColor(const FString& Civ)
{
	return Rok2Visual::CivilizationAccent(Civ);
}


TSharedRef<SWidget> URok2ChatWidget::RebuildWidget()
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
	MainBorder->SetBrush(Rok2Surface::Panel());
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
	KingdomTab->SetStyle(Rok2Surface::TabButton(true));
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
	AllianceTab->SetStyle(Rok2Surface::TabButton(false));
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
	UnreadBadge->SetColorAndOpacity(FSlateColor(Rok2Visual::DangerText()));
	URok2Typography::ApplyFont(UnreadBadge, ERok2TextRole::Button);
	UHorizontalBoxSlot* BadgeSlot = HeaderBox->AddChildToHorizontalBox(UnreadBadge);
	BadgeSlot->SetPadding(FMargin(Rok2Space::XS, Rok2Space::None));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	MinimizeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MinimizeButton"));
	MinimizeButton->SetStyle(Rok2Surface::SecondaryButton());
	MinimizeButton->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("minimize")));
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("_")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		MinimizeButton->AddChild(T);
	}
	MinimizeButton->OnClicked.AddDynamic(this, &URok2ChatWidget::OnMinimizeClicked);
	URok2MotionLibrary::BindPress(MinimizeButton);
	HeaderBox->AddChildToHorizontalBox(MinimizeButton);

	// P18-T5: زر إغلاق إلى جانب التصغير. التصغير يطوي الرسائل والإدخال
	// والتبويبات فيبقى شريط الترويسة معلقاً على الشاشة بلا مسار إزالة.
	{
		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ChatCloseButton"));
		CloseButton->SetStyle(Rok2Surface::SecondaryButton());
		CloseButton->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		UImage* CloseIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		CloseIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("close"), 14.f, Rok2Visual::Muted()));
		CloseIco->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		CloseIco->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		CloseButton->AddChild(CloseIco);
		CloseButton->OnClicked.AddDynamic(this, &URok2ChatWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(CloseButton);
		HeaderBox->AddChildToHorizontalBox(CloseButton);
	}

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
	InputField->WidgetStyle.SetBackgroundImageNormal(Rok2Surface::Card());
	InputField->WidgetStyle.SetForegroundColor(Rok2Visual::Ivory());
	InputField->OnTextCommitted.AddDynamic(this, &URok2ChatWidget::OnInputTextCommitted);
	UHorizontalBoxSlot* FieldSlot = InputBar->AddChildToHorizontalBox(InputField);
	FieldSlot->SetPadding(FMargin(2.f));
	FieldSlot->Size.SizeRule = ESlateSizeRule::Fill;

	SendButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SendButton"));
	SendButton->SetStyle(Rok2Surface::SuccessButton());
	SendButton->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("send")));
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

void URok2ChatWidget::CloseSelf()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	// التصغير حالة لا إغلاق؛ الودجة تُعاد للمنفذ من GameMode عند الفتح التالي،
	// فنُعيد الحالة المطويّة إلى وضعها الطبيعي وإلا فُتحت في المرة القادمة
	// بشريط ترويسة وحده.
	if (bMinimized)
	{
		OnMinimizeClicked();
	}
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2ChatWidget::OnCloseClicked()
{
	CloseSelf();
}

void URok2ChatWidget::SwitchChannel(const FString& NewChannel)
{
	ActiveChannel = NewChannel;
	// النمط الكامل (أربع حالات) لا لون Normal وحده — التبويب الخامل كان يفقد
	// رد فعل التحويم والضغط لأن ضبط TintColor يمسّ الحالة العادية فقط.
	if (KingdomTab) KingdomTab->SetStyle(Rok2Surface::TabButton(NewChannel == TEXT("kingdom")));
	if (AllianceTab) AllianceTab->SetStyle(Rok2Surface::TabButton(NewChannel == TEXT("alliance")));

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
	Bubble->SetBrush(Rok2Surface::Card());

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bubble->AddChild(VBox);

	// اسم المرسل بلون الحضارة + وسام النص يُكتب بجانبه — لا اعتماد على اللون فقط
	UTextBlock* SenderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SenderText->SetText(FText::FromString(Msg.PlayerName));
	SenderText->SetColorAndOpacity(FSlateColor(URok2Accessibility::Get()->AccessibleTextFor(GetCivColor(Msg.Civ))));
	URok2Typography::ApplyFont(SenderText, ERok2TextRole::Button);
	SenderText->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("chat_sender")));
	UVerticalBoxSlot* SenderSlot = VBox->AddChildToVerticalBox(SenderText);
	SenderSlot->SetPadding(FMargin(8.f, 4.f, 8.f, 0.f));

	// نص الرسالة
	UTextBlock* BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	BodyText->SetText(FText::FromString(Msg.Text));
	BodyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	BodyText->SetAutoWrapText(true);
	URok2Typography::ApplyFont(BodyText, ERok2TextRole::Body);
	UVerticalBoxSlot* BodySlot = VBox->AddChildToVerticalBox(BodyText);
	BodySlot->SetPadding(FMargin(8.f, 2.f, 8.f, 2.f));

	// الطابع الزمني
	UTextBlock* TimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FDateTime MsgTime = FDateTime::FromUnixTimestamp(Msg.TimestampMs / 1000);
	TimeText->SetText(FText::FromString(MsgTime.ToString(TEXT("%H:%M"))));
	TimeText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
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
		UnreadBadge->SetText(FText::FromString(FString::Printf(TEXT("(%d)"), FMath::Min(Count, 99))));
	}
	else
	{
		UnreadBadge->SetText(FText::GetEmpty());
	}
}
