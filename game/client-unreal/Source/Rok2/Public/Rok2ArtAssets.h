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
class UTexture2D;

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

	/** مجلد GLB داخل Content/Art (kaykit/HumanUnits/KenneyCastleKit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString Folder;
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

	/** مسار الأصل المسطّح كما يسمّيه مستورد glTF (Game/Art/kaykit/<file>.<file>) */
	static FString EditorPackagePath(const FString& GlbFile);

	/** مسار الشجرة الفرعية (Game/Art/kaykit/<file>/StaticMeshes/<file>) */
	static FString ImportedMeshPackagePath(const FString& GlbFile);

	/** كل المسارات المحتملة لميش أصل واحد، بالترتيب. ملفات KayKit متعددة العقد
	 *  تنتج اسماً بلاحقة لون (building_windmill_blue) لا اسم الملف. */
	static TArray<FString> MeshPackageCandidates(const FString& GlbFile);

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

	/**
	 * مسار أصل Texture2D المستورد لرموز المدينة والخريطة، أو سلسلة فارغة إن
	 * كان المعرّف خارج الحزمة. يبقى GetIconBrush مسؤولاً عن الاحتياط الإجرائي.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	static FString GetImportedUiIconAssetPath(const FString& IconId);

	// -------------------------------------------------------------------
	// P7-T10: أيقونات خريطة العالم (عُقد موارد، برابرة، عرش، ممرات، منشآت
	// تحالف، وأيقونات مسيرات الفروع) — حزمة Content/Art/WorldMapIcons.
	// -------------------------------------------------------------------

	/**
	 * مسار أصل Texture2D لأيقونة خريطة العالم، أو سلسلة فارغة لمعرّف خارج
	 * الحزمة. يستهلكه WorldRenderer عند رسم العُقد والمسيرات بدل الأشكال
	 * الهندسية الافتراضية، مع fallback آمن للسلوك القديم عند فشل التحميل.
	 * المعرّفات: node_wheat/node_wood/node_stone/node_gold/node_barbarian/
	 * objective_throne_crown/objective_pass_gate/alliance_bastion/
	 * alliance_catapult/march_infantry/march_cavalry/march_archer/march_siege.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	static FString GetWorldMapIconAssetPath(const FString& IconId);

	/** يحمّل UTexture2D لأيقونة خريطة العالم (مع تخزين مؤقت)، أو nullptr. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Icons")
	static UTexture2D* LoadWorldMapIcon(const FString& IconId);

	/** يتحقق من أن المعرّف يطابق أيقونة خريطة العالم المعروفة في الحزمة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Icons")
	static bool HasWorldMapIcon(const FString& IconId);

	// -------------------------------------------------------------------
	// 2.5D Isometric Building PBR Textures (Albedo, Normal, Emissive)
	// -------------------------------------------------------------------

	/** مسار أصل Texture2D لمبنى 2.5D (MapType: "D" للون، "N" للعمق، "E" للإضاءة) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Art")
	static FString GetCityBuildingTextureAssetPath(const FString& BuildingId, const FString& MapType = TEXT("D"));

	/** يحمّل Texture2D لمبنى 2.5D مع التخزين المؤقت */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Art")
	static UTexture2D* LoadCityBuildingTexture(const FString& BuildingId, const FString& MapType = TEXT("D"));

	/** مسار أصل Texture2D لمعالم خريطة العالم 2.5D */
	UFUNCTION(BlueprintPure, Category = "Rok2|Art")
	static FString GetWorldFeatureTextureAssetPath(const FString& FeatureId, const FString& MapType = TEXT("D"));

	/** يحمّل Texture2D لمعلم خريطة العالم 2.5D مع التخزين المؤقت */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Art")
	static UTexture2D* LoadWorldFeatureTexture(const FString& FeatureId, const FString& MapType = TEXT("D"));

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

	// -------------------------------------------------------------------
	// P8-T8: الوحدات البشرية 3D — شبكات T1–T5 × 4 فروع + 6 وحدات خاصة.
	// HumanUnits: infantry/archer/cavalry_t{1..5} + siege_arcuballista +
	// siege_mangonel (توليد إجرائي low-poly)؛ siege T3–T5 من Kenney Castle Kit.
	// الوحدات الخاصة الحضارية (legionary/chu_ko_nu/desert_rider/khopesh_guard/
	// huskarl/samurai) ترث شبكة فرعها عند unlock_tier=4 حتى توفر جلود مخصصة.
	// -------------------------------------------------------------------

	/**
	 * معرّف شبكة وحدة بشرية من الفرع والمرحلة، أو سلسلة فارغة لوحدات خاصة
	 * مغلقة (unlock_tier غير محقق) — على الراسم fallback الهندسي عندها.
	 * Branches: infantry/archer/cavalry/siege؛ Tier: 1–5.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Units")
	static FString GetHumanUnitId(const FString& Branch, int32 Tier, const FString& CivId = TEXT(""));

	/** مسار أصل المحرر لشبكة وحدة بشرية إن استُوردت (/Game/Art/HumanUnits/{id}.{id}). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Units")
	static FString GetHumanUnitAssetPath(const FString& UnitId);

	/** يحمّل UStaticMesh لشبكة وحدة بشرية إن استُوردت، وإلا nullptr (fallback هندسي). */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Units")
	static UStaticMesh* LoadHumanUnitMesh(const FString& UnitId);

	/** يتحقق من أن المعرّف يطابق وحدة بشرية معروفة في الحزمة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Units")
	static bool HasHumanUnit(const FString& UnitId);

	// -------------------------------------------------------------------
	// P10-T7: أصول الحانة والصناديق والمفاتيح والمنحوتات والمواد والمخططات
	// (Content/Art/Tavern + Content/Audio/sfx)
	// -------------------------------------------------------------------

	/** مسار شبكة الحانة أو الصناديق 3D (/Game/Art/Tavern/{AssetId}.{AssetId}) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Tavern")
	static FString GetTavernMeshAssetPath(const FString& MeshId);

	/** مسار أيقونة 2D لحزمة الحانة (/Game/Art/Tavern/{IconId}.{IconId}) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Tavern")
	static FString GetTavernIconAssetPath(const FString& IconId);

	/** يحمّل Texture2D لأصل من أصول الحانة/الصناديق/المفاتيح/المنحوتات/المواد */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Tavern")
	static UTexture2D* LoadTavernIcon(const FString& IconId);

	/** هل المعرّف يطابق أصلاً مسجلاً في حزمة الحانة؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2|Tavern")
	static bool HasTavernAsset(const FString& AssetId);

protected:
	UPROPERTY(Transient)
	TArray<FRok2ArtEntry> Catalog;

	UPROPERTY(Transient)
	TMap<FString, UStaticMesh*> Loaded;

	bool bCatalogBuilt = false;
	void BuildCatalog();

	/** كتالوج الوحدات البشرية (UnitId → GlbFile → Scale) يُبنى مرة واحدة. */
	TArray<FRok2ArtEntry> HumanUnitCatalog;
	bool bHumanCatalogBuilt = false;
	void BuildHumanUnitCatalog();
};
