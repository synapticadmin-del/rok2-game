// Copyright ROK2. شاشة الحانة والصناديق (P19-T4) — implementation.
// P6-T3: الحركة كلها من `URok2MotionLibrary` — دخول من المركز، وضغطة محسوسة على
// كل زر، ووميض ذهبي عند رمية أسطورية، وتسريح بـ`PlayFadeOut`.

#include "Rok2TavernWidget.h"
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
#include "Engine/Texture2D.h"

void URok2TavernBoxProxy::HandleClick()
{
	if (Owner)
	{
		Owner->RequestOpenBox(BoxId);
	}
}

void URok2TavernWidget::Setup(URok2Api* InApi)
{
	Api = InApi;
	if (!Api) return;

	Api->OnTavernUpdated.AddUniqueDynamic(this, &URok2TavernWidget::OnTavernUpdated);
	// المفاتيح تُمنح من المهام اليومية بين فتحة وأخرى؛ لقطة قديمة تُري اللاعب
	// رصيداً لا يملكه ثم يفشل الفتح.
	Api->FetchTavernState();
}

TSharedRef<SWidget> URok2TavernWidget::RebuildWidget()
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

void URok2TavernWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) return;

	UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TavernRoot"));
	WidgetTree->RootWidget = RootPanel;

	UButton* Backdrop = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TavernBackdrop"));
	Backdrop->SetStyle(Rok2Surface::GhostButton());
	Backdrop->SetColorAndOpacity(Rok2Visual::Scrim());
	Backdrop->OnClicked.AddDynamic(this, &URok2TavernWidget::OnCloseClicked);
	UCanvasPanelSlot* BackdropSlot = RootPanel->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));

	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TavernSheet"));
	Sheet->SetBrush(Rok2Surface::Sheet());
	Sheet->SetPadding(FMargin(Rok2Space::L));
	UCanvasPanelSlot* SheetSlot = RootPanel->AddChildToCanvas(Sheet);
	SheetSlot->SetAnchors(FAnchors(0.06f, 0.08f, 0.94f, 0.94f));
	SheetSlot->SetOffsets(FMargin(0.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TavernColumn"));
	Sheet->SetContent(Column);

	// ── الترويسة ──
	{
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("beer"), 24.f, Rok2Visual::GoldText()));
		Ico->SetDesiredSizeOverride(FVector2D(24.f, 24.f));
		Ico->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("beer")));
		UHorizontalBoxSlot* IcoSlot = Header->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Title->SetText(FText::FromString(TEXT("الحانة")));
		Title->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
		Header->AddChildToHorizontalBox(Title)->SetVerticalAlignment(VAlign_Center);

		USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
		Header->AddChildToHorizontalBox(Gap)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TavernClose"));
		Close->SetStyle(Rok2Surface::SecondaryButton());
		Close->SetToolTipText(URok2Accessibility::LabelForIcon(TEXT("close")));
		Close->OnClicked.AddDynamic(this, &URok2TavernWidget::OnCloseClicked);
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

	// ── سطر الحالة: سقف الفتح في الساعة + المفتاح اليومي ──
	{
		UHorizontalBox* StatusRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Column->AddChildToVerticalBox(StatusRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::M));

		HourlyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TavernHourlyText"));
		HourlyText->SetText(FText::FromString(TEXT("—")));
		HourlyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(HourlyText, ERok2TextRole::Caption);
		UHorizontalBoxSlot* HourlySlot = StatusRow->AddChildToHorizontalBox(HourlyText);
		HourlySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HourlySlot->SetVerticalAlignment(VAlign_Center);

		DailyKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TavernDailyKeyText"));
		DailyKeyText->SetText(FText::FromString(TEXT("")));
		DailyKeyText->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(DailyKeyText, ERok2TextRole::Caption);
		UHorizontalBoxSlot* DailyTextSlot = StatusRow->AddChildToHorizontalBox(DailyKeyText);
		DailyTextSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		DailyTextSlot->SetVerticalAlignment(VAlign_Center);
		DailyTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		DailyKeyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TavernDailyKeyBtn"));
		DailyKeyButton->SetStyle(Rok2Surface::PrimaryButton());
		DailyKeyButton->OnClicked.AddDynamic(this, &URok2TavernWidget::OnDailyKeyClicked);
		URok2MotionLibrary::BindPress(DailyKeyButton);
		UTextBlock* DailyBtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		DailyBtnText->SetText(FText::FromString(TEXT("مفتاح اليوم")));
		DailyBtnText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(DailyBtnText, ERok2TextRole::Button);
		DailyKeyButton->AddChild(DailyBtnText);
		UHorizontalBoxSlot* DailyBtnSlot = StatusRow->AddChildToHorizontalBox(DailyKeyButton);
		DailyBtnSlot->SetVerticalAlignment(VAlign_Center);
		DailyBtnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// ── الصناديق ──
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("TavernBoxScroll"));
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		BoxesList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TavernBoxes"));
		Scroll->AddChild(BoxesList);
	}

	// ── نتيجة آخر فتح ──
	{
		UTextBlock* ResultsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ResultsTitle->SetText(FText::FromString(TEXT("نتيجة آخر فتح")));
		ResultsTitle->SetColorAndOpacity(FSlateColor(Rok2Visual::GoldText()));
		URok2Typography::ApplyFont(ResultsTitle, ERok2TextRole::Subtitle);
		Column->AddChildToVerticalBox(ResultsTitle)->SetPadding(FMargin(Rok2Space::None, Rok2Space::M, Rok2Space::None, Rok2Space::XS));

		ResultsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TavernResults"));
		Column->AddChildToVerticalBox(ResultsList);
	}

	RebuildBoxes();
	RebuildResults();
	UpdateStatus();
	URok2MotionLibrary::PlayScaleInCenter(Sheet);
	URok2MotionLibrary::PlayFadeIn(Backdrop);
}

