// Copyright ROK2. Battle Report Widget impl — P1-T4.
// P6-T1: أيقونات النتائج والحالات إجرائية من URok2ArtAssets (بدل الإيموجي).
// P6-T3: البطاقة تفتح من المركز + خلفية تتلاشى + ضغطات محسوسة (URok2MotionLibrary).

#include "Rok2BattleReportWidget.h"
#include "Rok2Typography.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Rok2MotionLibrary.h"
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
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"

static FLinearColor Rok2Gold() { return FLinearColor(1.0f, 0.84f, 0.2f); }
// P6-T7: ألوان اللوحة تختلف حسب الحضارة
static FLinearColor Rok2Panel(const FString& Civ = TEXT(""))
{
	const FRok2CivTheme& Theme = URok2CivThemes::Get()->GetTheme(Civ);
	return Theme.PanelBg;
}

void URok2BattleReportWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnBattleReports.AddDynamic(this, &URok2BattleReportWidget::OnBattleReports);
	RebuildList(Api->GetBattleReports());

	// السجل خاص بكل لاعب/تحالف، لذلك يُقرأ دائماً من الاستعلام السلطوي عند فتح النافذة.
	Api->FetchBattleReports();
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

		// البطاقة الرئيسية في المنتصف — P6-T7: بلون الحضارة
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
		FString Civ;
		if (Api) Civ = Api->GetPlayer().Civ;
		Card->SetBrushColor(Rok2Panel(Civ));
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
		Title->SetText(FText::FromString(TEXT("تقارير القتال (Battle Reports)")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Gold()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);

		// P6-T1: أيقونة مخطوط إجرائية في ترويسة اللوحة
		UImage* TitleIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleIcon"));
		TitleIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("scroll"), 20.f, Rok2Gold()));
		TitleIco->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
		UHorizontalBoxSlot* IcoSlot = TitleHBox->AddChildToHorizontalBox(TitleIco);
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetPadding(FMargin(0, 0, 8, 0));
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UHorizontalBoxSlot* TitleSlot = TitleHBox->AddChildToHorizontalBox(Title);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);

		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		// P6-T1: زر إغلاق بأيقونة × إجرائية + نص
		{
			UHorizontalBox* CloseBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			CloseButton->AddChild(CloseBox);
			UImage* CloseIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			CloseIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("close"), 14.f, FLinearColor::White));
			CloseIco->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
			UHorizontalBoxSlot* CIcoSlot = CloseBox->AddChildToHorizontalBox(CloseIco);
			CIcoSlot->SetPadding(FMargin(4, 0, 4, 0));
			CIcoSlot->SetVerticalAlignment(VAlign_Center);
			CIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
			CloseText->SetText(FText::FromString(TEXT("إغلاق")));
			CloseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			CloseBox->AddChildToHorizontalBox(CloseText)->SetVerticalAlignment(VAlign_Center);
		}
		CloseButton->OnClicked.AddDynamic(this, &URok2BattleReportWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(CloseButton);	// P6-T3: ضغطة محسوسة
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

		// P6-T3: النافذة تفتح من المركز، والخلفية المعتمة تتلاشى معها
		URok2MotionLibrary::PlayScaleInCenter(Card);
		URok2MotionLibrary::PlayFadeIn(Backdrop);
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

		// تحديد النتيجة من منظور اللاعب — P6-T1: أيقونة إجرائية مصبوغة
		const bool bMine = (R.AttackerPlayerId == MyId);
		FString ResultIconId;
		FLinearColor ResultColor;
		if (R.Winner == TEXT("draw"))
		{
			ResultIconId = TEXT("handshake");
			ResultColor = FLinearColor(0.9f, 0.8f, 0.3f);
		}
		else if ((R.Winner == TEXT("attacker")) == bMine)
		{
			ResultIconId = TEXT("trophy");
			ResultColor = FLinearColor(0.3f, 0.9f, 0.4f);
		}
		else
		{
			ResultIconId = TEXT("skull");
			ResultColor = FLinearColor(0.95f, 0.35f, 0.3f);
		}

		const FDateTime Dt = FDateTime::FromUnixTimestamp(R.CreatedAt / 1000);

		UButton* Row = NewObject<UButton>(this);
		UHorizontalBox* RowBox = NewObject<UHorizontalBox>(this);
		Row->AddChild(RowBox);

		// أيقونة النتيجة الإجرائية
		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(ResultIconId, 18.f, ResultColor));
		Ico->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
		UHorizontalBoxSlot* IcoSlot = RowBox->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(4, 0, 6, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* RowText = NewObject<UTextBlock>(this);
		const FString Label = FString::Printf(TEXT("%s%s · %s\n%02d:%02d"),
				*KindLabel(R.Kind), R.RallyId.IsEmpty() ? TEXT("") : TEXT(" · رالي"),
				bMine ? TEXT("هجومك") : TEXT("معركة"), Dt.GetHour(), Dt.GetMinute());
		RowText->SetText(FText::FromString(Label));
		RowText->SetColorAndOpacity(FSlateColor(ResultColor));
		RowText->SetAutoWrapText(true);
		RowBox->AddChildToHorizontalBox(RowText)->SetVerticalAlignment(VAlign_Center);

		URok2ReportRowHandler* Handler = NewObject<URok2ReportRowHandler>(this);
		Handler->Index = i;
		Handler->Widget = this;
		Row->OnClicked.AddDynamic(Handler, &URok2ReportRowHandler::OnClick);
		URok2MotionLibrary::BindPress(Row);	// P6-T3: ضغطة محسوسة على صف التقرير

		ReportList->AddChildToVerticalBox(Row)->SetPadding(FMargin(4, 3, 4, 3));
	}
}

