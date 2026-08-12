// Copyright ROK2. P6-T10 — Kingdom Story season finale.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2SeasonStoryWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;

/** Client representation of the public `seasonStory` events emitted by KingdomShard. */
USTRUCT(BlueprintType)
struct FRok2SeasonStoryEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	FString Id;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	FString Kind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	int32 SeasonDay = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	FString SubjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	FString AllianceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	FString PreviousAllianceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ROK2|Season Story")
	int32 Score = 0;
};

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

private:
	UPROPERTY(Transient)
	UVerticalBox* Timeline = nullptr;

	UPROPERTY(Transient)
	UBorder* ChampionCard = nullptr;

	UPROPERTY(Transient)
	UTextBlock* ChampionTitle = nullptr;

	UPROPERTY(Transient)
	UTextBlock* ChampionDetail = nullptr;

	UPROPERTY()
	TArray<FRok2SeasonStoryEntry> StoryEvents;

	UFUNCTION()
	void OnCloseClicked();

	void RebuildTimeline();
	void UpdateChampion();
	FText LabelFor(const FRok2SeasonStoryEntry& Event) const;
	FLinearColor ColorFor(const FRok2SeasonStoryEntry& Event) const;
};
