// Copyright ROK2. Art asset library (P2-T7) — implementation.

#include "Rok2ArtAssets.h"
#include "Rok2IconLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Art, Log, All);

URok2ArtAssets* URok2ArtAssets::Get()
{
	static URok2ArtAssets* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2ArtAssets>();
		Instance->AddToRoot();
	}
	Instance->BuildCatalog();
	return Instance;
}

FString URok2ArtAssets::EditorPackagePath(const FString& GlbFile)
{
	// الاسم كما هو في ملف الـ GLB — هذا ما ينتجه مستورد glTF فعلاً.
	return FString::Printf(TEXT("/Game/Art/kaykit/%s.%s"), *GlbFile, *GlbFile);
}

FString URok2ArtAssets::ImportedMeshPackagePath(const FString& GlbFile)
{
	// شجرة فرعية تنتجها بعض إعدادات Interchange: <name>/StaticMeshes/<name>.
	return FString::Printf(TEXT("/Game/Art/kaykit/%s/StaticMeshes/%s.%s"), *GlbFile, *GlbFile, *GlbFile);
}

TArray<FString> URok2ArtAssets::MeshPackageCandidates(const FString& GlbFile)
{
	// بعض ملفات KayKit تحتوي عدة عُقد، فيُسمّي المستورد الميش الرئيسي باسم
	// العقدة لا باسم الملف: building_windmill.glb ينتج building_windmill_blue،
	// وbuilding_tower_A ينتج building_tower_A_blue. لذلك نبحث في مرشحات
	// مرتبة بدل مسار واحد، وإلا سقط المبنى إلى المكعب البديل بلا سبب ظاهر.
	TArray<FString> Out;
	Out.Add(EditorPackagePath(GlbFile));
	Out.Add(ImportedMeshPackagePath(GlbFile));

	static const TCHAR* NodeSuffixes[] = { TEXT("_blue"), TEXT("_red"), TEXT("_green"), TEXT("_yellow") };
	for (const TCHAR* Suffix : NodeSuffixes)
	{
		const FString Variant = GlbFile + Suffix;
		Out.Add(EditorPackagePath(Variant));
		Out.Add(ImportedMeshPackagePath(Variant));
	}
	return Out;
}

FString URok2ArtAssets::DiskPath(const FString& GlbFile)
{
	return FPaths::ProjectContentDir() / TEXT("Art/kaykit") / (GlbFile + TEXT(".glb"));
}

void URok2ArtAssets::BuildCatalog()
{
	if (bCatalogBuilt) return;
	bCatalogBuilt = true;

	// مبانٍ (KayKit Medieval Hexagon — CC0) — خريطة building_id → ملف
	auto Add = [this](const TCHAR* Id, const TCHAR* File, float Scale)
	{
		FRok2ArtEntry E;
		E.Id = Id;
		E.GlbFile = File;
		E.Scale = Scale;
		Catalog.Add(E);
	};

	// مباني المدينة
	Add(TEXT("city_hall"),     TEXT("building_castle"),       2.2f);
	Add(TEXT("farm"),          TEXT("building_windmill"),     1.8f);
	Add(TEXT("lumber_mill"),   TEXT("building_lumbermill"),   1.8f);
	Add(TEXT("quarry"),        TEXT("building_mine"),         1.8f);
	Add(TEXT("goldmine"),      TEXT("building_mine"),         1.6f);
	Add(TEXT("barracks"),      TEXT("building_barracks"),     1.8f);
	Add(TEXT("stable"),        TEXT("building_tavern"),       1.6f);
	Add(TEXT("archery_range"), TEXT("building_archeryrange"), 1.8f);
	Add(TEXT("hospital"),      TEXT("building_market"),       1.6f);
	Add(TEXT("wall"),          TEXT("building_tower_A"),      1.8f);
	Add(TEXT("storehouse"),    TEXT("building_blacksmith"),   1.6f);
	// بديل بلون أحمر للمدن المعادية على الخريطة
	Add(TEXT("city_enemy"),    TEXT("building_castle_red"),   2.2f);

	// أعلام التحالف (ألوان الفرق)
	Add(TEXT("flag_blue"),     TEXT("prop_flag_blue"),        1.5f);
	Add(TEXT("flag_red"),      TEXT("prop_flag_red"),         1.5f);
	Add(TEXT("flag_green"),    TEXT("prop_flag_green"),       1.5f);
	Add(TEXT("flag_yellow"),   TEXT("prop_flag_yellow"),      1.5f);

	// طبيعة (جبال/تلال للحدود)
	Add(TEXT("mountain"),      TEXT("nature_mountain_A"),     2.0f);
	Add(TEXT("hills"),         TEXT("nature_hills_A"),        2.0f);
}

