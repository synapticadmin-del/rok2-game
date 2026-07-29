// Copyright ROK2. City layout manager actor (P5-T1).
//
// يملك مدينة اللاعب السداسية بالكامل: شبكة hex + السور + المباني + التخطيط المحفوظ.
// يقرأ مباني اللاعب من URok2Api ويولّدها على الشبكة، ويدير وضع التحرير (City Editor).
// المواصفة: 07-game-design/castle-hex-city.md §7.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2HexGrid.h"
#include "Rok2BuildingActor.h"
#include "Rok2Types.h"
#include "Rok2CityLayoutActor.generated.h"

class URok2Api;
class ARok2HexWallActor;
class UInstancedStaticMeshComponent;

/** موضع مبنى محفوظ في التخطيط (يُرسل/يُستقبل للخادم). */
USTRUCT(BlueprintType)
struct FRok2BuildingPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 R = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 RotationSteps = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRok2OnBuildingPicked, const FString&, BuildingId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRok2OnLayoutChanged);

UCLASS()
class ROK2_API ARok2CityLayoutActor : public AActor
{
	GENERATED_BODY()

public:
	ARok2CityLayoutActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	/** بلاط الأرض السداسي (instanced) */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* GroundHexes;

	/** خلايا الإبراز (صلاحية/تظليل في وضع التحرير) */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* HighlightHexes;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* HexTileMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UMaterialInterface* GroundMaterial;

	/** نصف قطر المدينة بالخلايا (يتوسع مع مستوى City Hall). */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	int32 CityRadiusCells = 7;

	/** حجم الخلية (سمك hex). */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	float HexSize = 260.f;

	/** السور السداسي. */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	ARok2HexWallActor* Wall;

	/** المباني المزروعة حالياً (BuildingId → Actor). */
	UPROPERTY(Transient)
	TMap<FString, ARok2BuildingActor*> Buildings;

	/** هل وضع التحرير نشط؟ */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	bool bEditMode = false;

	/** يُبنى التخطيط من حالة الـ API (يُستدعى عند تحميل المدينة). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RebuildFromApi();

	/** تفعيل/إيقاف وضع التحرير. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetEditMode(bool bEnable);

	/** محاولة نقل مبنى لخلية جديدة (يتحقق من الصلاحية). تعيد true عند النجاح. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	bool TryMoveBuilding(const FString& BuildingId, const FRok2HexCell& NewCell);

	/** تدوير مبنى بمقدار 60°. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RotateBuilding(const FString& BuildingId);

	/** التخطيط الحالي كقائمة مواضع (للحفظ على الخادم). */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	TArray<FRok2BuildingPlacement> GetLayoutPlacements() const;

	/** حفظ التخطيط على الخادم (POST /v1/city/layout). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SaveLayoutToServer();

	/** نصف قطر المدينة المناسب لمستوى City Hall (قاعدة التوسع البصري §6). */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	static int32 RadiusForCityHallLevel(int32 CityHallLevel);

	/** يُطلق عند لمس مبنى. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FRok2OnBuildingPicked OnBuildingPicked;

	/** يُطلق عند تغيّر التخطيط (نقل/تدوير). */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FRok2OnLayoutChanged OnLayoutChanged;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	/** P4-T7: مبانٍ مخفية قابلة لإعادة الاستخدام عبر إعادة البناء (pool محلي خفيف). */
	UPROPERTY(Transient)
	TMap<FString, ARok2BuildingActor*> RecycledBuildings;

	UFUNCTION()
	void OnCityLoadedHandler(const FRok2City& City);

	UFUNCTION()
	void OnAnyBuildingClicked(AActor* TouchedActor, FKey ButtonPressed);

	void BuildGround();
	void SpawnWall();
	void SpawnBuildings();
	void ClearBuildings();

	/** هل يمكن وضع مبنى ببصمته على خلية (داخل السور وبلا تراكب)؟ */
	bool CanPlaceAt(const FString& BuildingId, ERok2Footprint Footprint, const FRok2HexCell& Cell, const FString& IgnoreBuildingId) const;

	/** تخطيط افتراضي دائري عند غياب layout محفوظ (توزيع حلقي حول المركز). */
	FRok2HexCell DefaultCellForIndex(int32 Index, int32 Total) const;

	void RefreshHighlights(const FString& ForBuildingId);
};
