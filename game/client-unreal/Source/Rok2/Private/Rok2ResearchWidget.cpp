// Copyright ROK2. شاشة البحث العلمي (P18-T1) — implementation.

#include "Rok2ResearchWidget.h"
#include "Rok2Accessibility.h"
#include "Rok2Api.h"
#include "Rok2ArtAssets.h"
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

namespace
{
	/** الفروع الثلاثة كما يسمّيها الخادم في data/research.json. */
	const TCHAR* BranchEconomy = TEXT("economy");
	const TCHAR* BranchMilitary = TEXT("military");
	const TCHAR* BranchDefense = TEXT("defense");

	FString FormatDuration(int32 Seconds)
	{
		if (Seconds <= 0) return TEXT("—");
		const int32 Hours = Seconds / 3600;
		const int32 Minutes = (Seconds % 3600) / 60;
		if (Hours > 0)
		{
			return FString::Printf(TEXT("%dس %02dد"), Hours, Minutes);
		}
		if (Minutes > 0)
		{
			return FString::Printf(TEXT("%dد"), Minutes);
		}
		return FString::Printf(TEXT("%dث"), Seconds);
	}

	FString FormatAmount(double Amount)
	{
		if (Amount >= 1000000.0) return FString::Printf(TEXT("%.1fم"), Amount / 1000000.0);
		if (Amount >= 1000.0) return FString::Printf(TEXT("%.1fك"), Amount / 1000.0);
		return FString::Printf(TEXT("%.0f"), Amount);
	}
}

void URok2ResearchRowProxy::HandleClick()
{
	if (Owner)
	{
		Owner->RequestResearch(TechId);
	}
}

void URok2ResearchWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnResearchLoaded.AddDynamic(this, &URok2ResearchWidget::OnResearchLoaded);
	// المدينة تتغيّر بعد كل بحث (خصم موارد + طابور)، وحالة «موارد ناقصة» تعتمد
	// عليها — فنعيد الرسم عند تحديثها لا عند وصول الشجرة وحدها.
	Api->OnCityLoaded.AddDynamic(this, &URok2ResearchWidget::OnCityUpdated);
	Api->FetchResearch();
}

TSharedRef<SWidget> URok2ResearchWidget::RebuildWidget()
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

void URok2ResearchWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	if (ActiveBranch.IsEmpty())
	{
		ActiveBranch = BranchEconomy;
	}

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResearchRoot"));
	WidgetTree->RootWidget = RootPanel;

	// حجاب يغلق ما تحته — كانت الشاشة بلا حجاب فيظهر العالم خلف قائمة نصف شفافة.
	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResearchBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnCloseClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResearchSheet"));
	Sheet->SetBrush(Rok2Surface::Sheet());
	Sheet->SetPadding(FMargin(Rok2Space::L));
	UCanvasPanelSlot* SheetSlot = RootPanel->AddChildToCanvas(Sheet);
	SheetSlot->SetAnchors(FAnchors(0.08f, 0.10f, 0.92f, 0.92f));
	SheetSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResearchColumn"));
	Sheet->SetContent(Column);

	// ── ترويسة: أيقونة + عنوان + مستوى الأكاديمية + إغلاق ──
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("research"), 24.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("research")));
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("البحث العلمي")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
		Header->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

		USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Header->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		AcademyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AcademyText"));
		AcademyText->SetText(FText::FromString(TEXT("الأكاديمية —")));
		AcademyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(AcademyText, ERok2TextRole::Caption);
		UHorizontalBoxSlot* AcademySlot = Header->AddChildToHorizontalBox(AcademyText);
		AcademySlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
		AcademySlot->SetVerticalAlignment(VAlign_Center);

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResearchClose"));
		Close->SetStyle(Rok2Surface::SecondaryButton());
		Close->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(Close);
		UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		CloseText->SetText(FText::FromString(TEXT("إغلاق")));
		CloseText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(CloseText, ERok2TextRole::Button);
		Close->AddChild(CloseText);
		Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);
	}

	// ── تبويبات الفروع ──
	{
		UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResearchTabs"));
		Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		auto MakeTab = [&](UButton*& OutBtn, const FString& Label, const FName Handler) {
			OutBtn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			T->SetText(FText::FromString(Label));
			T->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
			URok2Typography::ApplyFont(T, ERok2TextRole::Button);
			OutBtn->AddChild(T);
			URok2MotionLibrary::BindPress(OutBtn);
			UHorizontalBoxSlot* Slot = Tabs->AddChildToHorizontalBox(OutBtn);
			Slot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		};

		MakeTab(EconomyTab, TEXT("الاقتصاد"), FName(TEXT("OnEconomyTab")));
		EconomyTab->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnEconomyTab);
		MakeTab(MilitaryTab, TEXT("العسكري"), FName(TEXT("OnMilitaryTab")));
		MilitaryTab->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnMilitaryTab);
		MakeTab(DefenseTab, TEXT("الدفاع"), FName(TEXT("OnDefenseTab")));
		DefenseTab->OnClicked.AddDynamic(this, &URok2ResearchWidget::OnDefenseTab);
	}

	// ── قائمة التقنيات (قابلة للتمرير: الفرع الواحد قد يحمل عشر تقنيات) ──
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ResearchScroll"));
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		TechList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TechList"));
		Scroll->AddChild(TechList);
	}

	SelectBranch(ActiveBranch);
	URok2MotionLibrary::PlayFadeIn(RootPanel);
}