bool URok2ArtAssets::HasArt(const FString& Id) const
{
	for (const FRok2ArtEntry& E : Catalog)
	{
		if (E.Id == Id) return true;
	}
	return false;
}

UStaticMesh* URok2ArtAssets::LoadMesh(const FString& Id)
{
	BuildCatalog();

	if (UStaticMesh** Found = Loaded.Find(Id))
	{
		return *Found;
	}

	const FRok2ArtEntry* Entry = nullptr;
	for (const FRok2ArtEntry& E : Catalog)
	{
		if (E.Id == Id) { Entry = &E; break; }
	}
	if (!Entry) return nullptr;

	UStaticMesh* Mesh = nullptr;

	// كان التحميل محصوراً بـ #if WITH_EDITOR، فكان LoadMesh يعيد nullptr دائماً
	// في بناء مُطبَّق (APK) ويبقى العميل على مكعبات placeholder أبد الدهر — حتى
	// لو كانت الأصول مُستوردة ومحزَّمة. الأصول .uasset متاحة في وقت التشغيل
	// تماماً كما في المحرر، فالحاجز كان خطأ لا احتياطاً.
	for (const FString& Candidate : MeshPackageCandidates(Entry->GlbFile))
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, *Candidate);
		if (Mesh)
		{
			break;
		}
	}

	if (!Mesh)
	{
		UE_LOG(LogRok2Art, Warning, TEXT("Art mesh for '%s' not imported yet (%s) — geometric fallback stays active"),
			*Id, *DiskPath(Entry->GlbFile));
	}

	Loaded.Add(Id, Mesh);
	return Mesh;
}

// ---------------------------------------------------------------------------
// P6-T1: نظام أيقونات UI الموحد — تفويض لـ URok2IconLibrary
// ---------------------------------------------------------------------------

