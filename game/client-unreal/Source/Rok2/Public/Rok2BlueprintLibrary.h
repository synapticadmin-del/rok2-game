// Copyright ROK2. Blueprint Helper Library for Unreal Engine 5.8.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Rok2Types.h"
#include "Rok2BlueprintLibrary.generated.h"


class UUserWidget;
/**
 * ROK2 Blueprint Function Library for Unreal Engine 5.8
 * Exposes core game data, coordinate converters, and utility math to Blueprints.
 */
UCLASS()
class ROK2_API URok2BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get default list of 6 civilizations */
	UFUNCTION(BlueprintPure, Category = "ROK2|Civilizations")
	static TArray<FRok2Civilization> GetDefaultCivilizations();

	/** Get pre-configured Zone 1, Zone 2, and Zone 3 map regions */
	UFUNCTION(BlueprintPure, Category = "ROK2|Map")
	static TArray<FRok2MapRegion> GetDefaultMapRegions();

	/** Convert 2400x2400 2D Map Coordinates to Unreal Engine 3D World Space */
	UFUNCTION(BlueprintPure, Category = "ROK2|Coordinates")
	static FVector WorldToUnrealLocation(FVector2D WorldPos, float ZHeight = 0.0f, float MapScale = 100.0f);

	/** Convert Unreal Engine 3D World Space to 2400x2400 2D Map Coordinates */
	UFUNCTION(BlueprintPure, Category = "ROK2|Coordinates")
	static FVector2D UnrealToWorldLocation(FVector UnrealPos, float MapScale = 100.0f);

	/** Format large resource numbers (e.g. 1.5M, 250K, 1200) */
	UFUNCTION(BlueprintPure, Category = "ROK2|Formatting")
	static FString FormatResourceNumber(double Value);

	/** Calculate distance between two 2D map locations */
	UFUNCTION(BlueprintPure, Category = "ROK2|Math")
	static float GetDistance2D(FVector2D LocationA, FVector2D LocationB);

	/** Safe UMG Widget Factory */
	UFUNCTION(BlueprintCallable, Category = "ROK2|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* CreateRok2Widget(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass);
};
