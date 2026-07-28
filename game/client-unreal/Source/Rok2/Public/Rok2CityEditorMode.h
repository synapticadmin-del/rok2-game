// Copyright ROK2. City Editor mode controller (P5-T1).
//
// يدير وضع تحرير المدينة: سحب المباني بشبح أخضر/أحمر، إفلات على الخلايا،
// تدوير 60°، إعادة للمخزن، و3 تخطيطات محفوظة قابلة للتبديل الفوري.
// المواصفة: 07-game-design/castle-hex-city.md §3.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2HexGrid.h"
#include "Rok2BuildingActor.h"
#include "Rok2CityLayoutActor.h"
#include "Rok2CityEditorMode.generated.h"

class ARok2BuildingActor;
class ARok2CityLayoutActor;

/** تخطيط محفوظ (اسم + مواضع). */
USTRUCT(BlueprintType)
struct FRok2SavedLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2BuildingPlacement> Placements;
};

UCLASS()
class ROK2_API ARok2CityEditorMode : public AActor
{
	GENERATED_BODY()

public:
	ARok2CityEditorMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	/** المدير المالك للشبكة والمباني. */
	UPROPERTY(Transient)
	ARok2CityLayoutActor* Layout;

	/** هل التحرير نشط؟ */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	bool bActive = false;

	/** المبنى الجاري سحبه حالياً (إن وجد). */
	UPROPERTY(Transient)
	ARok2BuildingActor* DraggedBuilding = nullptr;

	/** المبنى الشبح (ghost) أثناء السحب. */
	UPROPERTY(Transient)
	ARok2BuildingActor* GhostBuilding = nullptr;

	/** التخطيطات المحفوظة الثلاثة. */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	TArray<FRok2SavedLayout> SavedLayouts;

	/** دخول/خروج وضع التحرير. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void EnterEditMode();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ExitEditMode(bool bSaveChanges = true);

	/** بدء سحب مبنى (عند لمسه في وضع التحرير). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void BeginDrag(const FString& BuildingId);

	/** تحديث موضع السحب لموقع عالم (يُستدعى كل إطار من PlayerController). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UpdateDrag(const FVector& WorldLocation);

	/** إنهاء السحب: إفلات على أقرب خلية صالحة (وإلا يرجع للأصل). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void EndDrag();

	/** تدوير المبنى المسحوب 60°. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RotateDragged();

	/** حفظ التخطيط الحالي في خانة (0..2). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SaveCurrentLayout(int32 Slot, const FString& Name);

	/** تحميل تخطيط من خانة (0..2) — يبدّل مواضع المباني بأنيميشن. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void LoadLayout(int32 Slot);

	/** ربط المدير. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetLayout(ARok2CityLayoutActor* InLayout);

protected:
	/** آخر خلية صالحة تحت المؤشر أثناء السحب. */
	UPROPERTY(Transient)
	FRok2HexCell HoverCell;

	UPROPERTY(Transient)
	bool bHoverValid = false;

	/** موضع المبنى قبل بدء السحب (للرجوع عند الإفلات غير الصالح). */
	UPROPERTY(Transient)
	FRok2HexCell DragOriginCell;

	void SpawnGhost();
	void DestroyGhost();
	void UpdateGhostTint();
};