FString URok2ArtAssets::GetImportedUiIconAssetPath(const FString& IconId)
{
	FString CanonicalId = IconId;
	if (CanonicalId == TEXT("scroll"))
	{
		CanonicalId = TEXT("reports");
	}

	// القائمة تطابق ما هو مستورد فعلاً في Content/Art/UIIcons (70 أيقونة).
	// كانت 20 فقط، فكانت أربعون معرّفاً — من بينها sword وshield وcrown
	// وcastle وhelmet وbanner، أي أيقونات الأزرار الرئيسية في الـHUD — تسقط
	// إلى الراسم الإجرائي: أشكال بدائية على شبكة 32×32 تُعرض بحجم 14–24px.
	// المجموعة المتبقية وُلّدت في scripts/generate_ui_icon_set.py بنفس أسلوب
	// المجموعة الأصلية، فالقائمتان الآن مجموعة واحدة متسقة.
	//
	// الترتيب يطابق `KnownIds` في Rok2IconLibrary.cpp؛ أي معرّف خارج القائمتين
	// يبقى على الراسم الإجرائي بلا انكسار.
	static const TSet<FString> ImportedIds = {
		// المجموعة الأصلية (P7-T9)
		TEXT("build"), TEXT("upgrade"), TEXT("train"), TEXT("research"),
		TEXT("alliance"), TEXT("map"), TEXT("reports"), TEXT("mail"),
		TEXT("settings"), TEXT("food"), TEXT("wood"), TEXT("stone"),
		TEXT("gold"), TEXT("gems"), TEXT("speedup"), TEXT("hospital"),
		TEXT("bag"), TEXT("bell"), TEXT("heal"), TEXT("speed"),
		// مجموعة P24-T2 — الخمسون الباقية من KnownIds
		TEXT("ap"), TEXT("sword"), TEXT("shield"), TEXT("helmet"),
		TEXT("banner"), TEXT("edit"), TEXT("lock"), TEXT("calendar"),
		TEXT("hourglass"), TEXT("flask"), TEXT("cross"), TEXT("scout"),
		TEXT("close"), TEXT("star"), TEXT("skull"), TEXT("blood"),
		TEXT("bandage"), TEXT("trophy"), TEXT("handshake"), TEXT("refresh"),
		TEXT("gift"), TEXT("wheat"), TEXT("box"), TEXT("cart"),
		TEXT("horse"), TEXT("bow"), TEXT("tent"), TEXT("tower"),
		TEXT("castle"), TEXT("bricks"), TEXT("rock"), TEXT("beer"),
		TEXT("scale"), TEXT("crown"), TEXT("builder"), TEXT("conn"),
		TEXT("governor"), TEXT("stats"), TEXT("move"), TEXT("sparkle"),
		TEXT("combat"), TEXT("ring"), TEXT("boots"), TEXT("arrow"),
		TEXT("skillup"), TEXT("pickaxe"), TEXT("clock"), TEXT("art"),
		TEXT("monument"), TEXT("wrench")
	};
	if (!ImportedIds.Contains(CanonicalId))
	{
		return FString();
	}

	return FString::Printf(TEXT("/Game/Art/UIIcons/icon_%s.icon_%s"), *CanonicalId, *CanonicalId);
}

FSlateBrush URok2ArtAssets::GetIconBrush(const FString& IconId, float Size, FLinearColor Tint)
{
	const FString ImportedPath = GetImportedUiIconAssetPath(IconId);
	if (!ImportedPath.IsEmpty())
	{
		if (UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ImportedPath))
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(ImportedTexture);
			Brush.ImageSize = FVector2D(Size, Size);
			Brush.TintColor = Tint;
			return Brush;
		}
	}

	return URok2IconLibrary::BrushFromArtAssets(IconId, Size, Tint);
}

bool URok2ArtAssets::HasIcon(const FString& IconId)
{
	return !GetImportedUiIconAssetPath(IconId).IsEmpty() || URok2IconLibrary::Get()->HasIcon(IconId);
}

// ---------------------------------------------------------------------------
// P7-T10: أيقونات خريطة العالم — عُقد موارد وأهداف ومسيرات فرع.
// ---------------------------------------------------------------------------

static const TSet<FString> WorldMapIconIds = {
	TEXT("node_wheat"), TEXT("node_wood"), TEXT("node_stone"), TEXT("node_gold"),
	TEXT("node_barbarian"), TEXT("node_resource_generic"),
	TEXT("objective_throne_crown"), TEXT("objective_pass_gate"),
	TEXT("alliance_bastion"), TEXT("alliance_catapult"),
	TEXT("march_infantry"), TEXT("march_cavalry"), TEXT("march_archer"), TEXT("march_siege")
};

FString URok2ArtAssets::GetWorldMapIconAssetPath(const FString& IconId)
{
	if (!WorldMapIconIds.Contains(IconId))
	{
		return FString();
	}
	return FString::Printf(TEXT("/Game/Art/WorldMapIcons/icon_%s.icon_%s"), *IconId, *IconId);
}

UTexture2D* URok2ArtAssets::LoadWorldMapIcon(const FString& IconId)
{
	static TMap<FString, UTexture2D*> Cached;
	UTexture2D** Found = Cached.Find(IconId);
	if (Found)
	{
		return *Found;
	}

	const FString Path = GetWorldMapIconAssetPath(IconId);
	UTexture2D* Texture = Path.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *Path);
	Cached.Add(IconId, Texture);
	return Texture;
}

