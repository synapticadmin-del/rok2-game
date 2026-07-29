// Copyright ROK2. Hexagonal city wall actor (P5-T1 / P5-T2).
//
// يرسم سوراً سداسياً حول المدينة: 6 أضلاع + بوابة رئيسية على ضلع واحد + أبراج.
// مظهره يتغير مع مستوى السور (خشب → حجر → محصّن → ذهبي) ومتانته تُقرأ من الخادم.
// P5-T2: يضيف ثيم الحضارة (لون + زخارف) للسور والبوابة والأبراج.
// المواصفة: 07-game-design/castle-hex-city.md §2.2 و §5 + 07-game-design/civilizations-visual-design.md.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2HexGrid.h"
#include "Rok2HexWallActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

/** مرحلة مظهر السور حسب المستوى. */
UENUM(BlueprintType)
enum class ERok2WallTier : uint8
{
	Wood = 0,   // L1-4  سور خشبي
	Stone,      // L5-9  سور حجري
	Fortified,  // L10-15 محصّن بأبراج
	Citadel     // L16+  عاصمة إمبراطورية (ذهب وزخارف)
};

UCLASS()
class ROK2_API ARok2HexWallActor : public AActor
{
	GENERATED_BODY()

public:
	ARok2HexWallActor();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	/** كتل السور (instanced) */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* WallSegments;

	/** الأبراج عند الزوايا والبوابة (instanced) */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* Towers;

	/** البوابة الرئيسية (instanced — مكعب واحد مميز) */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UInstancedStaticMeshComponent* Gate;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* WallSegmentMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* TowerMesh;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	UStaticMesh* GateMesh;

	/** نصف قطر المدينة بالخلايا (السور يُبنى على الحلقة == Radius). */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	int32 CityRadiusCells = 7;

	/** حجم الخلية (سمك hex من المركز للرأس بالسنتيمتر). */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	float HexSize = 260.f;

	/** مستوى السور الحالي (يُحدَّث من الخادم عبر SetWallState). */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	int32 WallLevel = 1;

	/** نسبة المتانة 0..1 (من الخادم). */
	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	float Durability01 = 1.f;

	/** حضارة مالك المدينة — تحدد لون السور والزخارف (P5-T2). */
	UPROPERTY(EditAnywhere, Category = "Rok2")
	FString CivId = TEXT("rome");

	/** يعيد بناء السور بالكامل بالمستوى والمتانة الحاليين. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RebuildWall();

	/** تحديث المستوى/المتانة من حالة الخادم ثم إعادة البناء. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetWallState(int32 InLevel, float InDurability01);

	/** مرحلة المظهر المقابلة لمستوى. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	static ERok2WallTier TierForLevel(int32 Level);

protected:
	UFUNCTION()
	void OnWallCellClicked(AActor* TouchedActor, FKey ButtonPressed);

	void ApplyTierMaterials();

	/** يطبق ثيم الحضارة على مواد السور والبوابة والأبراج. */
	void ApplyCivTheme();

	float TierScale() const;
};
