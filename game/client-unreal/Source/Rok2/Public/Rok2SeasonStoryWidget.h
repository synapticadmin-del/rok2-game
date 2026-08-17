// Copyright ROK2. P6-T10 — Kingdom Story season finale.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2SeasonStoryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UVerticalBox;

/**
 * A self-contained modal presenting the season timeline supplied by the shard.
 * The host only needs to call SetStoryEvents when it receives snapshot.seasonStory
 * or a season_story_event message; no private combat report is required.
 */
UCLASS()
class ROK2_API URok2SeasonStoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ROK2|Season Story")
	void SetStoryEvents(const TArray<FRok2SeasonStoryEntry>& InEvents);

	UFUNCTION(BlueprintCallable, Category = "ROK2|Season Story")
	void AddStoryEvent(const FRok2SeasonStoryEntry& InEvent);

	UFUNCTION(BlueprintPure, Category = "ROK2|Season Story")
	const TArray<FRok2SeasonStoryEntry>& GetStoryEvents() const { return StoryEvents; }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	UVerticalBox* Timeline = nullptr;

	UPROPERTY(Transient)
	UBorder* ChampionCard = nullptr;

	UPROPERTY(Transient)
	UTextBlock* ChampionTitle = nullptr;

	UPROPERTY(Transient)
	UTextBlock* ChampionDetail = nullptr;

	/**
	 * P24-T5: لوحة فصل الموسم خلف البطاقة. الصور الأربع في
	 * Content/Art/SeasonStory كانت مولّدة بلا قارئ، فكانت شاشة الحكاية نصاً
	 * على لون واحد.
	 */
	UPROPERTY(Transient)
	UImage* BackdropImage = nullptr;

	/** آخر فصل مُحمَّل — يمنع إعادة التحميل عند كل حدث يصل من الخادم. */
	FString LastBackdropAsset;

	UPROPERTY()
	TArray<FRok2SeasonStoryEntry> StoryEvents;

	UFUNCTION()
	void OnCloseClicked();

	void RebuildTimeline();
	void UpdateChampion();

	/** يختار لوحة الفصل حسب ما جرى في الموسم ويحمّلها عند التغيّر فقط. */
	void RefreshBackdrop();

	FText LabelFor(const FRok2SeasonStoryEntry& Event) const;
	FLinearColor ColorFor(const FRok2SeasonStoryEntry& Event) const;
};
