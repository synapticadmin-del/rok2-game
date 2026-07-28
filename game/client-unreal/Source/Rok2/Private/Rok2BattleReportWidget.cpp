// Copyright ROK2. Battle Report Widget impl — P1-T4.

#include "Rok2BattleReportWidget.h"
#include "Rok2Api.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"

static FLinearColor Rok2Gold() { return FLinearColor(1.0f, 0.84f, 0.2f); }
static FLinearColor Rok2Panel() { return FLinearColor(0.04f, 0.07f, 0.14f, 0.95f); }

void URok2BattleReportWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnBattleReports.AddDynamic(this, &URok2BattleReportWidget::OnBattleReports);
	RebuildList(Api->GetBattleReports());

	// اطلب أحدث البيانات لو القائمة فاضية
	if (Api->GetBattleReports().Num() == 0)
	{
		Api->RefreshWorld();
	}
}

void URok2BattleReportWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		// خلفية شبه شفافة تغطي الشاشة
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
		UCanvasPanelSlot* BdSlot = RootCanvas->AddChildToCanvas(Backdrop);
		BdSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));

		// البطاقة الرئيسية في المنتصف
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
		Card->SetBrushColor(Rok2Panel());
		UCanvasPanelSlot* CardSlot = RootCanvas->AddChildToCanvas(Card);
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetSize(FVector2D(760.f, 520.f));

		UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
		Card->SetContent(MainVBox);

		// شريط العنوان + زر الإغلاق
		UHorizontalBox* TitleHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TitleHBox"));
		MainVBox->AddChildToVerticalBox(TitleHBox)->SetPadding(FMargin(14, 12, 14, 8));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
		Title->SetText(FText::FromString(TEXT("📜 تقارير القتال (Battle Reports)")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Gold()));
		FSlateFontInfo TitleFont = Title->GetFont();
		TitleFont.Size = 18;
		Title->SetFont(TitleFont);
		UHorizontalBoxSlot* TitleSlot = TitleHBox->AddChildToHorizontalBox(Title);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);

		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
		CloseText->SetText(FText::FromString(TEXT("✖ إغلاق")));
		CloseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CloseButton->AddChild(CloseText);
		CloseButton->OnClicked.AddDynamic(this, &URok2BattleReportWidget::OnCloseClicked);
		TitleHBox->AddChildToHorizontalBox(CloseButton)->SetVerticalAlignment(VAlign_Center);

		// جسم اللوحة: قائمة التقارير + التفاصيل جنباً إلى جنب
		UHorizontalBox* BodyHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyHBox"));
		UVerticalBoxSlot* BodySlot = MainVBox->AddChildToVerticalBox(BodyHBox);
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BodySlot->SetPadding(FMargin(14, 0, 14, 14));

		// قائمة التقارير (يمين)
		UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ListScroll"));
		UHorizontalBoxSlot* ListSlot = BodyHBox->AddChildToHorizontalBox(ListScroll);
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListSlot->SetPadding(FMargin(0, 0, 8, 0));

		ReportList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ReportList"));
		ListScroll->AddChild(ReportList);

		// لوحة التفاصيل (شمال)
		UBorder* DetailBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBorder"));
		DetailBorder->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.10f, 1.0f));
		UHorizontalBoxSlot* DetailSlot = BodyHBox->AddChildToHorizontalBox(DetailBorder);
		DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UScrollBox* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DetailScroll"));
		DetailBorder->SetContent(DetailScroll);

		DetailPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailPanel"));
		DetailScroll->AddChild(DetailPanel);

		UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
		Hint->SetText(FText::FromString(TEXT("اختر تقريراً من القائمة لعرض تفاصيل الخسائر")));
		Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.65f, 0.7f)));
		DetailPanel->AddChildToVerticalBox(Hint)->SetPadding(FMargin(12, 12, 12, 0));
	}
}

void URok2BattleReportWidget::OnBattleReports(const TArray<FRok2BattleReport>& Reports)
{
	RebuildList(Reports);
}

void URok2BattleReportWidget::RebuildList(const TArray<FRok2BattleReport>& Reports)
{
	Current = Reports;
	if (!ReportList) return;

	ReportList->ClearChildren();

	if (Reports.Num() == 0)
	{
		UTextBlock* Empty = NewObject<UTextBlock>(this);
		Empty->SetText(FText::FromString(TEXT("لا توجد تقارير قتال بعد — هاجم ممراً أو معسكر برابرة لتبدأ سجل المعارك")));
		Empty->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.65f, 0.7f)));
		Empty->SetAutoWrapText(true);
		ReportList->AddChildToVerticalBox(Empty)->SetPadding(FMargin(6, 6, 6, 6));
		return;
	}

	const FString MyId = Api ? Api->GetPlayer().Id : FString();

	for (int32 i = 0; i < Reports.Num(); ++i)
	{
		const FRok2BattleReport& R = Reports[i];

		// تحديد النتيجة من منظور اللاعب
		const bool bMine = (R.AttackerPlayerId == MyId);
		FString ResultIcon;
		FLinearColor ResultColor;
		if (R.Winner == TEXT("draw"))
		{
			ResultIcon = TEXT("🤝");
			ResultColor = FLinearColor(0.9f, 0.8f, 0.3f);
		}
		else if ((R.Winner == TEXT("attacker")) == bMine)
		{
			ResultIcon = TEXT("🏆");
			ResultColor = FLinearColor(0.3f, 0.9f, 0.4f);
		}
		else
		{
			ResultIcon = TEXT("💀");
			ResultColor = FLinearColor(0.95f, 0.35f, 0.3f);
		}

		const FDateTime Dt = FDateTime::FromUnixTimestamp(R.CreatedAt / 1000);
		const FString Label = FString::Printf(TEXT("%s %s · %s\n%02d:%02d"),
			*ResultIcon, *KindLabel(R.Kind), bMine ? TEXT("هجومك") : TEXT("معركة"),
			Dt.GetHour(), Dt.GetMinute());

		UButton* Row = NewObject<UButton>(this);
		UTextBlock* RowText = NewObject<UTextBlock>(this);
		RowText->SetText(FText::FromString(Label));
		RowText->SetColorAndOpacity(FSlateColor(ResultColor));
		RowText->SetAutoWrapText(true);
		Row->AddChild(RowText);

		URok2ReportRowHandler* Handler = NewObject<URok2ReportRowHandler>(this);
		Handler->Index = i;
		Handler->Widget = this;
		Row->OnClicked.AddDynamic(Handler, &URok2ReportRowHandler::OnClick);

		ReportList->AddChildToVerticalBox(Row)->SetPadding(FMargin(4, 3, 4, 3));
	}
}

