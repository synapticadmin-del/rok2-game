// Copyright ROK2. Art asset library (P2-T7) — implementation.

#include "Rok2ArtAssets.h"
#include "Rok2IconLibrary.h"
#include "Engine/StaticMesh.h"
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
	return FString::Printf(TEXT("/Game/Art/kaykit/%s.%s"), *GlbFile, *GlbFile);
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

#if WITH_EDITOR
	// في المحرر: لو استُورد الـ GLB مسبقاً كأصل uasset يمكن تحميله مباشرة
	Mesh = LoadObject<UStaticMesh>(nullptr, *EditorPackagePath(Entry->GlbFile));
#endif

	// ملاحظة: التحميل من ملف .glb على القرص يتطلب موديول استيراد glTF (مثل glTFRuntime)
	// — في غيابه نعيد nullptr ويبقى الراسم على الأشكال الهندسية الافتراضية (fallback).
	if (!Mesh)
	{
		UE_LOG(LogRok2Art, Verbose, TEXT("Art mesh for '%s' not imported yet (%s) — geometric fallback stays active"),
			*Id, *DiskPath(Entry->GlbFile));
	}

	Loaded.Add(Id, Mesh);
	return Mesh;
}

// ---------------------------------------------------------------------------
// P6-T1: نظام أيقونات UI الموحد — تفويض لـ URok2IconLibrary
// ---------------------------------------------------------------------------

FSlateBrush URok2ArtAssets::GetIconBrush(const FString& IconId, float Size, FLinearColor Tint)
{
	return URok2IconLibrary::BrushFromArtAssets(IconId, Size, Tint);
}

bool URok2ArtAssets::HasIcon(const FString& IconId)
{
	return URok2IconLibrary::Get()->HasIcon(IconId);
}