bool URok2ArtAssets::HasWorldMapIcon(const FString& IconId)
{
	return WorldMapIconIds.Contains(IconId);
}

// ---------------------------------------------------------------------------
// 2.5D Isometric Building PBR Textures
// ---------------------------------------------------------------------------

FString URok2ArtAssets::GetCityBuildingTextureAssetPath(const FString& BuildingId, const FString& MapType)
{
	return FString::Printf(TEXT("/Game/Art/CityBuildingIcons/T_%s_%s.T_%s_%s"), *BuildingId, *MapType, *BuildingId, *MapType);
}

UTexture2D* URok2ArtAssets::LoadCityBuildingTexture(const FString& BuildingId, const FString& MapType)
{
	static TMap<FString, UTexture2D*> CachedCityTextures;
	const FString Key = FString::Printf(TEXT("%s_%s"), *BuildingId, *MapType);
	UTexture2D** Found = CachedCityTextures.Find(Key);
	if (Found)
	{
		return *Found;
	}

	const FString Path = GetCityBuildingTextureAssetPath(BuildingId, MapType);
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
	CachedCityTextures.Add(Key, Texture);
	return Texture;
}

// ---------------------------------------------------------------------------
// P24-T6: معالم الخريطة المقطّعة (2.5D).
//
// كانت `GetWorldFeatureTextureAssetPath` تشير إلى `T_world_*` في
// WorldMapIcons، وتلك **صفائح** لا sprites: `T_world_resource_nodes_quad`
// يحمل أربع عقد ونصاً عربياً مطبوعاً داخل الصورة، و`T_world_stone_gold_quarry_mine`
// يحمل منشأتين. فالرسم منها يُظهر عقدة تحمل ثلاث عقد أخرى ونصاً معكوساً.
//
// `scripts/slice_world_feature_sprites.py` يقطّعها بمركّبات الشفافية المتصلة
// إلى Content/Art/WorldFeatures كـ`T_feat_<id>_<D|N|E>`، والمعرّفات أدناه
// مقروءة من بيان `sprites.json` لا مخترعة. أي معرّف خارجها يعيد سلسلة فارغة
// فيبقى الراسم على أيقونته الحالية.
// ---------------------------------------------------------------------------

static const TSet<FString>& WorldFeatureSpriteIds()
{
	static const TSet<FString> Ids = {
		// عقد الموارد الأربع (من صفيحة الشبكة 2×2)
		TEXT("farm_field"), TEXT("lumber_camp"), TEXT("stone_quarry"), TEXT("gold_mine"),
		// منشآت مفردة
		TEXT("gold_mine_large"), TEXT("barbarian_camp"), TEXT("barbarian_keep"),
		TEXT("pass_fortress"), TEXT("throne_temple"), TEXT("holy_shrine"),
		TEXT("mountain_ridge")
	};
	return Ids;
}

FString URok2ArtAssets::GetWorldFeatureTextureAssetPath(const FString& FeatureId, const FString& MapType)
{
	if (!WorldFeatureSpriteIds().Contains(FeatureId))
	{
		return FString();
	}
	return FString::Printf(TEXT("/Game/Art/WorldFeatures/T_feat_%s_%s.T_feat_%s_%s"),
		*FeatureId, *MapType, *FeatureId, *MapType);
}

bool URok2ArtAssets::HasWorldFeatureSprite(const FString& FeatureId)
{
	return WorldFeatureSpriteIds().Contains(FeatureId);
}

