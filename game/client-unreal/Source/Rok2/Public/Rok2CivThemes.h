// Copyright ROK2. Civilization visual themes (P5-T2) — palette + architectural identity.
//
// يقرأ ثيم الحضارة (ألوان + نمط عمارة) من data/civilizations.json على القرص
// أو من ذاكرة مدمجة إذا تعذر الوصول للملف. يستخدمه Rok2BuildingActor لتلوين
// وتشكيل المباني (City Hall + 5 مبانٍ أساسية) لكل حضارة من الست.
// المرجع: 07-game-design/civilizations-visual-design.md

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rok2CivThemes.generated.h"

// FJsonObject غير منعكس — تصريح أمامي كافٍ لاستخدام TSharedPtr.
class FJsonObject;

/** نمط العمارة الذي يحدد شكل السقف/الزخارف placeholder. */
UENUM(BlueprintType)
enum class ERok2ArchStyle : uint8
{
	ArchesMarble,   // روما: أقواس وقناطر رخامية
	CurvedRoofs,    // الصين: أسقف منحنية الأطراف بطبقات
	DomesArches,    // العرب: قباب فيروزية وذهبية
	ObelisksColumns,// مصر: مسلات وأعمدة بردي
	CarvedWood,     // الفايكنج: خشب طويل برؤوس تنانين
	TempleWood      // اليابان: قلعة tenshu خشبية داكنة
};

/** لوحة ألوان حضارة واحدة (ثيم كامل للمباني). */
USTRUCT(BlueprintType)
struct FRok2CivTheme
{
	GENERATED_BODY()

	/** معرف الحضارة (rome, china, arabia, egypt, vikings, japan) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString CivId;

	/** الاسم المعروض */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FString DisplayName;

	/** اللون الرئيسي (الجدران/الحجر) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor Primary = FLinearColor::White;

	/** اللون الثانوي (السقف/الخشب/الزخارف) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor Secondary = FLinearColor::White;

	/** لون التمييز (ذهب/نحاس/تفاصيل) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor Accent = FLinearColor::White;

	// P6-T7: ألوان خلفية اللوحات لهذه الحضارة
	/** لون الخلفية الأساسي للوحة (أغمق من Primary) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor PanelBg = FLinearColor(0.06f, 0.05f, 0.04f, 0.92f);

	/** لون الخلفية الثانوي للوحة (للإطار السفلي أو التدرج) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor PanelBgAlt = FLinearColor(0.1f, 0.08f, 0.05f, 0.85f);

	/** لون الإطار المزخرف حول اللوحة */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	FLinearColor PanelFrame = FLinearColor(0.79f, 0.63f, 0.15f, 0.6f);

	/** نمط العمارة الذي يحدد شكل الـ placeholder */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2")
	ERok2ArchStyle ArchStyle = ERok2ArchStyle::ArchesMarble;
};

UCLASS(BlueprintType)
class ROK2_API URok2CivThemes : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) — تحمّل الثيمات مرة واحدة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2CivThemes* Get();

	/** يعيد ثيم حضارة بمعرفها (مثل "rome"). إن لم توجد يعيد الثيم الافتراضي (روما). */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2CivTheme& GetTheme(const FString& CivId) const;

	/** يتحقق من وجود ثيم للحضارة. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool HasTheme(const FString& CivId) const;

	/** يعيد قائمة معرفات الحضارات المتاحة. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	TArray<FString> GetAvailableCivIds() const;

	/** يملأ FRok2CivTheme من كائن JSON (تُستخدم من الـ API أو الملف). */
	static FRok2CivTheme ParseThemeFromJson(const TSharedPtr<FJsonObject>& Obj);

	/** يحوّل نص نمط العمارة إلى ERok2ArchStyle. */
	static ERok2ArchStyle ArchStyleFromString(const FString& StyleStr);

protected:
	UPROPERTY(Transient)
	TArray<FRok2CivTheme> Themes;

	bool bLoaded = false;

	void LoadFromDiskOrDefaults();

	/** يبني الثيمات الست من القيم الافتراضية (تطابق data/civilizations.json). */
	void BuildDefaults();

	/** يضيف ثيماً إلى القائمة. */
	void AddTheme(const FRok2CivTheme& Theme);
};