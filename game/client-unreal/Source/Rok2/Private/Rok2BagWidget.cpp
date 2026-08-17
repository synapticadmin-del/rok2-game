// Copyright ROK2. شاشة الحقيبة (P19-T5) — implementation.
// P6-T3: الحركة كلها من `URok2MotionLibrary` — دخول من المركز، وضغطة محسوسة على
// كل زر، وتسريح بـ`PlayFadeOut` لا إزالة مفاجئة.

#include "Rok2BagWidget.h"
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

void URok2BagItemProxy::HandleClick()
{
	if (Owner)
	{
		Owner->HandleProxyClick(ItemId, CategoryId);
	}
}

void URok2BagWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnBagUpdated.AddUniqueDynamic(this, &URok2BagWidget::OnBagUpdated);
	// لقطة حديثة عند كل فتح: العناصر تُمنح من المهام وBattle Pass والحانة بين
	// فتحة وأخرى، ولقطة قديمة تُري اللاعب رصيداً لا يملكه.
	Api->FetchBag();
}

TSharedRef<SWidget> URok2BagWidget::RebuildWidget()
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

void URok2BagWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BagRoot"));
	WidgetTree->RootWidget = RootPanel;

	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BagBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2BagWidget::OnCloseClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BagSheet"));
	Sheet->SetBrush(Rok2Surface::Sheet());
	Sheet->SetPadding(FMargin(Rok2Space::L));
	UCanvasPanelSlot* SheetSlot = RootPanel->AddChildToCanvas(Sheet);
	SheetSlot->SetAnchors(FAnchors(0.08f, 0.10f, 0.92f, 0.92f));
	SheetSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BagColumn"));
	Sheet->SetContent(Column);

	// ── الترويسة: أيقونة + عنوان + رصيد الجواهر + إغلاق ──
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("bag"), 24.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("bag")));
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("الحقيبة")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
		Header->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

		USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Header->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UImage* GemIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		GemIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("gems"), 16.f, Rok2Visual::ResourceGems()));
		GemIco->SetDesiredSizeOverride(FVector2D(16.f, 16.f));
		GemIco->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("gems")));
		UHorizontalBoxSlot* GemIcoSlot = Header->AddChildToHorizontalBox(GemIco);
		GemIcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
		GemIcoSlot->SetVerticalAlignment(VAlign_Center);
		GemIcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		GemsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BagGemsText"));
		GemsText->SetText(FText::FromString(TEXT("—")));
		GemsText->SetColorAndOpacity(FSlateColor(Rok2Visual::ResourceGems()));
		URok2Typography::ApplyFont(GemsText, ERok2TextRole::Numeric);
		UHorizontalBoxSlot* GemsSlot = Header->AddChildToHorizontalBox(GemsText);
		GemsSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
		GemsSlot->SetVerticalAlignment(VAlign_Center);
		GemsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BagClose"));
		Close->SetStyle(Rok2Surface::SecondaryButton());
		Close->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		Close->OnClicked.AddDynamic(this, &URok2BagWidget::OnCloseClicked);
		URok2MotionLibrary::BindPress(Close);
		UImage* CloseIco = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		CloseIco->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("close"), 18.f, Rok2Visual::Muted()));
		CloseIco->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
		CloseIco->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		Close->AddChild(CloseIco);
		UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(Close);
		CloseSlot->SetVerticalAlignment(VAlign_Center);
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// ── التبويبات (تُملأ من فئات الخادم) ──
	TabsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BagTabs"));
	Column->AddChildToVerticalBox(TabsBox)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

	// ── الصفوف ──
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("BagScroll"));
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		ItemsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BagItems"));
		Scroll->AddChild(ItemsList);
	}

	// حالة فارغة صادقة: تفصل «لم تُقرأ بعد» عن «لا عناصر».
	EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BagEmptyText"));
	EmptyText->SetText(FText::FromString(TEXT("جارٍ قراءة الحقيبة…")));
	EmptyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
	EmptyText->SetJustification(ETextJustify::Center);
	URok2Typography::ApplyFont(EmptyText, ERok2TextRole::Body);
	Column->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(Rok2Space::None, Rok2Space::M, Rok2Space::None, Rok2Space::None));

	RebuildTabs();
	RebuildItems();
	URok2MotionLibrary::PlayScaleInCenter(Sheet);
	URok2MotionLibrary::PlayFadeIn(Backdrop);
}

void URok2BagWidget::OnCloseClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2BagWidget::OnBagUpdated(const FRok2BagState& State)
{
	RebuildTabs();
	RebuildItems();
}