FString URok2ArtAssets::WorldFeatureIdForNode(const FString& NodeKind, int32 Level)
{
	// أسماء الأنواع من لقطة الخادم (`FRok2NodeEntity::Kind`) كما يفسّرها
	// `URok2WorldIconography::ResourceIconId` — لا اختراع اسم في العميل.
	if (NodeKind == TEXT("food") || NodeKind == TEXT("wheat") || NodeKind == TEXT("farm"))
	{
		return TEXT("farm_field");
	}
	if (NodeKind == TEXT("wood") || NodeKind == TEXT("lumber"))
	{
		return TEXT("lumber_camp");
	}
	if (NodeKind == TEXT("stone") || NodeKind == TEXT("quarry"))
	{
		return TEXT("stone_quarry");
	}
	if (NodeKind == TEXT("gold") || NodeKind == TEXT("goldmine"))
	{
		// المنجم الكبير للعقد عالية المستوى: الرسم نفسه بحجم أوفى تفصيلاً،
		// فيقرأ اللاعب ثراء العقدة من شكلها لا من رقم وحده.
		return Level >= 4 ? TEXT("gold_mine_large") : TEXT("gold_mine");
	}
	if (NodeKind == TEXT("barb") || NodeKind == TEXT("barbarian"))
	{
		// المعسكر للمستويات الدنيا والحصن للعليا — تراتب مرئي يطابق تراتب
		// الخطر في `zones.json`.
		return Level >= 4 ? TEXT("barbarian_keep") : TEXT("barbarian_camp");
	}
	return FString();
}

UTexture2D* URok2ArtAssets::LoadWorldFeatureTexture(const FString& FeatureId, const FString& MapType)
{
	static TMap<FString, UTexture2D*> CachedWorldTextures;
	const FString Key = FString::Printf(TEXT("%s_%s"), *FeatureId, *MapType);
	UTexture2D** Found = CachedWorldTextures.Find(Key);
	if (Found)
	{
		return *Found;
	}

	const FString Path = GetWorldFeatureTextureAssetPath(FeatureId, MapType);
	UTexture2D* Texture = Path.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *Path);
	CachedWorldTextures.Add(Key, Texture);
	return Texture;
}

// ---------------------------------------------------------------------------
// P24-T4: صور المباني المرسومة (2.5D).
//
// 96 صورة في Content/Art/CityBuildingIcons لم يكن لها قارئ: الدالتان أعلاه
// معرّفتان بلا مستدعٍ، وبطاقة المبنى تعرض رمزاً إجرائياً بدلاً منها. الأسماء
// أدناه مقروءة من القرص لا مخترعة — أي مبنى خارجها يبقى على الرمز الإجرائي.
// ---------------------------------------------------------------------------

namespace
{
	/** المباني التي تملك صورة `T_<id>_base_tier1_D` فعلاً. */
	const TSet<FString>& BasePortraitBuildings()
	{
		static const TSet<FString> Ids = {
			TEXT("academy"), TEXT("archery_range"), TEXT("barracks"), TEXT("city_hall"),
			TEXT("farm"), TEXT("hospital"), TEXT("siege_workshop"), TEXT("stable"),
			TEXT("storehouse"), TEXT("wall")
		};
		return Ids;
	}

	/** الحضارات التي تملك قاعة مدينة مخصّصة `T_civ_<civ>_hall_tier4_D`. */
	const TSet<FString>& CivHallCivilizations()
	{
		static const TSet<FString> Ids = {
			TEXT("arabia"), TEXT("china"), TEXT("egypt"),
			TEXT("japan"), TEXT("rome"), TEXT("vikings")
		};
		return Ids;
	}

	/** المستوى الذي تحلّ عنده قاعة الحضارة محل القاعة العامة (اسم الأصل tier4). */
	constexpr int32 CivHallLevel = 10;
}

