// Copyright ROK2. City building system (isometric grid + visual placement).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2CityBuilder.generated.h"

class URok2Api;
class UInstancedStaticMeshComponent;

USTRUCT(BlueprintType)
struct FRok2BuildingVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Rok2")
	FString BuildingId;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	FVector GridOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS()
class ARok2CityBuilder : public AActor
{
	GENERATED_BODY()

public:
	ARok2CityBuilder();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Rebuild visual city from current API state. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Rebuild();

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* GroundTiles;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* GroundTileMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UMaterialInterface* GroundMaterial;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	int32 GridSize = 8;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float TileWorldSize = 400.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	TArray<FRok2BuildingVisual> BuildingVisuals;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	TMap<FString, AActor*> SpawnedBuildings;

	bool bRebuildQueued = false;
	float RefreshTimer = 0.f;

	UFUNCTION()
	void OnCityLoadedHandler(const FRok2City& City);

	UFUNCTION()
	void OnBuildingClicked(AActor* TouchedActor, FKey ButtonPressed);
};