void URok2TavernWidget::OnCloseClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	URok2MotionLibrary::PlayFadeOut(this);
}

void URok2TavernWidget::OnDailyKeyClicked()
{
	if (Api)
	{
		Api->ClaimTavernDailyKey();
	}
}

void URok2TavernWidget::RequestOpenBox(const FString& BoxId)
{
	if (!Api || BoxId.IsEmpty()) return;

	// صوت الدوران عند الطلب لا عند الوصول: رد الفعل يجب أن يكون فورياً
	// (<100ms، قاعدة §8.6) والاستجابة تعبر الشبكة.
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::WheelSpin);
	}
	Api->OpenTavernBox(BoxId);
}

void URok2TavernWidget::OnTavernUpdated(const FRok2TavernState& State)
{
	// رميات جديدة = فتحة حقيقية. المقارنة تمنع تشغيل صوت الصندوق عند كل تحديث
	// حالة (`FetchTavernState` يبثّ الحدث نفسه عند فتح الشاشة).
	const bool bNewRolls = State.LastRolls.Num() > 0 && State.LastRolls.Num() != LastRollCount;
	LastRollCount = State.LastRolls.Num();

	RebuildBoxes();
	RebuildResults();
	UpdateStatus();

	if (bNewRolls)
	{
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::ChestOpen);
		}
	}
}

// ---------------------------------------------------------------------------
// البناء
// ---------------------------------------------------------------------------

void URok2TavernWidget::RebuildBoxes()
{
	if (!BoxesList || !WidgetTree || !Api) return;

	BoxesList->ClearChildren();
	Proxies.Empty();

	const FRok2TavernState& State = Api->GetTavernState();

	// الصناديق الثلاثة ومفاتيحها تطابق `data/tavern.json` — الأسماء العربية
	// هناك، لكن حمولة `tavern-state` تعيد الأرصدة وحدها فلا كتالوج يصل. حتى
	// يُرسله الخادم (P19-T4 لا يغيّر عقد `/v1/tavern/state`) تبقى الأسماء هنا
	// مطابقةً حرفياً لملف البيانات ويحرسها الفحص البنيوي.
	BuildBoxCard(TEXT("silver_box"), TEXT("صندوق فضي"), TEXT("silver_key"), State.Keys.FindRef(TEXT("silver_key")));
	BuildBoxCard(TEXT("gold_box"), TEXT("صندوق ذهبي"), TEXT("gold_key"), State.Keys.FindRef(TEXT("gold_key")));
	BuildBoxCard(TEXT("gear_box"), TEXT("صندوق معدات"), TEXT("gear_key"), State.Keys.FindRef(TEXT("gear_key")));
}

