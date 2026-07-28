#include "Rok2BootWidget.h"
#include "Rok2Api.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"

void URok2BootWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnLoginComplete.AddDynamic(this, &URok2BootWidget::OnLoginComplete);
	Api->OnPlayerLoaded.AddDynamic(this, &URok2BootWidget::OnPlayerLoaded);
	Api->OnApiError.AddDynamic(this, &URok2BootWidget::OnApiError);
	Api->OnConnectionState.AddDynamic(this, &URok2BootWidget::OnConnectionState);

	if (EnterButton) EnterButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnEnterClicked);
	if (StartButton) StartButton->OnClicked.AddDynamic(this, &URok2BootWidget::OnStartClicked);

	// Populate civ combo
	if (CivCombo)
	{
		CivCombo->ClearOptions();
		for (const FRok2Civilization& C : Api->GetCivilizations())
		{
			CivCombo->AddOption(FString::Printf(TEXT("%s|%s"), *C.Name, *C.Id));
		}
		CivCombo->SetSelectedIndex(0);
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

		// Title
		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("👑 ROK2 : RISE OF KINGDOMS 2")));
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.2f)));
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 22;
		TitleText->SetFont(TitleFont);
		UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0, 15, 0, 5));

		// Subtitle
		UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		SubtitleText->SetText(FText::FromString(TEXT("مرحباً بك يا حاكم — اختر اسمك وحضارتك لإنشاء المملكة")));
		SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.85f, 0.9f)));
		UVerticalBoxSlot* SubSlot = VBox->AddChildToVerticalBox(SubtitleText);
		SubSlot->SetHorizontalAlignment(HAlign_Center);
		SubSlot->SetPadding(FMargin(0, 0, 0, 20));

		// Enter Button (Guest login)
		EnterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EnterButton"));
		UTextBlock* EnterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnterText"));
		EnterText->SetText(FText::FromString(TEXT("⚡ دخول سريع كضيف (Quick Guest Login)")));
		EnterText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo BtnFont = EnterText->GetFont();
		BtnFont.Size = 15;
		EnterText->SetFont(BtnFont);
		EnterButton->AddChild(EnterText);

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
		CivSlot->SetPadding(FMargin(30, 5, 30, 15));

		// Start Journey Button
		StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		UTextBlock* StartText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartText"));
		StartText->SetText(FText::FromString(TEXT("⚔️ ابدأ رحلة التوسع والمجد (Start Journey)")));
		StartText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		StartText->SetFont(BtnFont);
		StartButton->AddChild(StartText);

		UVerticalBoxSlot* StartBtnSlot = VBox->AddChildToVerticalBox(StartButton);
		StartBtnSlot->SetPadding(FMargin(30, 5, 30, 15));

		// --- شاشة التحميل: لوحة سفلية بنص متحرك ---
		LoadingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoadingPanel"));
		LoadingPanel->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
		LoadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingText"));
		LoadingText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.0f)));
		FSlateFontInfo LoadFont = LoadingText->GetFont();
		LoadFont.Size = 14;
		LoadingText->SetFont(LoadFont);
		LoadingText->SetJustification(ETextJustify::Center);
		LoadingPanel->SetContent(LoadingText);
		LoadingPanel->SetPadding(FMargin(0, 10, 0, 10));
		UVerticalBoxSlot* LoadSlot = VBox->AddChildToVerticalBox(LoadingPanel);
		LoadSlot->SetPadding(FMargin(30, 5, 30, 5));

		// --- نص حالة الاتصال (أخطاء/إعادة محاولة) ---
		StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.55f, 0.4f)));
		FSlateFontInfo StatusFont = StatusText->GetFont();
		StatusFont.Size = 12;
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
	}
}

void URok2BootWidget::OnStartClicked()
{
	if (!Api) return;
	FString Civ = TEXT("rome");
	if (CivCombo)
	{
		FString Sel = CivCombo->GetSelectedOption();
		FString Left, Right;
		if (Sel.Split(TEXT("|"), &Left, &Right)) Civ = Right;
		else Civ = Sel;
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
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("⚠ %s"), *Message)));
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
