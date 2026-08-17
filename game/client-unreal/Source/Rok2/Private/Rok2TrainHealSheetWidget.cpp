// Copyright ROK2. ورقة التدريب والشفاء اللمسية (P18-T2) — implementation.

#include "Rok2TrainHealSheetWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
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
#include "Components/ScrollBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void URok2TrainUnitProxy::HandleClick()
{
	if (!Owner) return;
	if (bIsAction)
	{
		Owner->HandleUnitAction(UnitId);
	}
	else
	{
		Owner->AdjustCount(UnitId, Delta);
	}
}

namespace
{
	FString FormatCount(int32 Count)
	{
		return FString::Printf(TEXT("%d"), Count);
	}
}

void URok2TrainHealSheetWidget::Setup(URok2Api* InApi, const FString& InMode, const FString& InBuildingId)
{
	Api = InApi;
	Mode = InMode;
	BuildingId = InBuildingId;

	if (Api)
	{
		// المدينة تحمل الجرحى وسعة المستشفى؛ ولقطة حديثة قبل فتح نمط الشفاء
		// تضمن أرقاماً لا تسبق آخر معركة.
		Api->OnCityLoaded.AddDynamic(this, &URok2TrainHealSheetWidget::OnCityLoadedHandler);
		Api->LoadCity();
	}
}

TSharedRef<SWidget> URok2TrainHealSheetWidget::RebuildWidget()
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

void URok2TrainHealSheetWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TrainHealRoot"));
	WidgetTree->RootWidget = RootPanel;

	// حجاب للإغلاق باللمس خارج الورقة — نفس سلوك بطاقة المبنى وشاشة البحث.
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TrainHealBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2TrainHealSheetWidget::OnCloseClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	// ورقة سفلية (Bottom Sheet) حسب مواصفة UI/UX §3.2 — بطاقات المباني
	// تفتح من أسفل، وأفعالها من نفس العائلة.
	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TrainHealSheet"));
	Sheet->SetBrush(Rok2Surface::Sheet());
	Sheet->SetPadding(FMargin(Rok2Space::L));
	UCanvasPanelSlot* SheetSlot = RootPanel->AddChildToCanvas(Sheet);
	SheetSlot->SetAnchors(FAnchors(0.05f, 0.28f, 0.95f, 0.95f));
	SheetSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TrainHealColumn"));
	Sheet->SetContent(Column);

	// ── الترويسة ──
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		const bool bHeal = (Mode == TEXT("heal"));
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(bHeal ? TEXT("cross") : TEXT("sword"), 24.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TrainHealHeader"));
		HeaderText->SetText(FText::FromString(bHeal ? TEXT("المستشفى — شفاء الجرحى") : TEXT("تدريب القوات")));
		HeaderText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(HeaderText, ERok2TextRole::Title);
		Header->AddChildToHorizontalBox(HeaderText)->SetVerticalAlignment(VAlign_Center);

		USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Header->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TrainHealClose"));
		Close->SetStyle(Rok2Surface::SecondaryButton());
		Close->OnClicked.AddDynamic(this, &URok2TrainHealSheetWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(Close);
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CloseText->SetText(FText::FromString(TEXT("إغلاق")));
		CloseText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(CloseText, ERok2TextRole::Button);
		Close->AddChild(CloseText);
		Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);
	}

	// ── سطر الحالة (سعة المستشفى في نمط الشفاء) ──
	if (Mode == TEXT("heal"))
	{
		UButton* HealAll = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HealAllButton"));
		HealAll->SetStyle(Rok2Surface::PrimaryButton());
		HealAll->OnClicked.AddDynamic(this, &URok2TrainHealSheetWidget::OnHealAllClicked);
		URok2MotionLibrary::BindPress(HealAll);
		UTextBlock* HealAllText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HealAllText->SetText(FText::FromString(TEXT("شفاء الكل")));
		HealAllText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(HealAllText, ERok2TextRole::Button);
		HealAll->AddChild(HealAllText);
		Column->AddChildToVerticalBox(HealAll)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));
	}

	// ── الصفوف (تمرير: الفروع قد تحمل خمس درجات) ──
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TrainHealScroll"));
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		RowsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TrainHealRows"));
		Scroll->AddChild(RowsList);
	}

	RebuildRows();
	URok2MotionLibrary::PlayFadeIn(RootPanel);
}