FString URok2ArtAssets::GetCityBuildingPortraitId(const FString& BuildingId, int32 Level, const FString& CivilizationId)
{
	// قاعة المدينة عند مستوى متقدم تكسب طابع الحضارة — وهو المكان الوحيد الذي
	// تُرى فيه هوية الحضارة بعد شاشة الاختيار.
	if (BuildingId == TEXT("city_hall") && Level >= CivHallLevel && !CivilizationId.IsEmpty())
	{
		const FString Civ = CivilizationId.ToLower();
		if (CivHallCivilizations().Contains(Civ))
		{
			return FString::Printf(TEXT("civ_%s_hall_tier4"), *Civ);
		}
	}

	if (BasePortraitBuildings().Contains(BuildingId))
	{
		return FString::Printf(TEXT("%s_base_tier1"), *BuildingId);
	}
	return FString();
}

UTexture2D* URok2ArtAssets::LoadCityBuildingPortrait(const FString& BuildingId, int32 Level, const FString& CivilizationId)
{
	const FString PortraitId = GetCityBuildingPortraitId(BuildingId, Level, CivilizationId);
	if (PortraitId.IsEmpty())
	{
		return nullptr;
	}
	// خريطة اللون وحدها تُعرض في الواجهة؛ العمق والإضاءة لمواد العالم 2.5D.
	return LoadCityBuildingTexture(PortraitId, TEXT("D"));
}

// ---------------------------------------------------------------------------
// P6-T8: مسارات أصول الإحساس الصوتي للواجهة.
// تحتفظ المكتبة بالمعرّفات فقط؛ فلا تملك الموسيقى ولا تتدخل في دورة حياة الصوت.
// ---------------------------------------------------------------------------

FString URok2ArtAssets::GetUiSfxAssetPath(const FString& SfxId)
{
	if (SfxId == TEXT("button_click")) return TEXT("Audio/sfx/ui_button_click");
	if (SfxId == TEXT("panel_open")) return TEXT("Audio/sfx/ui_panel_open");
	if (SfxId == TEXT("panel_close")) return TEXT("Audio/sfx/ui_panel_close");
	if (SfxId == TEXT("error")) return TEXT("Audio/sfx/ui_error");
	return FString();
}

FString URok2ArtAssets::GetCivilizationWhisperAssetPath(const FString& CivilizationId)
{
	if (CivilizationId == TEXT("china")) return TEXT("Audio/sfx/ui_civ_whisper_china");
	if (CivilizationId == TEXT("rome")) return TEXT("Audio/sfx/ui_civ_whisper_rome");
	if (CivilizationId == TEXT("arabia")) return TEXT("Audio/sfx/ui_civ_whisper_arabia");
	if (CivilizationId == TEXT("egypt")) return TEXT("Audio/sfx/ui_civ_whisper_egypt");
	if (CivilizationId == TEXT("vikings")) return TEXT("Audio/sfx/ui_civ_whisper_vikings");
	if (CivilizationId == TEXT("japan")) return TEXT("Audio/sfx/ui_civ_whisper_japan");
	return FString();
}

bool URok2ArtAssets::HasUiSfx(const FString& SfxId)
{
	return !GetUiSfxAssetPath(SfxId).IsEmpty();
}

// ---------------------------------------------------------------------------
// P8-T8: الوحدات البشرية 3D — شبكات T1–T5 × 4 فروع + 6 وحدات خاصة حضارية.
// المصادر: توليد إجرائي (HumanUnits/) + Kenney Castle Kit (siege T3–T5، CC0).
// ---------------------------------------------------------------------------