void URok2BattleReportWidget::ShowReport(const FRok2BattleReport& R)
{
	if (!DetailPanel) return;
	DetailPanel->ClearChildren();

	auto AddLine = [this](const FString& Text, const FLinearColor& Color,
		ERok2TextRole Role = ERok2TextRole::BodySmall, const FMargin& Pad = FMargin(12, 4, 12, 0))
	{
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(Text));
		T->SetColorAndOpacity(FSlateColor(Color));
		URok2Typography::ApplyFont(T, Role);
		T->SetAutoWrapText(true);
		DetailPanel->AddChildToVerticalBox(T)->SetPadding(Pad);
	};

	// العنوان — P6-T1: أيقونة إجرائية للنتيجة + عنوان نصي
	const bool bAttackerWon = (R.Winner == TEXT("attacker"));
	const TCHAR* HeadlineIconId = TEXT("handshake");
	FString Headline;
	if (R.Winner == TEXT("draw"))
	{
		Headline = TEXT("تعادل — انسحاب الطرفين");
		HeadlineIconId = TEXT("handshake");
	}
	else if (bAttackerWon)
	{
		Headline = TEXT("انتصار المهاجم");
		HeadlineIconId = TEXT("trophy");
	}
	else
	{
		Headline = TEXT("صمود المدافع");
		HeadlineIconId = TEXT("shield");
	}

	// صف عنوان التفاصيل: أيقونة + نص
	{
		UHorizontalBox* HeadRow = NewObject<UHorizontalBox>(this);
		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(HeadlineIconId, 20.f, Rok2Gold()));
		Ico->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
		UHorizontalBoxSlot* IcoSlot = HeadRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 6, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *Headline, *KindLabel(R.Kind))));
		T->SetColorAndOpacity(FSlateColor(Rok2Gold()));
		URok2Typography::ApplyFont(T, ERok2TextRole::Subtitle);
		T->SetAutoWrapText(true);
		HeadRow->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		DetailPanel->AddChildToVerticalBox(HeadRow)->SetPadding(FMargin(12, 12, 12, 4));
	}

	// القوة قبل المعركة
	AddLine(FString::Printf(TEXT("القوة قبل المعركة — مهاجم: %d · مدافع: %d"),
		R.Attacker.PowerBefore, R.Defender.PowerBefore),
		FLinearColor(0.8f, 0.85f, 0.9f), 13, FMargin(12, 8, 12, 0));

	// المهاجم (أيقونة سيف حمراء)
	{
		UHorizontalBox* SideRow = NewObject<UHorizontalBox>(this);
		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("sword"), 15.f, FLinearColor(0.95f, 0.45f, 0.4f)));
		Ico->SetDesiredSizeOverride(FVector2D(15.f, 15.f));
		UHorizontalBoxSlot* IcoSlot = SideRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(TEXT("المهاجم")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.45f, 0.4f)));
		URok2Typography::ApplyFont(T, ERok2TextRole::Subtitle);
		SideRow->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		DetailPanel->AddChildToVerticalBox(SideRow)->SetPadding(FMargin(12, 14, 12, 2));
	}
	AddLine(SummarizeSide(R.Attacker), FLinearColor::White);

	// المدافع (أيقونة درع زرقاء)
	{
		UHorizontalBox* SideRow = NewObject<UHorizontalBox>(this);
		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("shield"), 15.f, FLinearColor(0.45f, 0.65f, 1.0f)));
		Ico->SetDesiredSizeOverride(FVector2D(15.f, 15.f));
		UHorizontalBoxSlot* IcoSlot = SideRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(TEXT("المدافع")));
		T->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.65f, 1.0f)));
		URok2Typography::ApplyFont(T, ERok2TextRole::Subtitle);
		SideRow->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		DetailPanel->AddChildToVerticalBox(SideRow)->SetPadding(FMargin(12, 14, 12, 2));
	}
	AddLine(SummarizeSide(R.Defender), FLinearColor::White);

	// تسوية الرالي: هذه الصفوف مخرجات الخادم لكل مشارك، لا حسابات عميل.
	if (!R.RallyId.IsEmpty())
	{
		AddLine(FString::Printf(TEXT("نتيجة رالي التحالف · %d مشاركين"), R.RallyParticipants.Num()),
			FLinearColor(0.95f, 0.78f, 0.30f), ERok2TextRole::Subtitle, FMargin(12, 16, 12, 2));
		const FString MyId = Api ? Api->GetPlayer().Id : FString();
		auto TotalOf = [](const TArray<FRok2TroopLoss>& Units)
		{
			int32 Total = 0;
			for (const FRok2TroopLoss& Unit : Units) Total += Unit.Count;
			return Total;
		};
		for (const FRok2RallyReportParticipant& Participant : R.RallyParticipants)
		{
			const bool bIsMe = Participant.PlayerId == MyId;
			const int32 HospitalTotal = TotalOf(Participant.HospitalAdmitted);
			const FString MemberName = bIsMe ? TEXT("مساهمتك") : FString::Printf(TEXT("عضو التحالف %s"), *Participant.PlayerId.Left(8));
			AddLine(FString::Printf(TEXT("%s — أرسل %d · عاد %d · خسر %d · قتلى %d · مستشفى %d"),
				*MemberName, TotalOf(Participant.Committed), TotalOf(Participant.Remaining),
				TotalOf(Participant.Losses), TotalOf(Participant.Dead), HospitalTotal),
				bIsMe ? FLinearColor(0.45f, 0.9f, 0.75f) : FLinearColor(0.82f, 0.85f, 0.9f),
				ERok2TextRole::Caption, FMargin(16, 3, 12, 0));
		}
	}

	if (R.Rewards.Num() > 0)
	{
		AddLine(TEXT("المكافآت السلطوية"), FLinearColor(0.95f, 0.78f, 0.30f), ERok2TextRole::Subtitle, FMargin(12, 16, 12, 2));
		for (const FRok2BattleReward& Reward : R.Rewards)
		{
			const FString RewardLabel = Reward.Kind == TEXT("season_points") ? TEXT("نقاط الموسم")
				: Reward.Kind == TEXT("barbarian_event_points") ? TEXT("نقاط حدث البرابرة") : Reward.Kind;
			AddLine(FString::Printf(TEXT("%s: +%d"), *RewardLabel, Reward.Amount),
				FLinearColor(0.95f, 0.84f, 0.35f), ERok2TextRole::Caption, FMargin(16, 3, 12, 0));
		}
	}

	// ملاحظة المستشفى (أيقونة صليب خضراء)
	int32 SevTotal = 0;
	for (const FRok2TroopLoss& L : R.Attacker.Severely) SevTotal += L.Count;
	if (SevTotal > 0)
	{
		UHorizontalBox* HospRow = NewObject<UHorizontalBox>(this);
		UImage* Ico = NewObject<UImage>(this);
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("cross"), 14.f, FLinearColor(0.4f, 0.9f, 0.7f)));
		Ico->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
		UHorizontalBoxSlot* IcoSlot = HospRow->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(0, 0, 5, 0));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UTextBlock* T = NewObject<UTextBlock>(this);
		T->SetText(FText::FromString(FString::Printf(TEXT("%d جريح خطير يحتاجون مستشفى للشفاء"), SevTotal)));
		T->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.9f, 0.7f)));
		URok2Typography::ApplyFont(T, ERok2TextRole::Caption);
		T->SetAutoWrapText(true);
		HospRow->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		DetailPanel->AddChildToVerticalBox(HospRow)->SetPadding(FMargin(12, 16, 12, 12));
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
		TEXT("الخسائر: %d (قتلى %d · خطير %d · خفيف %d)\nالمتبقي: %d\n%s"),
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
	// P6-T3: تسريح بتلاشٍ ثم إزالة (لا قفزة مفاجئة) — §1 «لا قفزات جامدة»
	URok2MotionLibrary::PlayFadeOut(this);
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