void URok2ResearchWidget::OnEconomyTab() { SelectBranch(BranchEconomy); }
void URok2ResearchWidget::OnMilitaryTab() { SelectBranch(BranchMilitary); }
void URok2ResearchWidget::OnDefenseTab() { SelectBranch(BranchDefense); }

void URok2ResearchWidget::OnCloseClicked()
{
	RemoveFromParent();
}

void URok2ResearchWidget::OnResearchLoaded()
{
	RebuildList();
}

void URok2ResearchWidget::OnCityUpdated(const FRok2City& City)
{
	// خصم أو طابور جديد بعد بحث أو ترقية — حالات التقنيات قد تنقلب، فنرسم من جديد.
	RebuildList();
}

void URok2ResearchWidget::SelectBranch(const FString& Branch)
{
	ActiveBranch = Branch;

	// النمط الكامل لا لون Normal — التبويب الخامل يحتفظ بتحويم وضغط.
	if (EconomyTab) EconomyTab->SetStyle(Rok2Surface::TabButton(Branch == BranchEconomy));
	if (MilitaryTab) MilitaryTab->SetStyle(Rok2Surface::TabButton(Branch == BranchMilitary));
	if (DefenseTab) DefenseTab->SetStyle(Rok2Surface::TabButton(Branch == BranchDefense));

	RebuildList();
}

void URok2ResearchWidget::RequestResearch(const FString& TechId)
{
	if (Api)
	{
		// الخادم يفرض الأكاديمية والمتطلبات والتكلفة؛ العميل يعطّل الزر مسبقاً
		// للوضوح فقط، ولا يعتمد التعطيل كتحقق.
		Api->StartResearch(TechId);
	}
}

