// Copyright ROK2. Art asset library (P2-T7) — KayKit CC0 GLB loader with graceful fallback.
//
// يحمّل موديلات KayKit (GLB) من Content/Art/kaykit في المحرر أو عند توفر
// موديول استيراد glTF؛ وإلا يعيد nullptr فيبقى الكود على الأشكال الهندسية الحالية.
// المسارات المفهرسة هنا تُستخدم أيضاً من setup_level.py لتوليد مراجع المستوى.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateBrush.h"
#include "Rok2ArtAssets.generated.h"

class UStaticMesh;

/** خريطة مبنى اللعبة → ملف GLB في Content/Art/kaykit (بدون الامتداد) */
USTRUCT(BlueprintType)
struct FRok2ArtEntry
{
	GENERATED_BODY()

	/** معرف المبنى/العنصر (city_hall, barracks, flag_blue, mountain ...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString Id;

	/** اسم ملف الـ GLB داخل Content/Art/kaykit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString GlbFile;

	/** مقياس التصحيح عند الرسم (موديلات KayKit بمقياس سنتيمتر تقريباً) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	float Scale = 1.f;
};

UCLASS(BlueprintType)
class ROK2_API URok2ArtAssets : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2ArtAssets* Get();

	/** فهرس الأصول الكامل: مبانٍ + أعلام + طبيعة */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2ArtEntry>& GetCatalog() const { return Catalog; }

	/** هل توجد نسخة فنية لعنصر ما؟ (لا تعني أنها حُمّلت) */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool HasArt(const FString& Id) const;

	/**
	 * يحمّل UStaticMesh من GLB إن أمكن (محرر / موديول glTF مستورد)،
	 * وإلا nullptr — وعلى الراسم عندها استخدام الشكل الهندسي الافتراضي.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	UStaticMesh* LoadMesh(const FString& Id);

	/** مسار حزمة المحرر للأصل إن استُورد مسبقاً (Game/Art/kaykit/<file>.<file>) */
	static FString EditorPackagePath(const FString& GlbFile);

	/** مسار الملف على القرص داخل المحتوى */
	static FString DiskPath(const FString& GlbFile);

	// -------------------------------------------------------------------
	// P6-T1: نظام أيقونات UI الموحد — يُخدم من هنا (تفويض لـ URok2IconLibrary)
	// -------------------------------------------------------------------

	/**
	 * يعيد FSlateBrush لأيقونة UI إجرائية بالمعرّف والحجم المطلوب (24/32/48).
	 * Tint يصبغ الأيقونة (الافتراضي عاجي #F5E9D0). المعرّفات: food/wood/stone/gold/
	 * gems/ap/build/sword/shield/helmet/bag/banner/scroll/map/edit/bell/lock/...
	 * انظر URok2IconLibrary::GetIconIds للقائمة الكاملة.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Icons")
	static FSlateBrush GetIconBrush(const FString& IconId, float Size = 32.f, FLinearColor Tint = FLinearColor(0.96f, 0.91f, 0.81f, 1.f));

	/** هل يوجد رسم لهذا المعرّف؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	static bool HasIcon(const FString& IconId);

	// -------------------------------------------------------------------
	// P6-T8: فهرس أصوات الواجهة (أصول WAV مشتركة + همس حضاري عند الفتح).
	// يقتصر على المسارات؛ التشغيل يتولاه مالك الواجهة/مدير الصوت عند الربط.
	// -------------------------------------------------------------------

	/** مسار أصل واجهة مشترك: button_click / panel_open / panel_close / error. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Audio")
	static FString GetUiSfxAssetPath(const FString& SfxId);

	/** مسار همس الحضارة من P6-T8، أو سلسلة فارغة لمعرّف حضارة غير معروف. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Audio")
	static FString GetCivilizationWhisperAssetPath(const FString& CivilizationId);

	/** يتحقق من أن المعرّف يطابق أثراً صوتياً معروفاً في فهرس P6-T8. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Audio")
	static bool HasUiSfx(const FString& SfxId);

protected:
	UPROPERTY(Transient)
	TArray<FRok2ArtEntry> Catalog;

	UPROPERTY(Transient)
	TMap<FString, UStaticMesh*> Loaded;

	bool bCatalogBuilt = false;
	void BuildCatalog();
};
