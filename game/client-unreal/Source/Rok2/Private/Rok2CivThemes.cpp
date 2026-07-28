// Copyright ROK2. Civilization visual themes (P5-T2) — implementation.

#include "Rok2CivThemes.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2CivThemes, Log, All);

URok2CivThemes* URok2CivThemes::Get()
{
	static URok2CivThemes* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2CivThemes>();
		Instance->AddToRoot();
		Instance->LoadFromDiskOrDefaults();
	}
	return Instance;
}

const FRok2CivTheme& URok2CivThemes::GetTheme(const FString& CivId) const
{
	for (const FRok2CivTheme& T : Themes)
	{
		if (T.CivId == CivId) return T;
	}
	// Fallback: أول ثيم في القائمة (عادة روما)
	check(Themes.Num() > 0);
	return Themes[0];
}

bool URok2CivThemes::HasTheme(const FString& CivId) const
{
	for (const FRok2CivTheme& T : Themes)
	{
		if (T.CivId == CivId) return true;
	}
	return false;
}

TArray<FString> URok2CivThemes::GetAvailableCivIds() const
{
	TArray<FString> Out;
	for (const FRok2CivTheme& T : Themes)
	{
		Out.Add(T.CivId);
	}
	return Out;
}

FRok2CivTheme URok2CivThemes::ParseThemeFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FRok2CivTheme T;
	if (!Obj.IsValid()) return T;

	Obj->TryGetStringField(TEXT("id"), T.CivId);
	Obj->TryGetStringField(TEXT("name"), T.DisplayName);

	const TSharedPtr<FJsonObject>* ThemeObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("theme"), ThemeObj) && ThemeObj && ThemeObj->IsValid())
	{
		FString PrimaryHex, SecondaryHex, AccentHex, ArchStyleStr;
		if ((*ThemeObj)->TryGetStringField(TEXT("primary"), PrimaryHex))
		{
			T.Primary = FLinearColor::FromSRGBColor(FColor::FromHex(PrimaryHex));
		}
		if ((*ThemeObj)->TryGetStringField(TEXT("secondary"), SecondaryHex))
		{
			T.Secondary = FLinearColor::FromSRGBColor(FColor::FromHex(SecondaryHex));
		}
		if ((*ThemeObj)->TryGetStringField(TEXT("accent"), AccentHex))
		{
			T.Accent = FLinearColor::FromSRGBColor(FColor::FromHex(AccentHex));
		}
		if ((*ThemeObj)->TryGetStringField(TEXT("architecture"), ArchStyleStr))
		{
			T.ArchStyle = ArchStyleFromString(ArchStyleStr);
		}
	}
	return T;
}

ERok2ArchStyle URok2CivThemes::ArchStyleFromString(const FString& StyleStr)
{
	if (StyleStr == TEXT("arches_marble")) return ERok2ArchStyle::ArchesMarble;
	if (StyleStr == TEXT("curved_roofs")) return ERok2ArchStyle::CurvedRoofs;
	if (StyleStr == TEXT("domes_arches")) return ERok2ArchStyle::DomesArches;
	if (StyleStr == TEXT("obelisks_columns")) return ERok2ArchStyle::ObelisksColumns;
	if (StyleStr == TEXT("carved_wood")) return ERok2ArchStyle::CarvedWood;
	if (StyleStr == TEXT("temple_wood")) return ERok2ArchStyle::TempleWood;
	return ERok2ArchStyle::ArchesMarble;
}

void URok2CivThemes::AddTheme(const FRok2CivTheme& Theme)
{
	// لا تكرار
	for (const FRok2CivTheme& T : Themes)
	{
		if (T.CivId == Theme.CivId) return;
	}
	Themes.Add(Theme);
}

void URok2CivThemes::BuildDefaults()
{
	// القيم تطابق data/civilizations.json و 07-game-design/civilizations-visual-design.md

	auto Add = [this](const TCHAR* Id, const TCHAR* Name, const TCHAR* PrimaryHex, const TCHAR* SecondaryHex, const TCHAR* AccentHex, ERok2ArchStyle Style)
	{
		FRok2CivTheme T;
		T.CivId = Id;
		T.DisplayName = Name;
		T.Primary = FLinearColor::FromSRGBColor(FColor::FromHex(PrimaryHex));
		T.Secondary = FLinearColor::FromSRGBColor(FColor::FromHex(SecondaryHex));
		T.Accent = FLinearColor::FromSRGBColor(FColor::FromHex(AccentHex));
		T.ArchStyle = Style;
		AddTheme(T);
	};

	Add(TEXT("rome"),     TEXT("Rome"),     TEXT("#8B1E1E"), TEXT("#D8C3A5"), TEXT("#C9A227"), ERok2ArchStyle::ArchesMarble);
	Add(TEXT("china"),    TEXT("China"),    TEXT("#B5121B"), TEXT("#F0C14A"), TEXT("#8B4513"), ERok2ArchStyle::CurvedRoofs);
	Add(TEXT("arabia"),   TEXT("Arabia"),   TEXT("#C9A227"), TEXT("#1F3D2B"), TEXT("#40E0D0"), ERok2ArchStyle::DomesArches);
	Add(TEXT("egypt"),    TEXT("Egypt"),    TEXT("#0E7C7B"), TEXT("#E1B84B"), TEXT("#D4AF37"), ERok2ArchStyle::ObelisksColumns);
	Add(TEXT("vikings"),  TEXT("Vikings"),  TEXT("#2E4057"), TEXT("#8AA0B4"), TEXT("#A0522D"), ERok2ArchStyle::CarvedWood);
	Add(TEXT("japan"),    TEXT("Japan"),    TEXT("#111111"), TEXT("#9B1D20"), TEXT("#F5F5F5"), ERok2ArchStyle::TempleWood);
}

void URok2CivThemes::LoadFromDiskOrDefaults()
{
	if (bLoaded) return;
	bLoaded = true;

	// محاولة قراءة data/civilizations.json من مجلد المحتوى (للتطوير المشترك)
	const FString JsonPath = FPaths::ProjectContentDir() / TEXT("../../data/civilizations.json");
	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		TSharedPtr<FJsonObject> RootObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* CivsArray = nullptr;
			if (RootObj->TryGetArrayField(TEXT("civilizations"), CivsArray) && CivsArray)
			{
				for (const TSharedPtr<FJsonValue>& V : *CivsArray)
				{
					const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
					if (V->TryGetObject(ObjPtr) && ObjPtr && ObjPtr->IsValid())
					{
						FRok2CivTheme T = ParseThemeFromJson(*ObjPtr);
						if (!T.CivId.IsEmpty())
						{
							AddTheme(T);
						}
					}
				}
				UE_LOG(LogRok2CivThemes, Log, TEXT("Loaded %d civ themes from %s"), Themes.Num(), *JsonPath);
				return;
			}
		}
		UE_LOG(LogRok2CivThemes, Warning, TEXT("Failed to parse %s — falling back to defaults"), *JsonPath);
	}
	else
	{
		UE_LOG(LogRok2CivThemes, Log, TEXT("civilizations.json not found at %s — using built-in defaults"), *JsonPath);
	}

	// Fallback: قيم افتراضية مدمجة (مطابقة للـ JSON)
	BuildDefaults();
}