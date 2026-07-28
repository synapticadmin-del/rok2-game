// Copyright ROK2. Single city building actor (P5-T1).
//
// مبنى واحد داخل المدينة السداسية: له بصمة hex، مستوى، حالة بصرية
// (قيد البناء/مكتمل/إنتاج جاهز/تدريب/جرحى)، وقابل للنقر والسحب في وضع التحرير.
// المواصفة: 07-game-design/castle-hex-city.md §4.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2HexGrid.h"
#include "Rok2BuildingActor.generated.h"

class UStaticMeshComponent;

/** حجم بصمة المبنى على الشبكة. */
UENUM(BlueprintType)
enum class ERok2Footprint : uint8
{
	Small = 0,  // 1 خلية
	Medium,     // 7 خلايا (زهرة)
	Large       // 12 خلية (City Hall / Castle)
};

/** الحالة البصرية للمبنى. */
UENUM(BlueprintType)
enum class ERok2BuildingVisualState : uint8
{
	Complete = 0,   // مكتمل ويعمل
	Constructing,   // قيد البناء (سقالات + شريط تقدم)
	ReadyToCollect, // إنتاج جاهز للجمع (فقاعة مورد)
	Training,       // تدريب جارٍ (أعلام)
	HasWounded      // جرحى موجودون (مستشفى — صليب نابض)
};

UCLASS()
class ROK2_API ARok2BuildingActor : public AActor
{
	GENERATED_BODY()

public:
	ARok2BuildingActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UStaticMeshComponent* Mesh;

	/** مؤشر الحالة العائم (فقاعة/صليب/أعلام) — مكعب placeholder حالياً. */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UStaticMeshComponent* StatusIndicator;

	/** معرف المبنى (city_hall, farm, barracks ...) */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	FString BuildingId;

	/** مستوى المبنى الحالي. */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	int32 Level = 1;

	/** بصمة المبنى. */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	ERok2Footprint Footprint = ERok2Footprint::Small;

	/** خلية مركز المبنى على الشبكة. */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	FRok2HexCell AnchorCell;

	/** دوران المبنى بمضاعفات 60°. */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	int32 RotationSteps = 0;

	/** الحالة البصرية الحالية. */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	ERok2BuildingVisualState VisualState = ERok2BuildingVisualState::Complete;

	/** هل المبنى ثابت (City Hall) لا يُنقل؟ */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	bool bIsStatic = false;

	/** تهيئة المبنى بمعرفه ومستواه وخليته. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(const FString& InId, int32 InLevel, const FRok2HexCell& InCell, float HexSize);

	/** ضبط الحالة البصرية وتحديث المؤشر العائم. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetVisualState(ERok2BuildingVisualState NewState);

	/** خلايا بصمة المبنى حول مركزه الحالي. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	TArray<FRok2HexCell> OccupiedCells() const;

	/** هل المبنى يشغل خلية معينة؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool Occupies(const FRok2HexCell& Cell) const;

	/** نصف قطر البصمة بالخلايا (0 لصغير، 1 لوسط، 2 لكبير تقريباً). */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	int32 FootprintRadius() const;

	/** حجم الخلية المخزن. */
	UPROPERTY(Transient)
	float CachedHexSize = 260.f;

protected:
	UFUNCTION()
	void OnClicked(AActor* TouchedActor, FKey ButtonPressed);

	void UpdateStatusIndicator();
	float FootprintWorldScale() const;
};