void URok2TrainHealSheetWidget::OnCloseClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	// P18-T5: كانت `RemoveFromParent()` عارية بينما الورقة تفتح بحركة تلاشٍ —
	// خروج غير متماثل تمنعه §1 «لا قفزات جامدة».
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2TrainHealSheetWidget::OnHealAllClicked()
{
	if (!Api) return;

	const FRok2City& City = Api->GetCity();
	if (City.Wounded.Num() == 0)
	{
		Api->EmitToast(TEXT("لا جرحى للشفاء"));
		return;
	}
	Api->HealWounded(City.Wounded);
	OnCloseClicked();
}

void URok2TrainHealSheetWidget::AdjustCount(const FString& UnitId, int32 Delta)
{
	int32* Existing = Counts.Find(UnitId);
	const int32 Current = Existing ? *Existing : 1;
	// صعوداً وهبوطاً ضمن حدود معقولة؛ حدّ الشذوذ النهائي عند الخادم.
	const int32 Next = FMath::Clamp(Current + Delta, 1, 999);
	Counts.Add(UnitId, Next);
	RebuildRows();
}

void URok2TrainHealSheetWidget::HandleUnitAction(const FString& UnitId)
{
	if (!Api) return;

	if (Mode == TEXT("heal"))
	{
		const FRok2City& City = Api->GetCity();
		const int32* WoundedCount = City.Wounded.Find(UnitId);
		if (!WoundedCount || *WoundedCount <= 0)
		{
			Api->EmitToast(TEXT("لا جرحى من هذه الوحدة"));
			return;
		}
		TMap<FString, int32> Troops;
		Troops.Add(UnitId, *WoundedCount);
		Api->HealWounded(Troops);
	}
	else
	{
		const int32 Count = Counts.FindRef(UnitId);
		Api->Train(UnitId, Count > 0 ? Count : 1);
	}
	OnCloseClicked();
}

FString URok2TrainHealSheetWidget::BranchForBuilding(const FString& InBuildingId) const
{
	// مطابقة أسماء buildings.json — لا هنا ولا في الخادم فروع مختلقة.
	if (InBuildingId == TEXT("barracks")) return TEXT("infantry");
	if (InBuildingId == TEXT("stable")) return TEXT("cavalry");
	if (InBuildingId == TEXT("archery_range")) return TEXT("archer");
	if (InBuildingId == TEXT("siege_workshop")) return TEXT("siege");
	return TEXT("");
}

FString URok2TrainHealSheetWidget::UnitName(const FString& UnitId) const
{
	if (Api)
	{
		for (const FRok2TrainableUnit& Unit : Api->GetMeta().TrainableUnits)
		{
			if (Unit.Id == UnitId && !Unit.Name.IsEmpty())
			{
				return Unit.Name;
			}
		}
	}
	return UnitId;
}

void URok2TrainHealSheetWidget::RebuildRows()
{
	if (!RowsList || !WidgetTree || !Api) return;

	RowsList->ClearChildren();
	Proxies.Empty();

	const bool bHeal = (Mode == TEXT("heal"));

	if (bHeal)
	{
		const FRok2City& City = Api->GetCity();
		if (HeaderText)
		{
			HeaderText->SetText(FText::FromString(FString::Printf(TEXT("المستشفى — الجرحى %d / السعة %d"),
				City.HospitalUsed, City.HospitalCapacity)));
		}
		if (City.Wounded.Num() == 0)
		{
			UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Empty->SetText(FText::FromString(TEXT("لا جرحى حالياً — قواتك سليمة.")));
			Empty->SetColorAndOpacity(FSlateColor(Rok2Visual::SuccessText()));
			Empty->SetJustification(ETextJustify::Center);
			URok2Typography::ApplyFont(Empty, ERok2TextRole::Body);
			RowsList->AddChildToVerticalBox(Empty)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XL, Rok2Space::None, Rok2Space::None));
			return;
		}
	}
	else
	{
		const FString Branch = BranchForBuilding(BuildingId);
		int32 Available = 0;
		for (const FRok2TrainableUnit& Unit : Api->GetMeta().TrainableUnits)
		{
			if (Branch.IsEmpty() || Unit.Branch == Branch) ++Available;
		}
		if (HeaderText && Available == 0)
		{
			HeaderText->SetText(FText::FromString(TEXT("لا وحدات متاحة للتدريب هنا بعد.")));
		}
	}

	// نمط التدريب: صف لكل وحدة من فرع المبنى.
	if (!bHeal)
	{
		const FString Branch = BranchForBuilding(BuildingId);
		for (const FRok2TrainableUnit& Unit : Api->GetMeta().TrainableUnits)
		{
			if (!Branch.IsEmpty() && Unit.Branch != Branch) continue;
			if (!Counts.Contains(Unit.Id))
			{
				Counts.Add(Unit.Id, 1);
			}
			BuildRow(Unit.Id, UnitName(Unit.Id), true);
		}
	}
	else
	{
		// نمط الشفاء: صف لكل وحدة جريحة.
		const FRok2City& City = Api->GetCity();
		for (const TPair<FString, int32>& Pair : City.Wounded)
		{
			BuildRow(Pair.Key, UnitName(Pair.Key), false);
		}
	}
}

