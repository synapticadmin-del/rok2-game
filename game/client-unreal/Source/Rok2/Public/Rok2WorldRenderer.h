// Copyright ROK2. World map renderer (regions/passes/marches/cities).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2Types.h"
#include "Rok2WorldRenderer.generated.h"

class URok2Api;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class ARok2WorldRenderer : public AActor
{
	GENERATED_BODY()

public:
	ARok2WorldRenderer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RefreshFromApi();

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* GroundMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* CityMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* PassMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* NodeMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* MarchMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float WorldToUnrealScale = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UHierarchicalInstancedStaticMeshComponent* GroundHISM;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UHierarchicalInstancedStaticMeshComponent* CityHISM;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UHierarchicalInstancedStaticMeshComponent* PassHISM;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UHierarchicalInstancedStaticMeshComponent* ResourceNodeHISM;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UHierarchicalInstancedStaticMeshComponent* BarbarianNodeHISM;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float CityZ = 0.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float PassZ = 100.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float NodeZ = 50.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float MarchZ = 75.f;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	TArray<AActor*> SpawnedActors;

	UPROPERTY(Transient)
	TMap<FString, AActor*> SpawnedMarches;

	TArray<FRok2MarchEntity> CurrentMarches;

	float RefreshTimer = 0.f;

	void ClearActors();
	AActor* SpawnMarkerActor(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);
	void SpawnMarker(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);

	UFUNCTION()
	void OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot);
};