void URok2TavernWidget::BuildBoxCard(const FString& BoxId, const FString& BoxName, const FString& KeyId, int32 KeysHeld)
{
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Card->SetBrush(Rok2Surface::Card());
	Card->SetPadding(FMargin(Rok2Space::M));
	BoxesList->AddChildToVerticalBox(Card)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::S));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Card->SetContent(Row);

	// صورة الصندوق المستوردة — `LoadTavernIcon` هي الدالة التي لم يكن لها
	// مستدعٍ واحد في المشروع. عند غياب الأصل نسقط إلى أيقونة إجرائية.
	UImage* BoxImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* Texture = URok2ArtAssets::LoadTavernIcon(
		BoxId == TEXT("silver_box") ? TEXT("chest_silver")
		: BoxId == TEXT("gold_box") ? TEXT("chest_gold")
		: TEXT("chest_equipment")))
	{
		BoxImage->SetBrushFromTexture(Texture, false);
		BoxImage->SetDesiredSizeOverride(FVector2D(64.f, 64.f));
	}
	else
	{
		BoxImage->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("box"), 48.f, Rok2Visual::GoldText()));
		BoxImage->SetDesiredSizeOverride(FVector2D(48.f, 48.f));
	}
	BoxImage->SetToolTipText(FText::FromString(BoxName));
	UHorizontalBoxSlot* ImageSlot = Row->AddChildToHorizontalBox(BoxImage);
	ImageSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::M, Rok2Space::None));
	ImageSlot->SetVerticalAlignment(VAlign_Center);
	ImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextColumn);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(BoxName));
	NameText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(NameText, ERok2TextRole::CardTitle);
	TextColumn->AddChildToVerticalBox(NameText);

	// صف المفتاح: صورته المستوردة + الرصيد. الرصيد صفر يُقال صريحاً لا يُخفى.
	UHorizontalBox* KeyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TextColumn->AddChildToVerticalBox(KeyRow)->SetPadding(FMargin(Rok2Space::None, Rok2Space::XS, Rok2Space::None, Rok2Space::None));

	UImage* KeyImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (UTexture2D* KeyTexture = URok2ArtAssets::LoadTavernIcon(
		KeyId == TEXT("silver_key") ? TEXT("key_silver")
		: KeyId == TEXT("gold_key") ? TEXT("key_gold")
		: TEXT("key_equipment")))
	{
		KeyImage->SetBrushFromTexture(KeyTexture, false);
	}
	else
	{
		KeyImage->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("gift"), 18.f, Rok2Visual::Muted()));
	}
	KeyImage->SetDesiredSizeOverride(FVector2D(18.f, 18.f));
	UHorizontalBoxSlot* KeyImageSlot = KeyRow->AddChildToHorizontalBox(KeyImage);
	KeyImageSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::XS, Rok2Space::None));
	KeyImageSlot->SetVerticalAlignment(VAlign_Center);
	KeyImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	KeyText->SetText(FText::FromString(FString::Printf(TEXT("المفاتيح: %d"), KeysHeld)));
	KeyText->SetColorAndOpacity(FSlateColor(KeysHeld > 0 ? Rok2Visual::SuccessText() : Rok2Visual::Muted()));
	URok2Typography::ApplyFont(KeyText, ERok2TextRole::Micro);
	KeyRow->AddChildToHorizontalBox(KeyText)->SetVerticalAlignment(VAlign_Center);

	URok2TavernBoxProxy* Proxy = NewObject<URok2TavernBoxProxy>(this);
	Proxy->BoxId = BoxId;
	Proxy->Owner = this;
	Proxies.Add(Proxy);

	UButton* Open = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Open->SetStyle(Rok2Surface::PrimaryButton());
	Open->OnClicked.AddDynamic(Proxy, &URok2TavernBoxProxy::HandleClick);
	URok2MotionLibrary::BindPress(Open);
	// معطّل بصرياً عند غياب المفتاح — الخادم يرفض دائماً (`spendKey` → `no_key`)،
	// لكن زراً يبدو جاهزاً ثم يفشل يُعلّم اللاعب ألّا يثق بالواجهة.
	Open->SetIsEnabled(KeysHeld > 0);
	UTextBlock* OpenText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OpenText->SetText(FText::FromString(KeysHeld > 0 ? TEXT("فتح") : TEXT("لا مفتاح")));
	OpenText->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
	URok2Typography::ApplyFont(OpenText, ERok2TextRole::Button);
	Open->AddChild(OpenText);
	UHorizontalBoxSlot* OpenSlot = Row->AddChildToHorizontalBox(Open);
	OpenSlot->SetVerticalAlignment(VAlign_Center);
	OpenSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
}