void URok2ArtAssets::BuildHumanUnitCatalog()
{
	if (bHumanCatalogBuilt) return;
	bHumanCatalogBuilt = true;

	auto AddUnit = [this](const TCHAR* Id, const TCHAR* File, const TCHAR* Folder, float Scale)
	{
		FRok2ArtEntry E;
		E.Id = Id;
		E.GlbFile = File;
		E.Folder = Folder;
		E.Scale = Scale;
		HumanUnitCatalog.Add(E);
	};

	// مشاة/رماة/فرسان T1–T5 (توليد إجرائي low-poly متسق مع KayKit)
	for (const TCHAR* Branch : { TEXT("infantry"), TEXT("archer"), TEXT("cavalry") })
	{
		for (int32 Tier = 1; Tier <= 5; ++Tier)
		{
				FString Id = FString::Printf(TEXT("%s_t%d"), Branch, Tier);
				AddUnit(*Id, *Id, TEXT("HumanUnits"), (FString(TEXT("cavalry")) == FString(Branch)) ? 1.9f : 1.75f);
		}
	}

	// حصار: T1–T2 إجرائي + T3–T5 من Kenney Castle Kit (CC0)
	AddUnit(TEXT("siege_t1"), TEXT("siege_arcuballista"), TEXT("HumanUnits"), 1.0f);
	AddUnit(TEXT("siege_t2"), TEXT("siege_mangonel"), TEXT("HumanUnits"), 1.0f);
	AddUnit(TEXT("siege_t3"), TEXT("siege-ballista"), TEXT("KenneyCastleKit"), 1.0f);
	AddUnit(TEXT("siege_t4"), TEXT("siege-trebuchet"), TEXT("KenneyCastleKit"), 1.0f);
	AddUnit(TEXT("siege_t5"), TEXT("siege-catapult"), TEXT("KenneyCastleKit"), 1.0f);

	// الوحدات الخاصة الحضارية — ترث شبكة فرعها عند unlock_tier=4 حتى توفر جلود
	// مخصصة (legionary/chu_ko_nu/desert_rider/khopesh_guard/huskarl/samurai).
	AddUnit(TEXT("legionary"), TEXT("infantry_t4"), TEXT("HumanUnits"), 1.75f);
	AddUnit(TEXT("khopesh_guard"), TEXT("infantry_t4"), TEXT("HumanUnits"), 1.75f);
	AddUnit(TEXT("huskarl"), TEXT("infantry_t4"), TEXT("HumanUnits"), 1.75f);
	AddUnit(TEXT("samurai"), TEXT("infantry_t4"), TEXT("HumanUnits"), 1.75f);
	AddUnit(TEXT("chu_ko_nu"), TEXT("archer_t4"), TEXT("HumanUnits"), 1.75f);
	AddUnit(TEXT("desert_rider"), TEXT("cavalry_t4"), TEXT("HumanUnits"), 1.9f);
}

FString URok2ArtAssets::GetHumanUnitId(const FString& Branch, int32 Tier, const FString& CivId)
{
	if (Branch.IsEmpty() || Tier < 1 || Tier > 5)
	{
		return FString();
	}
	// P8-T3: الوحدات الخاصة الحضارية تحل محل فرعها عند unlock_tier=4.
	if (Tier >= 4 && !CivId.IsEmpty())
	{
		if (CivId == TEXT("rome") && Branch == TEXT("infantry")) return TEXT("legionary");
		if (CivId == TEXT("egypt") && Branch == TEXT("infantry")) return TEXT("khopesh_guard");
		if (CivId == TEXT("vikings") && Branch == TEXT("infantry")) return TEXT("huskarl");
		if (CivId == TEXT("japan") && Branch == TEXT("infantry")) return TEXT("samurai");
		if (CivId == TEXT("china") && Branch == TEXT("archer")) return TEXT("chu_ko_nu");
		if (CivId == TEXT("arabia") && Branch == TEXT("cavalry")) return TEXT("desert_rider");
	}
	return FString::Printf(TEXT("%s_t%d"), *Branch, Tier);
}

FString URok2ArtAssets::GetHumanUnitAssetPath(const FString& UnitId)
{
	URok2ArtAssets* Assets = URok2ArtAssets::Get();
	Assets->BuildHumanUnitCatalog();
	for (const FRok2ArtEntry& E : Assets->HumanUnitCatalog)
	{
		if (E.Id == UnitId)
		{
			return FString::Printf(TEXT("/Game/Art/%s/%s.%s"), *E.Folder, *E.GlbFile, *E.GlbFile);
		}
	}
	return FString();
}

