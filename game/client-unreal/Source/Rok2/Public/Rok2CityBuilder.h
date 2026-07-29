// Copyright ROK2. City building system — hex castle orchestrator (P5-T1).
//
// يملك شاشة المدينة: يزرع ويدير CityLayoutActor (الشبكة + السور + المباني)
// و CityEditorMode (وضع التحرير)، ويربط لمس المباني ببطاقة التفاصيل.
// حُدّث من النظام الشبكي القديم إلى السور السداسي والبناء الحر.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2HexGrid.h"
#include "Rok2Types.h"
#include "Rok2CityBuilder.generated.h"

class URok2Api;
class UInstancedStaticMeshComponent;
class ARok2CityLayoutActor;
class ARok2CityEditorMode;
class USceneComponent;
class UStaticMesh;
class UMaterialInterface;

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

	/** إعادة بناء المدينة من حالة الـ API. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Rebuild();

	/** تفعيل/إيقاف وضع تحرير المدينة (يُستدعى من HUD). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ToggleEditMode();

	/** هل وضع التحرير نشط؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsEditModeActive() const;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	/** مدير التخطيط السداسي (الشبكة + السور + المباني). */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	ARok2CityLayoutActor* Layout;

	/** وضع التحرير (سحب/إفلات/تدوير/تخطيطات). */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	ARok2CityEditorMode* Editor;

	// --- إعدادات قديمة تُحفظ للتوافق مع المستوى (لا تُستخدم مع النظام الجديد) ---
	UPROPERTY(EditAnywhere, Category = "Rok2|Legacy")
	UStaticMesh* GroundTileMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2|Legacy")
	UMaterialInterface* GroundMaterial;

	UPROPERTY(EditAnywhere, Category = "Rok2|Legacy")
	int32 GridSize = 8;

	UPROPERTY(EditAnywhere, Category = "Rok2|Legacy")
	float TileWorldSize = 400.f;

	UPROPERTY(EditAnywhere, Category = "Rok2|Legacy")
	TArray<FRok2BuildingVisual> BuildingVisuals;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	bool bRebuildQueued = false;
	float RefreshTimer = 0.f;

	UFUNCTION()
	void OnCityLoadedHandler(const FRok2City& City);

	UFUNCTION()
	void OnBuildingPickedHandler(const FString& BuildingId);
};
