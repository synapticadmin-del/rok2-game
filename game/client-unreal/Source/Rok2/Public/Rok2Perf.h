// Copyright ROK2. Performance subsystem (P4-T7): shared mesh cache + actor pool.
//
// يعالج أهم اختناقات أداء العميل المكتشفة بالتدقيق:
//  1) LoadObject متكرر لنفس meshes (/Engine/BasicShapes/*) في كل building/refresh —
//     GetEngineMesh يخبئها مرة واحدة للأبد (UPROPERTY يحميها من GC).
//  2) churn كامل للـ actors كل تحديث (WorldRenderer كل 3ث + CityLayout عند كل city upsert):
//     تدمير وإعادة زرع عشرات الـ StaticMeshActors — Pool خفيف يعيد استخدامها
//     (إخفاء/إظهار بدل Destroy/SpawnActor) فتختفي كلفة الـ spawn/GC.
//  3) ثوابت LOD موحدة: مسافة رسم الخريطة كانت ثابتاً سحرياً 1000000² — صارت
//     قابلة للضبط من مكان واحد وتُحسب مربعها مرة واحدة.
//
// لا يغيّر أي سلوك مرئي — فقط يقلل التخصيصات والتحميل المتكرر.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Rok2Perf.generated.h"

class AStaticMeshActor;

UCLASS()
class ROK2_API URok2Perf : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** الوصول للنسخة المشتركة (null خارج سياق لعبة فعلية). */
	static URok2Perf* Get(const UObject* WorldContextObject);

	// ---- Mesh cache ----
	/** mesh من /Engine/BasicShapes باسمه (Plane/Cube/Sphere/Cylinder/Cone) — يُخبأ بعد أول تحميل. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Perf")
	UStaticMesh* GetEngineMesh(const FString& ShapeName);

	// ---- Actor pool ----
	/**
	 * يستعير StaticMeshActor من المسبح (أو ينشئ جديداً عند الحاجة).
	 * الممثل المُعاد يكون مخفياً وبدون tick — أعد ضبط mesh/الموضع/اللون ثم أظهره.
	 */
	AStaticMeshActor* AcquireMarkerActor(UWorld* World);

	/**
	 * يعيد ممثلاً للمسبح (يخفيه ويعطّل tickه ويفصله عن المشهد — لا يدمّره).
	 * آمن على nullptr وممثلات لم تأتِ من المسبح.
	 */
	void ReleaseMarkerActor(AStaticMeshActor* Actor);

	/** يدمّر محتوى المسبح فعلياً (انتقال خرائط/إغلاق). */
	void FlushPool();

	/** عدد الممثلات الخاملة حالياً في المسبح (للتشخيص/الاختبارات). */
	int32 PoolSize() const { return Pool.Num(); }

	/** سقف المسبح — الفائض يُدمّر عند الإعادة بدل التضخم بلا حد. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Perf")
	int32 MaxPoolSize = 64;

	// ---- LOD constants ----
	/** مسافة رسم الخريطة بالوحدات (يُقارن مربعها — LOD). كانت ثابتاً سحرياً في WorldRenderer. */
	UPROPERTY(EditAnywhere, Category = "Rok2|Perf")
	float WorldRenderDistance = 1000000.f;

	/** مربع مسافة الرسم (يُحسب مرة واحدة عند الاستعلام المتكرر). */
	float WorldRenderDistanceSq() const { return WorldRenderDistance * WorldRenderDistance; }

protected:
	/** meshes المخبأة من /Engine/BasicShapes (UPROPERTY يحميها من GC). */
	UPROPERTY(Transient)
	TMap<FString, UStaticMesh*> EngineMeshCache;

	/** ممثلات خاملة قابلة لإعادة الاستخدام. */
	UPROPERTY(Transient)
	TArray<AStaticMeshActor*> Pool;
};
