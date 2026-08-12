// Copyright ROK2. P6-T10 — Kingdom Story season finale.

#include "Rok2SeasonStoryWidget.h"
#include "Rok2Typography.h"
#include "Rok2AudioManager.h"
#include "Rok2MotionLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

namespace Rok2SeasonStory
{
	static const FLinearColor Ink(0.06f, 0.10f, 0.16f, 0.98f);
	static const FLinearColor Paper(0.95f, 0.91f, 0.78f, 1.f);
	// P7-T7: ألوان WCAG AA مقروءة فوق خلفية داكنة (الخلفية ~#0B1220):
	// Gold يُفتح (1,0.76,0.22 يعطي تباين ~7.5:1)، Crimson وAzure وJade تُفتح لتجاوز 4.5:1
	static const FLinearColor Gold(1.f, 0.80f, 0.34f, 1.f);
	static const FLinearColor Azure(0.52f, 0.78f, 1.0f, 1.f);
	static const FLinearColor Crimson(0.95f, 0.42f, 0.36f, 1.f);
	static const FLinearColor Jade(0.40f, 0.85f, 0.58f, 1.f);
}

void URok2SeasonStoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree || WidgetTree->RootWidget) {
		RebuildTimeline();
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SeasonStoryRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StoryBackdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.68f));
	UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StoryCard"));
	Card->SetBrushColor(Rok2SeasonStory::Ink);
	UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(Card);
	CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	CardSlot->SetSize(FVector2D(820.f, 590.f));

	UVerticalBox* Main = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StoryMain"));
	Card->SetContent(Main);

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StoryHeader"));
	Main->AddChildToVerticalBox(Header)->SetPadding(FMargin(18.f, 14.f, 18.f, 8.f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StoryTitle"));
	Title->SetText(FText::FromString(TEXT("حكاية المملكة")));
	Title->SetColorAndOpacity(FSlateColor(Rok2SeasonStory::Gold));
	URok2Typography::ApplyFont(Title, ERok2TextRole::Title);
	Header->AddChildToHorizontalBox(Title)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UButton* Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseStory"));
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CloseText->SetText(FText::FromString(TEXT("إغلاق")));
	CloseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Close->AddChild(CloseText);
	Close->OnClicked.AddDynamic(this, &URok2SeasonStoryWidget::OnCloseClicked);
	URok2MotionLibrary::BindPress(Close);
	Header->AddChildToHorizontalBox(Close)->SetVerticalAlignment(VAlign_Center);

	ChampionCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionCard"));
	ChampionCard->SetBrushColor(FLinearColor(0.23f, 0.16f, 0.04f, 1.f));
	Main->AddChildToVerticalBox(ChampionCard)->SetPadding(FMargin(18.f, 4.f, 18.f, 10.f));
	UVerticalBox* ChampionContent = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChampionContent"));
	ChampionCard->SetContent(ChampionContent);
	ChampionTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionTitle"));
	ChampionTitle->SetJustification(ETextJustify::Center);
	ChampionTitle->SetColorAndOpacity(FSlateColor(Rok2SeasonStory::Gold));
	URok2Typography::ApplyFont(ChampionTitle, ERok2TextRole::Title);
	ChampionContent->AddChildToVerticalBox(ChampionTitle)->SetPadding(FMargin(8.f, 8.f, 8.f, 2.f));
	ChampionDetail = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionDetail"));
	ChampionDetail->SetJustification(ETextJustify::Center);
	ChampionDetail->SetColorAndOpacity(FSlateColor(Rok2SeasonStory::Paper));
	ChampionContent->AddChildToVerticalBox(ChampionDetail)->SetPadding(FMargin(8.f, 0.f, 8.f, 8.f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("StoryScroll"));
	Main->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Timeline = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StoryTimeline"));
	Scroll->AddChild(Timeline);

	RebuildTimeline();
}

void URok2SeasonStoryWidget::SetStoryEvents(const TArray<FRok2SeasonStoryEntry>& InEvents)
{
	StoryEvents = InEvents;
	StoryEvents.Sort([](const FRok2SeasonStoryEntry& A, const FRok2SeasonStoryEntry& B) {
		return A.SeasonDay == B.SeasonDay ? A.Id < B.Id : A.SeasonDay < B.SeasonDay;
	});
	RebuildTimeline();
}

void URok2SeasonStoryWidget::AddStoryEvent(const FRok2SeasonStoryEntry& InEvent)
{
	if (StoryEvents.ContainsByPredicate([&InEvent](const FRok2SeasonStoryEntry& Existing) { return Existing.Id == InEvent.Id; })) return;
	StoryEvents.Add(InEvent);
	SetStoryEvents(StoryEvents);
}

void URok2SeasonStoryWidget::OnCloseClicked()
{
	if (URok2AudioManager* Audio = URok2AudioManager::Get())
	{
		Audio->PlaySfx(ERok2AudioType::UiPanelClose);
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

FText URok2SeasonStoryWidget::LabelFor(const FRok2SeasonStoryEntry& Event) const
{
	if (Event.Kind == TEXT("region_unlocked")) return FText::FromString(FString::Printf(TEXT("فُتحت المنطقة %s أمام المملكة"), *Event.SubjectId));
	if (Event.Kind == TEXT("first_pass_capture")) return FText::FromString(FString::Printf(TEXT("تحالف %s حقق أول احتلال للممر %s"), *Event.AllianceId, *Event.SubjectId));
	if (Event.Kind == TEXT("pass_conquered")) return FText::FromString(FString::Printf(TEXT("حرب تحالفات: %s انتزع الممر من %s"), *Event.AllianceId, *Event.PreviousAllianceId));
	if (Event.Kind == TEXT("throne_captured")) return FText::FromString(FString::Printf(TEXT("تحالف %s استولى على العرش"), *Event.AllianceId));
	if (Event.Kind == TEXT("season_champion")) return FText::FromString(FString::Printf(TEXT("تُوّج تحالف %s بطلاً للموسم"), *Event.AllianceId));
	return FText::FromString(TEXT("حدث جديد في حكاية المملكة"));
}

FLinearColor URok2SeasonStoryWidget::ColorFor(const FRok2SeasonStoryEntry& Event) const
{
	if (Event.Kind == TEXT("season_champion") || Event.Kind == TEXT("throne_captured")) return Rok2SeasonStory::Gold;
	if (Event.Kind == TEXT("pass_conquered")) return Rok2SeasonStory::Crimson;
	if (Event.Kind == TEXT("first_pass_capture")) return Rok2SeasonStory::Azure;
	if (Event.Kind == TEXT("region_unlocked")) return Rok2SeasonStory::Jade;
	return Rok2SeasonStory::Paper;
}

void URok2SeasonStoryWidget::UpdateChampion()
{
	if (!ChampionCard || !ChampionTitle || !ChampionDetail) return;
	const FRok2SeasonStoryEntry* Champion = StoryEvents.FindByPredicate([](const FRok2SeasonStoryEntry& Event) {
		return Event.Kind == TEXT("season_champion");
	});
	ChampionCard->SetVisibility(Champion ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (!Champion) return;
	ChampionTitle->SetText(FText::FromString(TEXT("تتويج بطل الموسم")));
	ChampionDetail->SetText(FText::FromString(FString::Printf(TEXT("تحالف %s أنهى الموسم بـ %d نقطة"), *Champion->AllianceId, Champion->Score)));
}

void URok2SeasonStoryWidget::RebuildTimeline()
{
	UpdateChampion();
	if (!Timeline) return;
	Timeline->ClearChildren();
	if (StoryEvents.Num() == 0) {
		UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Empty->SetText(FText::FromString(TEXT("لم تبدأ حكاية المملكة بعد — ستظهر هنا المناطق المفتوحة وفتوحات الممرات ومعارك العرش.")));
		Empty->SetAutoWrapText(true);
		Empty->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.70f, 0.78f)));
		Timeline->AddChildToVerticalBox(Empty)->SetPadding(FMargin(24.f, 20.f));
		return;
	}

	for (const FRok2SeasonStoryEntry& Event : StoryEvents) {
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Timeline->AddChildToVerticalBox(Row)->SetPadding(FMargin(18.f, 5.f));
		UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Dot->SetBrushColor(ColorFor(Event));
		Row->AddChildToHorizontalBox(Dot)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		UHorizontalBoxSlot* DotSlot = Cast<UHorizontalBoxSlot>(Dot->Slot);
		if (DotSlot) { DotSlot->SetPadding(FMargin(0.f, 8.f, 10.f, 8.f)); DotSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); }
		Dot->SetPadding(FMargin(5.f));

		UVerticalBox* Copy = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row->AddChildToHorizontalBox(Copy)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* Day = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		// P7-T7: بادئة رمزية لنوع الحدث — تمييز دون الاعتماد على اللون فقط
		const FString DayPrefix = (Event.Kind == TEXT("season_champion") || Event.Kind == TEXT("throne_captured")) ? TEXT("★ ")
			: (Event.Kind == TEXT("pass_conquered") || Event.Kind == TEXT("first_pass_capture")) ? TEXT("⚔ ")
			: (Event.Kind == TEXT("region_unlocked")) ? TEXT("◈ ") : TEXT("• ");
		Day->SetText(FText::FromString(DayPrefix + FString::Printf(TEXT("اليوم %d"), Event.SeasonDay)));
		Day->SetColorAndOpacity(FSlateColor(ColorFor(Event)));
		Copy->AddChildToVerticalBox(Day);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(LabelFor(Event));
		Label->SetAutoWrapText(true);
		Label->SetColorAndOpacity(FSlateColor(Rok2SeasonStory::Paper));
		Copy->AddChildToVerticalBox(Label)->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
	}
}
