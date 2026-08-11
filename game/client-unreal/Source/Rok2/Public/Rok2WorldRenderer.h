// Copyright ROK2. World map renderer (regions/passes/marches/cities).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2Types.h"
#include "Rok2WorldRenderer.generated.h"

class URok2Api;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

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

	/** ينشئ طلب بناء في موضع عالم Unreal؛ تحقق الخادم هو السلطة النهائية للنوع والرتبة والإقليم. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance Structures")
	void RequestAllianceStructureAtWorldPoint(const FString& StructureKind, FVector WorldPoint);

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

	/** أصل موحّد لمؤشر الحصن/المنجنيق/برج المراقبة؛ يُلوّن حسب تحالف المالك. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Alliance Structures")
	UStaticMesh* AllianceStructureMesh;

	/** قرص أو حلقة مسطّحة بقطر 100cm تُستخدم لعرض نطاق الحماية، ويمكن استبدالها بأصل فني نهائي. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Alliance Structures")
	UStaticMesh* ProtectionRadiusMesh;

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

	UPROPERTY(EditAnywhere, Category = "Rok2|Alliance Structures")
	float AllianceStructureZ = 85.f;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	TArray<AActor*> SpawnedActors;

	UPROPERTY(Transient)
	TMap<FString, AActor*> SpawnedMarches;

	TArray<FRok2MarchEntity> CurrentMarches;

	float RefreshTimer = 0.f;

	/** P2-T7: هل زُرعت مرتفعات KayKit حول المدن؟ (مرة واحدة) */
	bool bArtHillsSpawned = false;

	/** P4-T7: ممثلات التلال الحية — تُدار بمفتاح محتوى ولا تُعاد زراعتها كل تحديث. */
	UPROPERTY(Transient)
	TArray<AActor*> SpawnedHills;

	/** P4-T7: مفتاح محتوى مجموعة المدن — التلال تُعاد زراعتها فقط عند تغيّره. */
	int64 ArtHillsKey = 0;

	void ClearActors();
	AActor* SpawnMarkerActor(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);
	void SpawnMarker(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);

	UFUNCTION()
	void OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot);
};