void URok2ResearchWidget::RebuildList()
{
	if (!TechList || !WidgetTree) return;

	TechList->ClearChildren();
	RowProxies.Empty();

	if (!Api)
	{
		return;
	}

	const FRok2ResearchState& State = Api->GetResearchState();
	if (AcademyText)
	{
		AcademyText->SetText(FText::FromString(FString::Printf(TEXT("الأكاديمية %d"), State.AcademyLevel)));
	}

	const FRok2City& City = Api->GetCity();

	int32 Shown = 0;
	for (const FRok2TechNode& Tech : State.Technologies)
	{
		if (Tech.Category != ActiveBranch) continue;
		++Shown;

		// ── تقييم الحالة ──
		// الترتيب مقصود: السقف أولاً (فلا معنى لباقي الشروط)، ثم الأكاديمية،
		// ثم المتطلبات، ثم الموارد — وهو ترتيب الخادم نفسه في /v1/city/research
		// كي تطابق الرسالة المعروضة سبب الرفض الفعلي لو ضُغط الزر.
		bool bBlocked = false;
		FString StateLabel;
		FLinearColor StateColor = Rok2Visual::SuccessText();
		FString StateIcon = TEXT("flask");

		if (!Tech.bHasNextLevel)
		{
			bBlocked = true;
			StateLabel = TEXT("بلغت السقف");
			StateColor = Rok2Visual::GoldText();
			StateIcon = TEXT("star");
		}
		else if (State.AcademyLevel < Tech.NextAcademyRequirement)
		{
			bBlocked = true;
			StateLabel = FString::Printf(TEXT("تحتاج أكاديمية %d"), Tech.NextAcademyRequirement);
			StateColor = Rok2Visual::DangerText();
			StateIcon = TEXT("lock");
		}
		else
		{
			// متطلب ناقص: تقنية سابقة مستواها صفر.
			FString MissingName;
			for (const FString& PrereqId : Tech.Prerequisites)
			{
				const FRok2TechNode* Prereq = State.Technologies.FindByPredicate(
					[&PrereqId](const FRok2TechNode& Node) { return Node.Id == PrereqId; });
				if (Prereq && Prereq->Level <= 0)
				{
					MissingName = Prereq->Name.IsEmpty() ? PrereqId : Prereq->Name;
					break;
				}
			}

			if (!MissingName.IsEmpty())
			{
				bBlocked = true;
				StateLabel = FString::Printf(TEXT("يتطلب: %s"), *MissingName);
				StateColor = Rok2Visual::DangerText();
				StateIcon = TEXT("lock");
			}
			else if (City.Resources.Food < Tech.NextCost.Food
				|| City.Resources.Wood < Tech.NextCost.Wood
				|| City.Resources.Stone < Tech.NextCost.Stone
				|| City.Resources.Gold < Tech.NextCost.Gold)
			{
				bBlocked = true;
				StateLabel = TEXT("موارد غير كافية");
				StateColor = Rok2Visual::DangerText();
				StateIcon = TEXT("gold");
			}
			else
			{
				StateLabel = TEXT("متاح للبحث");
			}
		}

		// ── بطاقة الصف ──
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		// البطاقة المتاحة تحمل حافة ذهبية تدلّ عليها قبل قراءة النص.
		Card->SetBrush(bBlocked ? Rok2Surface::Card() : Rok2Surface::AccentCard(Rok2Visual::Gold()));
		Card->SetPadding(FMargin(Rok2Space::M));
		TechList->AddChildToVerticalBox(Card)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Card->SetContent(Row);

		UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row->AddChildToHorizontalBox(Info)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// الاسم + المستوى الحالي من السقف
		UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Name->SetText(FText::FromString(FString::Printf(TEXT("%s  %d/%d"),
			*Tech.Name, Tech.Level, Tech.MaxLevel)));
		Name->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(Name, ERok2TextRole::CardTitle);
		Info->AddChildToVerticalBox(Name);

		if (!Tech.Description.IsEmpty())
		{
			UTextBlock* Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Desc->SetText(FText::FromString(Tech.Description));
			Desc->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
			Desc->SetAutoWrapText(true);
			URok2Typography::ApplyFont(Desc, ERok2TextRole::BodySmall);
			Info->AddChildToVerticalBox(Desc)->SetPadding(FMargin(Rok2Space::None, Rok2Space::Hair, Rok2Space::None, Rok2Space::None));
		}

		// تكلفة المستوى التالي ومدته — من الخادم، لا حساب محلي.
		if (Tech.bHasNextLevel)
		{
			UHorizontalBox* CostRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Info->AddChildToVerticalBox(CostRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XS, Rok2Space::None, Rok2Space::None));

			auto AddCost = [&](const TCHAR* IconId, double Amount, const FLinearColor& Tint, double Have) {
				if (Amount <= 0.0) return;
				UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 14.f, Tint));
				Ico->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
				Ico->SetToolTipText(URok2Accessibility::LabelForIcon(FString(IconId)));
				UHorizontalBoxSlot* IcoSlot = CostRow->AddChildToHorizontalBox(Ico);
				IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
				IcoSlot->SetVerticalAlignment(VAlign_Center);
				IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

				UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				T->SetText(FText::FromString(FormatAmount(Amount)));
				// المورد الناقص بلون الخطر — فيرى اللاعب أيّ مورد يعوقه لا مجرد «غير كافٍ».
				T->SetColorAndOpacity(FSlateColor(Have < Amount ? Rok2Visual::DangerText() : Rok2Visual::Ivory()));
				URok2Typography::ApplyFont(T, ERok2TextRole::Numeric);
				UHorizontalBoxSlot* TxtSlot = CostRow->AddChildToHorizontalBox(T);
				TxtSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
				TxtSlot->SetVerticalAlignment(VAlign_Center);
			};

			AddCost(TEXT("food"), Tech.NextCost.Food, Rok2Visual::ResourceFood(), City.Resources.Food);
			AddCost(TEXT("wood"), Tech.NextCost.Wood, Rok2Visual::ResourceWood(), City.Resources.Wood);
			AddCost(TEXT("stone"), Tech.NextCost.Stone, Rok2Visual::ResourceStone(), City.Resources.Stone);
			AddCost(TEXT("gold"), Tech.NextCost.Gold, Rok2Visual::ResourceGold(), City.Resources.Gold);

			UTextBlock* Time = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Time->SetText(FText::FromString(FormatDuration(Tech.NextDurationSeconds)));
			Time->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
			URok2Typography::ApplyFont(Time, ERok2TextRole::Timer);
			CostRow->AddChildToHorizontalBox(Time)->SetVerticalAlignment(VAlign_Center);
		}

		// سطر الحالة: أيقونة + نص + لون — ثلاث إشارات لا لون وحده.
		{
			UHorizontalBox* StateRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Info->AddChildToVerticalBox(StateRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XS, Rok2Space::None, Rok2Space::None));

			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(StateIcon, 13.f, StateColor));
			Ico->SetDesiredSizeOverride(FVector2D(13.f, 13.f));
			UHorizontalBoxSlot* IcoSlot = StateRow->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

			UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			T->SetText(FText::FromString(StateLabel));
			T->SetColorAndOpacity(FSlateColor(StateColor));
			URok2Typography::ApplyFont(T, ERok2TextRole::Caption);
			StateRow->AddChildToHorizontalBox(T)->SetVerticalAlignment(VAlign_Center);
		}

		// ── زر البحث ──
		if (Tech.bHasNextLevel)
		{
			UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
			Btn->SetStyle(Rok2Surface::PrimaryButton());
			// الحالة المعطّلة تُقرأ بصرياً (فقدان تشبّع + خفوت) لا بالنص وحده.
			Btn->SetIsEnabled(!bBlocked);
			Btn->SetToolTipText(FText::FromString(bBlocked ? StateLabel : TEXT("ابدأ البحث")));
			URok2MotionLibrary::BindPress(Btn);

			URok2ResearchRowProxy* Proxy = NewObject<URok2ResearchRowProxy>(this);
			Proxy->TechId = Tech.Id;
			Proxy->Owner = this;
			RowProxies.Add(Proxy);
			Btn->OnClicked.AddDynamic(Proxy, &URok2ResearchRowProxy::HandleClick);

			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			BtnText->SetText(FText::FromString(FString::Printf(TEXT("بحث %d"), Tech.Level + 1)));
			BtnText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
			URok2Typography::ApplyFont(BtnText, ERok2TextRole::Button);
			Btn->AddChild(BtnText);

			UHorizontalBoxSlot* BtnSlot = Row->AddChildToHorizontalBox(Btn);
			BtnSlot->SetPadding(FMargin(Rok2Space::M, Rok2Space::None, Rok2Space::None, Rok2Space::None));
			BtnSlot->SetVerticalAlignment(VAlign_Center);
			BtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	if (Shown == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Empty->SetText(FText::FromString(TEXT("لا تقنيات في هذا الفرع بعد.")));
		Empty->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(Empty, ERok2TextRole::Body);
		Empty->SetJustification(ETextJustify::Center);
		TechList->AddChildToVerticalBox(Empty)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XL, Rok2Space::None, Rok2Space::None));
	}
}
