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
class UTexture2D;

/** لقطة خفيفة لأعباء تمثيل خريطة العالم؛ تستعمل في PIE ولا تمثل زمن GPU أو ذاكرة النظام. */
USTRUCT(BlueprintType)
struct FRok2WorldPerfSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 CityInstances = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 PassInstances = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 ResourceNodeInstances = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 BarbarianNodeInstances = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 MarkerActors = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 MarchActors = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 HillActors = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 PooledMarkerActors = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") int32 WorldFrameSamples = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") float WorldFrameAverageMs = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Perf") float WorldFramePeakMs = 0.f;
};

/** طبقة العرض المحسوبة من مسافة التكبير المستهدفة، لا من عدد العناصر في اللقطة. */
UENUM(BlueprintType)
enum class ERok2WorldZoomLayer : uint8
{
	Tactical,
	Regional,
	Kingdom
};

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

	/** P8-T7: يرسم عرش الملك في موقع التتويج عند توفر FRok2KingMarker في اللقطة. */
	void DrawKingMarker();

	/** يحدّث طبقة العالم وفق مسافة الكاميرا؛ يعيد true فقط عند تغير الطبقة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Zoom Layers")
	bool UpdateZoomLayer(float TargetZoomDistance);

	UFUNCTION(BlueprintPure, Category = "Rok2|Zoom Layers")
	ERok2WorldZoomLayer GetZoomLayer() const { return CurrentZoomLayer; }

	/** أمر المسيرة مسموح فقط عندما تكون الأهداف التكتيكية مرئية وقابلة للاختيار. */
	UFUNCTION(BlueprintPure, Category = "Rok2|World Interaction")
	bool CanIssueMarchCommand() const;

	/** يحدّد ما إذا كان نوع هدف معيّن قابلاً للمعاينة أو للأمر عند طبقة التكبير الحالية. */
	UFUNCTION(BlueprintPure, Category = "Rok2|World Interaction")
	bool CanInteractWithWorldTarget(const FString& TargetType, bool bRequiresMarchOrder) const;

	/** عدد المسيرات الحية للاعب، مشتق من آخر لقطة عالم سلطوية. */
	UFUNCTION(BlueprintPure, Category = "Rok2|World Interaction")
	int32 GetActiveMarchCount() const;

	/** سعة المسيرات التي تمنحها قاعة المدينة الحالية؛ تطابق التحقق السلطوي في الخادم. */
	UFUNCTION(BlueprintPure, Category = "Rok2|World Interaction")
	int32 GetMarchCapacity() const;

	/** ينشئ طلب بناء في موضع عالم Unreal؛ تحقق الخادم هو السلطة النهائية للنوع والرتبة والإقليم. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance Structures")
	void RequestAllianceStructureAtWorldPoint(const FString& StructureKind, FVector WorldPoint);

	/** لقطة أعداد تمثيل العالم وزمن tick المتراكم لراسم الخريطة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Perf")
	FRok2WorldPerfSnapshot GetPerformanceSnapshot() const;

	/** يبدأ نافذة قياس جديدة لزمن tick راسم العالم قبل مسار قبول PIE. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Perf")
	void ResetPerformanceSnapshot();

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

	/** دون هذا الحد تظهر العقد والبرابرة وأنصاف أقطار الحماية. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Zoom Layers")
	float TacticalZoomMaxDistance = 12000.f;

	/** دون هذا الحد تظهر الممرات والمسيرات، وفوقه تبقى المدن ومنشآت التحالف فقط. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Zoom Layers")
	float RegionalZoomMaxDistance = 35000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rok2|Zoom Layers")
	ERok2WorldZoomLayer CurrentZoomLayer = ERok2WorldZoomLayer::Tactical;

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
	/** عرش الملك (P8-T7) — أُعيد تعيينه عند كل لقطة عالم. */
	AActor* SpawnedThrone = nullptr;

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
	void ApplyZoomLayerVisibility();
	bool IsTacticalLayer() const { return CurrentZoomLayer == ERok2WorldZoomLayer::Tactical; }
	bool IsRegionalOrCloserLayer() const { return CurrentZoomLayer != ERok2WorldZoomLayer::Kingdom; }
	AActor* SpawnMarkerActor(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);
	void SpawnMarker(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color);

	/**
	 * P7-T10: ينشئ أيقونة Sprite تواجه الكاميرا (BillboardComponent بـ UTexture2D
	 * لأيقونة خريطة العالم من Content/Art/WorldMapIcons). يعيد nullptr إذا لم
	 * تُستورد الحزمة بعد، وعندها يعود الراسم إلى الشكل الهندسي الافتراضي.
	 * Scale يُطبّق على أبعاد sprite في مستويات العالم.
	 */
	AActor* SpawnSpriteActor(UTexture2D* Icon, const FVector& Loc, const FString& Label, float Scale = 1.f);

	UFUNCTION()
	void OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot);
};