void URok2BattleReportWidget::ShowReport(const FRok2BattleReport& R)
{
	if (!DetailPanel) return;
	DetailPanel->ClearChildren();

	auto AddLine = [this](const FString& Text, const FLinearColor& Color, int32 Size = 13, const FMargin& Pad = FMargin(12, 4, 12, 0))
	{
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(Text));
		T->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		T->SetAutoWrapText(true);
		DetailPanel->AddChildToVerticalBox(T)->SetPadding(Pad);
	};

	// العنوان
	const bool bAttackerWon = (R.Winner == TEXT("attacker"));
	const FString Headline = R.Winner == TEXT("draw")
		? TEXT("🤝 تعادل — انسحاب الطرفين")
		: (bAttackerWon ? TEXT("🏆 انتصار المهاجم") : TEXT("🛡️ صمود المدافع"));
	AddLine(FString::Printf(TEXT("%s\n%s"), *Headline, *KindLabel(R.Kind)), Rok2Gold(), 17, FMargin(12, 12, 12, 4));

	// القوة قبل المعركة
	AddLine(FString::Printf(TEXT("⚔️ القوة قبل المعركة — مهاجم: %d · مدافع: %d"),
		R.Attacker.PowerBefore, R.Defender.PowerBefore),
		FLinearColor(0.8f, 0.85f, 0.9f), 13, FMargin(12, 8, 12, 0));

	// المهاجم
	AddLine(TEXT("🔴 المهاجم"), FLinearColor(0.95f, 0.45f, 0.4f), 15, FMargin(12, 14, 12, 2));
	AddLine(SummarizeSide(R.Attacker), FLinearColor::White);

	// المدافع
	AddLine(TEXT("🔵 المدافع"), FLinearColor(0.45f, 0.65f, 1.0f), 15, FMargin(12, 14, 12, 2));
	AddLine(SummarizeSide(R.Defender), FLinearColor::White);

	// ملاحظة المستشفى
	int32 SevTotal = 0;
	for (const FRok2TroopLoss& L : R.Attacker.Severely) SevTotal += L.Count;
	if (SevTotal > 0)
	{
		AddLine(FString::Printf(TEXT("🏥 %d جريح خطير يحتاجون مستشفى للشفاء"), SevTotal),
			FLinearColor(0.4f, 0.9f, 0.7f), 12, FMargin(12, 16, 12, 12));
	}
}

FString URok2BattleReportWidget::SummarizeSide(const FRok2BattleSide& Side)
{
	if (Side.Losses.Num() == 0 && Side.Remaining.Num() == 0)
	{
		return TEXT("لا توجد بيانات وحدات");
	}

	auto TotalOf = [](const TArray<FRok2TroopLoss>& Arr)
	{
		int32 T = 0;
		for (const FRok2TroopLoss& L : Arr) T += L.Count;
		return T;
	};

	const int32 TotalLoss = TotalOf(Side.Losses);
	const int32 Dead = TotalOf(Side.Dead);
	const int32 Sev = TotalOf(Side.Severely);
	const int32 Slight = TotalOf(Side.Slightly);
	const int32 Remaining = TotalOf(Side.Remaining);

	// تفصيل الوحدات
	FString Units;
	for (const FRok2TroopLoss& L : Side.Losses)
	{
		if (Units.Len() > 0) Units += TEXT("، ");
		Units += FString::Printf(TEXT("%s -%d"), *L.UnitId, L.Count);
	}
	if (Units.IsEmpty()) Units = TEXT("بدون خسائر");

	return FString::Printf(
		TEXT("الخسائر: %d (💀 %d · 🩸 خطير %d · 🤕 خفيف %d)\nالمتبقي: %d\n%s"),
		TotalLoss, Dead, Sev, Slight, Remaining, *Units);
}

FString URok2BattleReportWidget::KindLabel(const FString& Kind)
{
	if (Kind == TEXT("pass_attack")) return TEXT("هجوم على ممر جبلي");
	if (Kind == TEXT("throne_attack")) return TEXT("هجوم على العرش");
	if (Kind == TEXT("barb")) return TEXT("معسكر برابرة");
	if (Kind == TEXT("city_attack")) return TEXT("هجوم على مدينة");
	return Kind.IsEmpty() ? TEXT("معركة") : Kind;
}

void URok2BattleReportWidget::Close()
{
	RemoveFromParent();
}

void URok2BattleReportWidget::OnCloseClicked()
{
	Close();
}

void URok2ReportRowHandler::OnClick()
{
	if (Widget && Widget->Current.IsValidIndex(Index))
	{
		Widget->ShowReport(Widget->Current[Index]);
	}
}
