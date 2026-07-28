// Copyright ROK2. Blueprint Helper Library Implementation for Unreal Engine 5.8.

#include "Rok2BlueprintLibrary.h"

TArray<FRok2Civilization> URok2BlueprintLibrary::GetDefaultCivilizations()
{
	TArray<FRok2Civilization> List;

	FRok2Civilization Rome;
	Rome.Id = TEXT("rome");
	Rome.Name = TEXT("روما (Rome)");
	Rome.Fantasy = TEXT("فرسان الصليب والإمبراطورية العظمى — مشاة +5%، سرعة حركة +5%");
	Rome.SpecialUnit = TEXT("Centurion");
	List.Add(Rome);

	FRok2Civilization Arabia;
	Arabia.Id = TEXT("arabia");
	Arabia.Name = TEXT("العرب (Arabia)");
	Arabia.Fantasy = TEXT("صقراء الصحراء — فرسان +5%، سرعة جمع الموارد +10%");
	Arabia.SpecialUnit = TEXT("Mamluk");
	List.Add(Arabia);

	FRok2Civilization China;
	China.Id = TEXT("china");
	China.Name = TEXT("الصين (China)");
	China.Fantasy = TEXT("تنين المشرق — دفاع المباني +10%، سرعة البناء +5%");
	China.SpecialUnit = TEXT("Cho-Ko-Nu");
	List.Add(China);

	FRok2Civilization Byzantium;
	Byzantium.Id = TEXT("byzantium");
	Byzantium.Name = TEXT("بيزنطة (Byzantium)");
	Byzantium.Fantasy = TEXT("حصن الشرق — مستشفى +15%، سعة الجيش +5%");
	Byzantium.SpecialUnit = TEXT("Cataphract");
	List.Add(Byzantium);

	FRok2Civilization Vikings;
	Vikings.Id = TEXT("vikings");
	Vikings.Name = TEXT("الفايكنج (Vikings)");
	Vikings.Fantasy = TEXT("غزاة الشمال — هجوم المشاة +5%، ضرر الهجوم المضاد +5%");
	Vikings.SpecialUnit = TEXT("Berserker");
	List.Add(Vikings);

	FRok2Civilization Japan;
	Japan.Id = TEXT("japan");
	Japan.Name = TEXT("اليابان (Japan)");
	Japan.Fantasy = TEXT("محاربو الساموراي — هجوم جميع القوات +3%، سرعة الكشافة +30%");
	Japan.SpecialUnit = TEXT("Samurai");
	List.Add(Japan);

	return List;
}

TArray<FRok2MapRegion> URok2BlueprintLibrary::GetDefaultMapRegions()
{
	TArray<FRok2MapRegion> Regions;

	auto AddRegion = [&](const FString& Id, int32 ZoneId, const FString& Name, double MinX, double MinY, double MaxX, double MaxY)
	{
		FRok2MapRegion R;
		R.Id = Id;
		R.ZoneId = ZoneId;
		R.Name = Name;
		R.Aabb = { MinX, MinY, MaxX, MaxY };
		Regions.Add(R);
	};

	// Zone 1
	AddRegion(TEXT("R1"), 1, TEXT("منطقة 1 - الشمال الغربي"), 0, 1800, 600, 2400);
	AddRegion(TEXT("R2"), 1, TEXT("منطقة 2 - الشمال"), 600, 1800, 1200, 2400);
	AddRegion(TEXT("R3"), 1, TEXT("منطقة 3 - الشمال الشرقي"), 1200, 1800, 1800, 2400);
	AddRegion(TEXT("R4"), 1, TEXT("منطقة 4 - الشرق"), 1800, 1200, 2400, 1800);
	AddRegion(TEXT("R5"), 1, TEXT("منطقة 5 - الجنوب الشرقي"), 1800, 0, 2400, 600);
	AddRegion(TEXT("R6"), 1, TEXT("منطقة 6 - الجنوب"), 1200, 0, 1800, 600);
	AddRegion(TEXT("R7"), 1, TEXT("منطقة 7 - الجنوب الغربي"), 600, 0, 1200, 600);
	AddRegion(TEXT("R8"), 1, TEXT("منطقة 8 - الغرب"), 0, 600, 600, 1200);

	// Zone 2
	AddRegion(TEXT("Z2_NW"), 2, TEXT("المنطقة 2 - الشمال الغربي"), 600, 1200, 1200, 1800);
	AddRegion(TEXT("Z2_NE"), 2, TEXT("المنطقة 2 - الشمال الشرقي"), 1200, 1200, 1800, 1800);
	AddRegion(TEXT("Z2_SW"), 2, TEXT("المنطقة 2 - الجنوب الغربي"), 600, 600, 1200, 1200);
	AddRegion(TEXT("Z2_SE"), 2, TEXT("المنطقة 2 - الجنوب الشرقي"), 1200, 600, 1800, 1200);

	// Zone 3
	AddRegion(TEXT("Z3_CENTER"), 3, TEXT("المملكة المفقودة - المركز"), 900, 900, 1500, 1500);

	return Regions;
}

FVector URok2BlueprintLibrary::WorldToUnrealLocation(FVector2D WorldPos, float ZHeight, float MapScale)
{
	// Map center offset (1200, 1200) -> Unreal origin (0, 0)
	float UnrealX = (WorldPos.X - 1200.0f) * MapScale;
	float UnrealY = (WorldPos.Y - 1200.0f) * MapScale;
	return FVector(UnrealX, UnrealY, ZHeight);
}

FVector2D URok2BlueprintLibrary::UnrealToWorldLocation(FVector UnrealPos, float MapScale)
{
	float WorldX = (UnrealPos.X / MapScale) + 1200.0f;
	float WorldY = (UnrealPos.Y / MapScale) + 1200.0f;
	return FVector2D(WorldX, WorldY);
}

FString URok2BlueprintLibrary::FormatResourceNumber(double Value)
{
	if (Value >= 1000000.0)
	{
		return FString::Printf(TEXT("%.1fM"), Value / 1000000.0);
	}
	if (Value >= 1000.0)
	{
		return FString::Printf(TEXT("%.1fK"), Value / 1000.0);
	}
	return FString::Printf(TEXT("%.0f"), Value);
}

#include "Blueprint/UserWidget.h"

float URok2BlueprintLibrary::GetDistance2D(FVector2D LocationA, FVector2D LocationB)
{
	return FVector2D::Distance(LocationA, LocationB);
}

UUserWidget* URok2BlueprintLibrary::CreateRok2Widget(UObject* WorldContextObject, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WorldContextObject || !WidgetClass) return nullptr;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return nullptr;
	return CreateWidget<UUserWidget>(World, WidgetClass);
}