void URok2BagWidget::HandleProxyClick(const FString& ItemId, const FString& CategoryId)
{
	if (!ItemId.IsEmpty())
	{
		if (!Api) return;
		const FString QueueId = FirstActiveQueueId();
		if (QueueId.IsEmpty())
		{
			// صدق لا صمت: العنصر صالح لكن لا هدف له.
			Api->EmitToast(TEXT("لا طابور نشط لتسريعه"));
			return;
		}
		Api->UseBagItemOnQueue(ItemId, QueueId);
		return;
	}

	// تبويب: `CategoryId` الفارغ يعني «الكل» وهو حالة صحيحة لا خطأ.
	ActiveCategory = CategoryId;
	RebuildTabs();
	RebuildItems();
}

FString URok2BagWidget::FirstActiveQueueId() const
{
	if (!Api) return FString();
	for (const FRok2QueueEntry& Queue : Api->GetCity().ActiveQueues)
	{
		if (!Queue.Id.IsEmpty())
		{
			return Queue.Id;
		}
	}
	return FString();
}

void URok2BagWidget::RebuildTabs()
{
	if (!TabsBox || !WidgetTree || !Api) return;

	TabsBox->ClearChildren();
	TabProxies.Empty();

	const FRok2BagState& Bag = Api->GetBag();

	auto AddTab = [&](const FString& CategoryId, const FString& Label, const FString& IconId, int32 Count)
	{
		URok2BagItemProxy* Proxy = NewObject<URok2BagItemProxy>(this);
		Proxy->CategoryId = CategoryId;
		Proxy->Owner = this;
		TabProxies.Add(Proxy);

		const bool bActive = (ActiveCategory == CategoryId);
		UButton* Tab = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Tab->SetStyle(Rok2Surface::TabButton(bActive));
		Tab->OnClicked.AddDynamic(Proxy, &URok2BagItemProxy::HandleClick);
		URok2MotionLibrary::BindPress(Tab);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Tab->AddChild(Row);

		if (!IconId.IsEmpty())
		{
			UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(IconId, 14.f,
				bActive ? Rok2Visual::GoldText() : Rok2Visual::Muted()));
			Ico->SetDesiredSizeOverride(FVector2D(14.f, 14.f));
			Ico->SetToolTipText(URok2Accessibility::LabelForIcon(IconId));
			UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
			IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
			IcoSlot->SetVerticalAlignment(VAlign_Center);
			IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		// العدد في نص التبويب: يعرف اللاعب أي فئة تستحق اللمس قبل أن يلمسها.
		Text->SetText(FText::FromString(Count > 0
			? FString::Printf(TEXT("%s (%d)"), *Label, Count)
			: Label));
		Text->SetColorAndOpacity(FSlateColor(bActive ? Rok2Visual::GoldText() : Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(Text, ERok2TextRole::Button);
		Row->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);

		UHorizontalBoxSlot* TabSlot = TabsBox->AddChildToHorizontalBox(Tab);
		TabSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
		TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	};

	AddTab(FString(), TEXT("الكل"), TEXT("bag"), Bag.Items.Num());

	// الفئات من الخادم لا من قائمة محلية: إضافة فئة في `items.json` تظهر هنا
	// بلا تعديل عميل. والفئة الفارغة من العناصر تُطوى — تبويب لا محتوى فيه
	// يستهلك لمسة بلا مقابل.
	for (const FRok2ItemCategory& Category : Bag.Categories)
	{
		int32 Count = 0;
		for (const FRok2BagItem& Item : Bag.Items)
		{
			if (Item.Category == Category.Id) ++Count;
		}
		if (Count <= 0) continue;
		AddTab(Category.Id, Category.Name, Category.IconId, Count);
	}
}

void URok2BagWidget::RebuildItems()
{
	if (!ItemsList || !WidgetTree || !Api) return;

	ItemsList->ClearChildren();
	// وسائط الصفوف تُعاد بناؤها معها؛ بلا تفريغ تتراكم عبر الجلسة. وهي منفصلة
	// عن وسائط التبويبات لأن `RebuildItems` تُنادى وحدها عند تبديل التبويب —
	// ولو تشاركتا مصفوفة لأتلف تفريغُ أحدهما أزرار الآخر.
	ItemProxies.Empty();

	const FRok2BagState& Bag = Api->GetBag();

	if (GemsText)
	{
		GemsText->SetText(FText::FromString(Bag.bLoaded
			? FString::Printf(TEXT("%d"), Bag.Gems)
			: TEXT("—")));
	}

	int32 Shown = 0;
	for (const FRok2BagItem& Item : Bag.Items)
	{
		if (!ActiveCategory.IsEmpty() && Item.Category != ActiveCategory) continue;
		BuildItemRow(Item);
		++Shown;
	}

	if (EmptyText)
	{
		if (!Bag.bLoaded)
		{
			EmptyText->SetText(FText::FromString(TEXT("جارٍ قراءة الحقيبة…")));
			EmptyText->SetVisibility(ESlateVisibility::Visible);
		}
		else if (Shown == 0)
		{
			EmptyText->SetText(FText::FromString(Bag.Items.Num() == 0
				? TEXT("حقيبتك فارغة — المهام اليومية والمتجر وBattle Pass تمنح عناصر.")
				: TEXT("لا عناصر في هذه الفئة.")));
			EmptyText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			EmptyText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void URok2BagWidget::BuildItemRow(const FRok2BagItem& Item)
{
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	// حافة النُدرة من رمز المشروع لا من لون مخترع لكل درجة.
	Card->SetBrush(Rok2Surface::AccentCard(Rok2Visual::RarityTier(Item.Rarity)));
	Card->SetPadding(FMargin(Rok2Space::M));
	ItemsList->AddChildToVerticalBox(Card)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Ico->SetBrush(URok2ArtAssets::GetIconBrush(Item.IconId, 32.f, Rok2Visual::RarityTier(Item.Rarity)));
	Ico->SetDesiredSizeOverride(FVector2D(32.f, 32.f));
	Ico->SetToolTipText(URok2Accessibility::LabelForIcon(Item.IconId));
	UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
	IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
	IcoSlot->SetVerticalAlignment(VAlign_Center);
	IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextColumn);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Name->SetText(FText::FromString(Item.Name));
	Name->SetColorAndOpacity(FSlateColor(Item.bKnown ? Rok2Visual::Ivory() : Rok2Visual::Muted()));
	URok2Typography::ApplyFont(Name, ERok2TextRole::CardTitle);
	TextColumn->AddChildToVerticalBox(Name);

	UTextBlock* Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	// العنصر المجهول يُقال عنه ذلك صريحاً بدل وصفٍ مخترع.
	Desc->SetText(FText::FromString(Item.bKnown
		? Item.Description
		: TEXT("عنصر لا يعرفه فهرس الخادم — يُعرض بمعرّفه.")));
	Desc->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
	Desc->SetAutoWrapText(true);
	URok2Typography::ApplyFont(Desc, ERok2TextRole::Micro);
	TextColumn->AddChildToVerticalBox(Desc);

	UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CountText->SetText(FText::FromString(FString::Printf(TEXT("×%d"), Item.Count)));
	CountText->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
	URok2Typography::ApplyFont(CountText, ERok2TextRole::Numeric);
	UHorizontalBoxSlot* CountSlot = Row->AddChildToHorizontalBox(CountText);
	CountSlot->SetPadding(FMargin(Rok2Space::M, Rok2Space::None, Rok2Space::M, Rok2Space::None));
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	if (!Item.bUsable)
	{
		// غير القابل للاستخدام من الحقيبة يُسمّى شاشته بدل زر معطّل بلا تفسير:
		// المفاتيح تُستخدم في الحانة، والمنحوتات في شاشة القائد.
		UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Hint->SetText(FText::FromString(
			Item.UseAction == TEXT("tavern") ? TEXT("يُستخدم في الحانة")
			: Item.UseAction == TEXT("commander") ? TEXT("يُستخدم في شاشة القائد")
			: Item.UseAction == TEXT("equipment") ? TEXT("يُستخدم في المعدات")
			: Item.UseAction == TEXT("city") ? TEXT("يُستخدم في ترقية المباني")
			: Item.UseAction == TEXT("quests") ? TEXT("يُستبدل في المهام")
			: TEXT("للاحتفاظ")));
		Hint->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(Hint, ERok2TextRole::Micro);
		UHorizontalBoxSlot* HintSlot = Row->AddChildToHorizontalBox(Hint);
		HintSlot->SetVerticalAlignment(VAlign_Center);
		HintSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		return;
	}

	URok2BagItemProxy* Proxy = NewObject<URok2BagItemProxy>(this);
	Proxy->ItemId = Item.ItemId;
	Proxy->Owner = this;
	ItemProxies.Add(Proxy);

	UButton* Use = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Use->SetStyle(Rok2Surface::SuccessButton());
	Use->OnClicked.AddDynamic(Proxy, &URok2BagItemProxy::HandleClick);
	URok2MotionLibrary::BindPress(Use);
	UTextBlock* UseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	UseText->SetText(FText::FromString(TEXT("استخدام")));
	UseText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(UseText, ERok2TextRole::Button);
	Use->AddChild(UseText);
	UHorizontalBoxSlot* UseSlot = Row->AddChildToHorizontalBox(Use);
	UseSlot->SetVerticalAlignment(VAlign_Center);
	UseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
}