void URok2TrainHealSheetWidget::BuildRow(const FString& UnitId, const FString& DisplayName, const bool bTrainMode)
{
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrush(Rok2Surface::Card());
	Card->SetPadding(FMargin(Rok2Space::M));
	RowsList->AddChildToVerticalBox(Card)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (bTrainMode)
	{
		Name->SetText(FText::FromString(DisplayName));
	}
	else
	{
		const int32* WoundedCount = Api ? Api->GetCity().Wounded.Find(UnitId) : nullptr;
		Name->SetText(FText::FromString(FString::Printf(TEXT("%s — جريح: %d"), *DisplayName, WoundedCount ? *WoundedCount : 0)));
	}
	Name->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(Name, ERok2TextRole::CardTitle);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	Row->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// ── العدّاد اللمسي (− عدد +) — بديل SpinBox سطح المكتب (خارطة P21-T2) ──
	if (bTrainMode)
	{
		auto MakeStep = [&](const int32 Delta, const TCHAR* Label) {
			URok2TrainUnitProxy* Proxy = NewObject<URok2TrainUnitProxy>(this);
			Proxy->UnitId = UnitId;
			Proxy->Delta = Delta;
			Proxy->Owner = this;
			Proxies.Add(Proxy);

			UButton* Step = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Step->SetStyle(Rok2Surface::SecondaryButton());
			Step->OnClicked.AddDynamic(Proxy, &URok2TrainUnitProxy::HandleClick);
			URok2MotionLibrary::BindPress(Step);
			UTextBlock* StepText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			StepText->SetText(FText::FromString(Label));
			StepText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
			URok2Typography::ApplyFont(StepText, ERok2TextRole::Button);
			Step->AddChild(StepText);
			UHorizontalBoxSlot* StepSlot = Row->AddChildToHorizontalBox(Step);
			StepSlot->SetPadding(FMargin(Rok2Space::XS, Rok2Space::None, Rok2Space::None, Rok2Space::None));
			StepSlot->SetVerticalAlignment(VAlign_Center);
			StepSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		};
		MakeStep(-1, TEXT("−"));
		MakeStep(+1, TEXT("+"));

		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NoneCount"));
		CountText->SetText(FText::FromString(FormatCount(Counts.FindRef(UnitId) > 0 ? Counts.FindRef(UnitId) : 1)));
		CountText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(CountText, ERok2TextRole::Numeric);
		UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountText);
		CountSlot->SetPadding(FMargin(Rok2Space::S, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// ── زر الإجراء (تدريب / شفاء) ──
	{
		URok2TrainUnitProxy* Proxy = NewObject<URok2TrainUnitProxy>(this);
		Proxy->UnitId = UnitId;
		Proxy->bIsAction = true;
		Proxy->Owner = this;
		Proxies.Add(Proxy);

		UButton* Action = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Action->SetStyle(Rok2Surface::PrimaryButton());
		Action->OnClicked.AddDynamic(Proxy, &URok2TrainUnitProxy::HandleClick);
		URok2MotionLibrary::BindPress(Action);
		UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ActionText->SetText(FText::FromString(bTrainMode ? TEXT("تدريب") : TEXT("شفاء")));
		ActionText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(ActionText, ERok2TextRole::Button);
		Action->AddChild(ActionText);
		UHorizontalBoxSlot* ActionSlot = Row->AddChildToHorizontalBox(Action);
		ActionSlot->SetPadding(FMargin(Rok2Space::M, Rok2Space::None, Rok2Space::None, Rok2Space::None));
		ActionSlot->SetVerticalAlignment(VAlign_Center);
		ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
}

void URok2TrainHealSheetWidget::OnCityLoadedHandler(const FRok2City& City)
{
	// نمط الشفاء يعتمد أرقام الجرحى؛ إعادة رسم عند وصول مدينة محدّثة.
	if (Mode == TEXT("heal"))
	{
		RebuildRows();
	}
}
