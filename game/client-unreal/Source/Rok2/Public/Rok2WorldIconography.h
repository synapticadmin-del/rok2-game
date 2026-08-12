// Copyright ROK2. World marker iconography (P6-T9).
//
// This library is deliberately renderer-agnostic: ARok2WorldRenderer may consume
// its canonical id, glyph, colours and scale when it replaces placeholder meshes.

#pragma once

#include "CoreMinimal.h"
#include "Rok2WorldIconography.generated.h"

USTRUCT(BlueprintType)
struct FRok2WorldIconStyle
{
	GENERATED_BODY()

	/** Stable visual id, suitable for a future mesh, sprite, or widget registry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	FName IconId = TEXT("world_marker");

	/** Localized-neutral fallback glyph for a billboard or debug label. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	FString Glyph = TEXT("•");

	/** Primary material tint: resource family or strategic-object family. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	FLinearColor BaseColor = FLinearColor::White;

	/** Bright ring/emissive tint that communicates the normalized level tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	FLinearColor TierColor = FLinearColor::White;

	/** Suggested uniform scale for tactical icon geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	float WorldScale = 1.f;

	/** 1=entry, 2=contested, 3=elite/final. Never zero. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ROK2|World Icons")
	int32 Tier = 1;
};

/**
 * Canonical P6-T9 icon grammar for map objectives.  The resolver understands
 * the node categories supplied by the world snapshot and the level ranges in
 * data/zones.json (resources 1..6; passes 1..5; throne = final objective).
 */
UCLASS()
class ROK2_API URok2WorldIconography : public UObject
{
	GENERATED_BODY()

public:
	/** Resolve the icon style for resource/barbarian/pass/throne targets. */
	UFUNCTION(BlueprintPure, Category = "ROK2|World Icons")
	static FRok2WorldIconStyle Resolve(const FString& TargetType, const FString& ResourceKind, int32 Level);

	/** Normalizes a raw level into the three readable map tiers. */
	UFUNCTION(BlueprintPure, Category = "ROK2|World Icons")
	static int32 TierForLevel(int32 Level, int32 MaximumLevel);

	/** Gives a stable display identity for resource kinds supplied by the server. */
	UFUNCTION(BlueprintPure, Category = "ROK2|World Icons")
	static FName ResourceIconId(const FString& ResourceKind);
};