void URok2TavernWidget::RebuildResults()
{
	if (!ResultsList || !WidgetTree || !Api) return;

	ResultsList->ClearChildren();
	const FRok2TavernState& State = Api->GetTavernState();

	if (State.LastRolls.Num() == 0)
	{
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Empty->SetText(FText::FromString(TEXT("لم تفتح صندوقاً بعد.")));
		Empty->SetColorAndOpacity(FSlateColor(Rok2Visual::Muted()));
		URok2Typography::ApplyFont(Empty, ERok2TextRole::Micro);
		ResultsList->AddChildToVerticalBox(Empty);
		return;
	}

	for (const FRok2TavernRoll& Roll : State.LastRolls)
	{
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		// النُدرة تحدد لون الحافة: `legendary` أعلى درجة و`common` أدناها.
		const int32 Rarity = Roll.Kind == TEXT("legendary") ? 5
			: Roll.Kind == TEXT("epic") ? 4
			: Roll.Kind == TEXT("rare") ? 3
			: Roll.Kind == TEXT("materials") ? 2 : 1;
		Card->SetBrush(Rok2Surface::AccentCard(Rok2Visual::RarityTier(Rarity)));
		Card->SetPadding(FMargin(Rok2Space::S));
		ResultsList->AddChildToVerticalBox(Card)->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::None, Rok2Space::XS));

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Card->SetContent(Row);

		UImage* Ico = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (UTexture2D* Texture = URok2ArtAssets::LoadTavernIcon(RollKindIcon(Roll.Kind)))
		{
			Ico->SetBrushFromTexture(Texture, false);
		}
		else
		{
			Ico->SetBrush(URok2ArtAssets::GetIconBrush(TEXT("star"), 20.f, Rok2Visual::RarityTier(Rarity)));
		}
		Ico->SetDesiredSizeOverride(FVector2D(20.f, 20.f));
		UHorizontalBoxSlot* IcoSlot = Row->AddChildToHorizontalBox(Ico);
		IcoSlot->SetPadding(FMargin(Rok2Space::None, Rok2Space::None, Rok2Space::S, Rok2Space::None));
		IcoSlot->SetVerticalAlignment(VAlign_Center);
		IcoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Roll.Quantity > 1
			? FString::Printf(TEXT("%s ×%d"), *RollKindName(Roll.Kind), Roll.Quantity)
			: RollKindName(Roll.Kind)));
		Label->SetColorAndOpacity(FSlateColor(Rok2Visual::Ivory()));
		URok2Typography::ApplyFont(Label, ERok2TextRole::BodySmall);
		Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);

		// وميض ذهبي للرمية الأسطورية — §1 «كل تأكيد له وميض ذهبي»، وهذا أجدر
		// موضع به في اللعبة.
		if (Roll.Kind == TEXT("legendary"))
		{
			URok2MotionLibrary::PlayGoldFlash(Card, Rok2Visual::Card());
		}
	}
}

void URok2TavernWidget::UpdateStatus()
{
	if (!Api) return;
	const FRok2TavernState& State = Api->GetTavernState();

	if (HourlyText)
	{
		// السقف من الخادم (`maxOpensPerHour` في tavern.json) ولا نعرفه في
		// العميل، فنعرض المستهلك وحده بلا اختراع مقام.
		HourlyText->SetText(FText::FromString(
			FString::Printf(TEXT("فتحات هذه الساعة: %d"), State.OpensThisHour)));
	}

	if (DailyKeyText)
	{
		DailyKeyText->SetText(FText::FromString(State.DailyKeyClaimed
			? TEXT("مفتاح اليوم مُستلَم")
			: TEXT("مفتاح فضي مجاني متاح")));
		DailyKeyText->SetColorAndOpacity(FSlateColor(State.DailyKeyClaimed
			? Rok2Visual::Muted()
			: Rok2Visual::SuccessText()));
	}

	if (DailyKeyButton)
	{
		DailyKeyButton->SetIsEnabled(!State.DailyKeyClaimed);
	}
}

FString URok2TavernWidget::RollKindName(const FString& Kind) const
{
	// أسماء فئات الرميات كما في `tavern.json` (`description` لكل مدخل في
	// `pool`) — المعرّف اللاتيني نفسه احتياطٌ صادق لفئة لم تُسمَّ بعد.
	if (Kind == TEXT("common")) return TEXT("موارد وخبرة");
	if (Kind == TEXT("rare")) return TEXT("منحوتات قائد نادر");
	if (Kind == TEXT("materials")) return TEXT("مواد");
	if (Kind == TEXT("epic")) return TEXT("قائد Epic");
	if (Kind == TEXT("legendary")) return TEXT("قائد أسطوري");
	return Kind;
}

FString URok2TavernWidget::RollKindIcon(const FString& Kind) const
{
	// من حزمة `Content/Art/Tavern` — منحوتات للقادة ومواد للمواد.
	if (Kind == TEXT("legendary")) return TEXT("sculpture_legendary");
	if (Kind == TEXT("epic")) return TEXT("sculpture_epic");
	if (Kind == TEXT("rare")) return TEXT("sculpture_elite");
	if (Kind == TEXT("materials")) return TEXT("material_iron");
	return TEXT("sculpture_advanced");
}
