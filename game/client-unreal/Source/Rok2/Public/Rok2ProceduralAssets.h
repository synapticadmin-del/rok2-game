// Copyright ROK2. Procedural materials generated at runtime (no external textures needed for prototype).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
UENUM(BlueprintType)
enum class ERok2MaterialType : uint8
{
	GroundTile,
	Building,
	Pass,
	Node,
	City,
	Mountain,
	Water
};

#include "Rok2ProceduralAssets.generated.h"


class UMaterialInstanceDynamic;
class UMaterialInterface;
UCLASS()
class ROK2_API URok2ProceduralAssets : public UObject
{
	GENERATED_BODY()

public:
	/** Creates or returns a shared instance of procedural materials. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2ProceduralAssets* Get();

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* GroundTileMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BuildingMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PassMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* NodeMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* CityMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MountainMat;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* WaterMat;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Init();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* GetMaterial(ERok2MaterialType Type);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UMaterialInstanceDynamic* CreateTintedMaterial(const FLinearColor& Color);

protected:
	UPROPERTY(Transient)
	UMaterialInterface* BaseMaterial;

	bool bInitialized = false;

	void EnsureInit();

	UMaterialInstanceDynamic* CreateDynInternal(const FLinearColor& Color);
};
