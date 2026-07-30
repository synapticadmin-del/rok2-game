// Copyright ROK2. Blueprint Helper Library Implementation for Unreal Engine 5.8.

#include "Rok2BlueprintLibrary.h"
#include "Rok2CivLore.h"

// ---------------------------------------------------------------------------
// P6-T5: قائمة الحضارات تُشتقّ من data/civilizations.json عبر URok2CivLore،
// ولا تُكتب هنا. اسم الدالة بقي على حاله لأن مستهلكيه قائمون، ومعناه صار
// «القائمة الأساسية» لا «قيم مكتوبة يدوياً».
//
// ما كان هنا قبل هذا البند لم يكن قديماً فحسب، كان **مُعطِّلاً للتسجيل**:
//
//   • «byzantium» كانت معروضة في قائمة اختيار الحضارة، وليست في
//     data/civilizations.json إطلاقاً. و/v1/city/init يرفض أي معرّف خارج الملف
//     بـ400 «Unknown civilization» — فلاعب يختار بيزنطة كان يصطدم بفشل تأسيس
//     المملكة على أول زرّ يضغطه في اللعبة. والوثيقة تقول ذلك صراحةً:
//     civilizations-visual-design.md §9 يُدرج Byzantium في **التوسع المستقبلي**.
//   • «egypt» كانت **غائبة** عن القائمة رغم أن لها ثيماً كاملاً (P5-T2) وقائد
//     بداية (cmd_egypt_starter) وصورة مدينة في assets/ — حضارة سادسة موجودة
//     في كل مكان إلا في الشاشة التي تُختار منها.
//   • أوصاف الفانتازي كانت تنطق بونصات مخالفة للملف (روما «سرعة حركة +5%»
//     والملف gathering_speed؛ الصين «دفاع المباني +10%» والملف building_speed؛
//     اليابان «كشافة +30%» والملف commander_xp) — وهو ما تمنعه AGENTS.md §3:
//     «لا قيم توازن ثابتة في الكود».
//   • وأسماء الوحدات الخاصة الثلاثة كانت مخالفة كذلك (Centurion/Mamluk/
//     Berserker مقابل legionary/desert_rider/huskarl).
//
// أي أن التصحيح ليس تنظيفاً استطرادياً بل شرطُ البند: نصٌّ يُؤلَّف في الملف
// «فيُخدم للطرفين» لا يصل أبداً إلى قائمةٍ تُبنى من مصدر آخر.
// ---------------------------------------------------------------------------
TArray<FRok2Civilization> URok2BlueprintLibrary::GetDefaultCivilizations()
{
	TArray<FRok2Civilization> List;

	URok2CivLore* Lore = URok2CivLore::Get();
	if (!Lore) return List;

	for (const FString& CivId : Lore->GetCivIds())
	{
		const FRok2CivLore& L = Lore->GetLore(CivId);
		if (!L.IsValid()) continue;

		FRok2Civilization C;
		C.Id = L.CivId;
		C.Name = L.NameLatin;
		C.NameAr = L.NameAr;
		C.Fantasy = L.FantasyLatin;
		C.FantasyAr = L.FantasyAr;
		C.SpecialUnit = L.SpecialUnitId;
		List.Add(C);
	}

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