UStaticMesh* URok2ArtAssets::LoadHumanUnitMesh(const FString& UnitId)
{
	URok2ArtAssets* Assets = URok2ArtAssets::Get();
	Assets->BuildHumanUnitCatalog();
	static TMap<FString, UStaticMesh*> Cached;
	UStaticMesh** Found = Cached.Find(UnitId);
	if (Found)
	{
		return *Found;
	}
	const FString Path = GetHumanUnitAssetPath(UnitId);
	UStaticMesh* Mesh = Path.IsEmpty() ? nullptr : LoadObject<UStaticMesh>(nullptr, *Path);
	if (!Mesh)
	{
		UE_LOG(LogRok2Art, Verbose, TEXT("Human unit mesh '%s' not imported yet — geometric fallback stays active"), *UnitId);
	}
	Cached.Add(UnitId, Mesh);
	return Mesh;
}

bool URok2ArtAssets::HasHumanUnit(const FString& UnitId)
{
	URok2ArtAssets* Assets = URok2ArtAssets::Get();
	Assets->BuildHumanUnitCatalog();
	for (const FRok2ArtEntry& E : Assets->HumanUnitCatalog)
	{
		if (E.Id == UnitId) return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// P10-T7: أصول الحانة والصناديق والمفاتيح والمنحوتات والمواد ومخططات الحداد.
// ---------------------------------------------------------------------------

namespace
{
	static const TSet<FString> TavernMeshIds = {
		TEXT("building_tavern"),
		TEXT("chest_silver"),
		TEXT("chest_gold"),
		TEXT("chest_equipment")
	};

	static const TSet<FString> TavernIconIds = {
		// 3 Chests
		TEXT("chest_silver"),
		TEXT("chest_gold"),
		TEXT("chest_equipment"),
		// 6 Keys
		TEXT("key_silver"),
		TEXT("key_gold"),
		TEXT("key_equipment"),
		TEXT("key_expedition"),
		TEXT("key_canyon"),
		TEXT("key_osiris"),
		// 4 Sculptures
		TEXT("sculpture_legendary"),
		TEXT("sculpture_epic"),
		TEXT("sculpture_elite"),
		TEXT("sculpture_advanced"),
		// 4 Materials
		TEXT("material_leather"),
		TEXT("material_iron"),
		TEXT("material_ebony"),
		TEXT("material_crystal"),
		// 6 Blueprints
		TEXT("blueprint_weapon"),
		TEXT("blueprint_helm"),
		TEXT("blueprint_chest"),
		TEXT("blueprint_gloves"),
		TEXT("blueprint_legs"),
		TEXT("blueprint_boots")
	};
}

FString URok2ArtAssets::GetTavernMeshAssetPath(const FString& MeshId)
{
	if (TavernMeshIds.Contains(MeshId))
	{
		return FString::Printf(TEXT("/Game/Art/Tavern/%s.%s"), *MeshId, *MeshId);
	}
	return FString();
}

FString URok2ArtAssets::GetTavernIconAssetPath(const FString& IconId)
{
	if (TavernIconIds.Contains(IconId))
	{
		return FString::Printf(TEXT("/Game/Art/Tavern/%s.%s"), *IconId, *IconId);
	}
	return FString();
}

UTexture2D* URok2ArtAssets::LoadTavernIcon(const FString& IconId)
{
	if (!TavernIconIds.Contains(IconId))
	{
		return nullptr;
	}
	static TMap<FString, UTexture2D*> Cached;
	if (UTexture2D** Found = Cached.Find(IconId))
	{
		return *Found;
	}
	const FString AssetPath = GetTavernIconAssetPath(IconId);
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
	if (!Texture)
	{
		UE_LOG(LogRok2Art, Verbose, TEXT("Tavern icon '%s' (%s) not found in cooked/editor package"), *IconId, *AssetPath);
	}
	Cached.Add(IconId, Texture);
	return Texture;
}

bool URok2ArtAssets::HasTavernAsset(const FString& AssetId)
{
	return TavernMeshIds.Contains(AssetId) || TavernIconIds.Contains(AssetId);
}

