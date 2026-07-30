// Copyright ROK2. Civilization narrative voice — lore registry (P6-T5).
//
// المواصفة (PLAN.md P6-T5): «النَفَس القصصي للحضارات الست: لكل حضارة نبذة
// أدبية (3-4 أسطر) تظهر عند اختيار الحضارة وفي شاشة معلوماتها + نبرة خاصة في
// نصوص الواجهة (تحية/تلميحات) — تُقرأ من data/civilizations.json (حقول
// story/greeting) فتُخدم للطرفين».
//
// هذا الملف هو **سجلّ النصّ** لا العرض: من أين يأتي النَفَس القصصي ومن يملكه.
// البطاقة في Rok2BootWidget (لحظة الاختيار) والشاشة في Rok2CivInfoWidget.
//
// ────────────────────────────────────────────────────────────────────────────
//  «فتُخدم للطرفين» — ثلاث طبقات لا واحدة، والسيرفر أعلاها
// ────────────────────────────────────────────────────────────────────────────
// النصّ مؤلَّف مرة واحدة في data/civilizations.json. من هناك يصل بثلاث طرق
// مرتّبة بالأولوية:
//
//   ١) **السيرفر** (أعلى سلطة): backend/src/data/civilizations.json نسخة طبق
//      الأصل، يخدمها /v1/meta/civilizations و/v1/meta/all. عند وصول الحمولة
//      يُستبدل السجلّ كاملاً — فتعديل نصٍّ أدبيّ ينزل على اللاعبين بنشر
//      backend وحده، بلا تحديث عميل. AGENTS.md §3: «الخادم هو السلطة».
//   ٢) **القرص** (تطوير): data/civilizations.json من شجرة المستودع، على نفس
//      اصطلاح URok2CivThemes — فالمصمّم يعدّل الملف ويرى النتيجة في PIE بلا
//      خادم ولا إعادة تجميع.
//   ٣) **مدمج** (الشحن): الطريقان أعلاه غير متاحين في أول إطار من بناء
//      Android — مجلد data/ خارج الحزمة، والسيرفر لم يُجب بعد. فبلا نسخة
//      مدمجة كانت أول شاشة يراها اللاعب بلا حضارات إطلاقاً.
//
// والنسخة المدمجة **مطابقة للـJSON حرفياً**، وذلك ليس أدباً بل شرطاً يفحصه
// verify_civ_lore: نسخة مدمجة تنحرف عن الملف تعني لاعبين يقرأون نصّاً غير
// الذي أُلّف، ولا شيء في المترجم يرصد ذلك.
//
// ────────────────────────────────────────────────────────────────────────────
//  الحكاية مصفوفة أسطر لا نصّاً واحداً
// ────────────────────────────────────────────────────────────────────────────
// story في الـJSON مصفوفة من 3-4 أسطر، لا سطرٌ واحد طويل بفواصل. السبب:
// مواضع القطع في نصٍّ أدبيّ عربي **قرار تأليف** — تقع على حدود الجُمَل فيقرأ
// السطر كبيتٍ مستقلّ. لفٌّ آلي على عرض البطاقة يقطع حيث اتّفق، فتنكسر الجملة
// في منتصفها ويضيع الوزن. والمصفوفة تجعل شرط «3-4 أسطر» قابلاً للقياس بدل أن
// يكون تقديراً بالعين.
//
// ────────────────────────────────────────────────────────────────────────────
//  لماذا سجلّ مستقلّ عن URok2CivThemes وهما يقرآن الملف نفسه
// ────────────────────────────────────────────────────────────────────────────
// الثيمات لون وعمارة تستهلكها ARok2BuildingActor في العالم ثلاثي الأبعاد؛
// والنَفَس القصصي نصٌّ تستهلكه الودجات. دمجهما كان سيجعل كل مبنى في المدينة
// يحمل معه أربعة أسطر شعر لا يقرأها أحد، وكل بطاقة نصّ تحمل ERok2ArchStyle.
// الفصل يبقي كل سجلٍّ بحجم مستهلكه — والملف واحد على أي حال.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rok2CivLore.generated.h"

// FJsonObject/FJsonValue غير منعكسين — تصريح أمامي كافٍ لاستخدام TSharedPtr،
// وJson وحدة عامة في Rok2.Build.cs (نفس ما يفعله Rok2CivThemes.h).
class FJsonObject;
class FJsonValue;

/**
 * هوية حضارة واحدة كنصّ. **لا لون ولا عمارة ولا أرقام بونص**: الأولان في
 * FRok2CivTheme، والأرقام بيانات توازن يخدمها الخادم ولا يُقتبس منها نصّ.
 */
USTRUCT(BlueprintType)
struct FRok2CivLore
{
	GENERATED_BODY()

	/** معرف الحضارة (rome, china, arabia, egypt, vikings, japan) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString CivId;

	/** الاسم العربي — «روما»، «الفايكنج». من name_ar. */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString NameAr;

	/** الاسم كما في البيانات (لاتيني) — للسجلات والتشخيص */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString NameLatin;

	/** جملة الفانتازي الواحدة (theme-and-values §1 Pillar Fantasy). من fantasy_ar. */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString FantasyAr;

	/** جملة الفانتازي كما في البيانات (لاتيني) — من fantasy. مرافقة لـNameLatin. */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString FantasyLatin;

	/** النبذة الأدبية — 3-4 أسطر، كل سطر جملة تامّة */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	TArray<FString> Story;

	/** تحية الحضارة — تُقال للاعب بنبرتها عند دخول المدينة */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString Greeting;

	/**
	 * تلميحات بنبرة الحضارة. كل تلميح يشير إلى بونص **حقيقي** في bonuses بلا
	 * ذكر رقمه: الأرقام بيانات توازن يخدمها الخادم، ونصٌّ يقول «+5%» يصير
	 * كذبةً موثّقة لحظة تعديل قيمة في الملف (AGENTS.md §3: لا قيم توازن ثابتة).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	TArray<FString> Hints;

	/**
	 * معرّف الوحدة الخاصة من special_unit.id — جزء من هوية الحضارة لا من
	 * توازنها (المعرّف لا إحصاءاته). موجود هنا لأنه كان **ثلاث قيم خاطئة**
	 * مكتوبة يدوياً في العميل: «Centurion» و«Mamluk» و«Berserker» بينما
	 * البيانات تقول legionary وdesert_rider وhuskarl. حمله من الملف يجعل
	 * FRok2Civilization مشتقّة بالكامل، فلا يبقى حقل يخترع قيمته.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|Lore")
	FString SpecialUnitId;

	/** هل هذا سجلّ حقيقي (لا القيمة الفارغة التي تُعاد عند معرّف مجهول)؟ */
	bool IsValid() const { return !CivId.IsEmpty(); }
};

/**
 * حدود بنيوية للنصّ — **ليست قيم توازن**. مصدر واحد كما في Rok2FtueSpec،
 * يقرأها السجلّ عند التحقق ويقرأها الحرس عند الفحص، فلا رقمان للشرط نفسه.
 */
namespace Rok2CivLoreSpec
{
	/** أقلّ عدد أسطر في النبذة — من نصّ البند «(3-4 أسطر)» */
	static constexpr int32 MinStoryLines = 3;

	/** أكثر عدد أسطر في النبذة */
	static constexpr int32 MaxStoryLines = 4;

	/** الحضارات الست في الإطلاق (GDD §2) — حرس ضد نسخة مدمجة ناقصة */
	static constexpr int32 ExpectedCivCount = 6;

	/** مسار الملف نسبةً إلى Content — نفس اصطلاح URok2CivThemes */
	extern ROK2_API const FString JsonRelativePath;
}

static_assert(Rok2CivLoreSpec::MinStoryLines <= Rok2CivLoreSpec::MaxStoryLines,
	"Rok2CivLoreSpec: مدى أسطر النبذة مقلوب");

/**
 * سجلّ النَفَس القصصي. نسخة مشتركة (AddToRoot) على اصطلاح URok2CivThemes
 * وURok2Typography — لأن الحمولة تصل من الشبكة مرة واحدة ويجب أن تنجو من
 * إعادة بناء الودجات، والودجات تُبنى وتُهدم بحرية في هذا المشروع.
 */
UCLASS(BlueprintType)
class ROK2_API URok2CivLore : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|Lore")
	static URok2CivLore* Get();

	// ---------------------------------------------------------------------
	// قراءة
	// ---------------------------------------------------------------------

	/**
	 * سجلّ حضارة بمعرفها. عند معرّف مجهول يعيد سجلاً **فارغاً** لا أول عنصر:
	 * حضارة خاطئة تعرض نصّ حضارة أخرى عطلٌ صامت، أما بطاقة فارغة فتُرى فوراً.
	 * والمنادي يسأل IsValid() أو HasLore() قبل العرض.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	const FRok2CivLore& GetLore(const FString& CivId) const;

	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	bool HasLore(const FString& CivId) const;

	/** المعرّفات بترتيب البيانات — ترتيب البطاقات في شاشة الاختيار */
	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	TArray<FString> GetCivIds() const;

	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	int32 Num() const { return Entries.Num(); }

	/** النبذة كنصّ واحد بفواصل أسطر — للـTextBlock مباشرة */
	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	FString StoryText(const FString& CivId) const;

	/** تلميح بالفهرس، ملفوف دورياً. نصّ فارغ إن لا تلميحات لهذه الحضارة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	FString HintAt(const FString& CivId, int32 Index) const;

	/** هل السجلّ الحالي من حمولة الخادم؟ (وإلا فمن القرص أو المدمج) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Lore")
	bool IsFromServer() const { return bFromServer; }

	// ---------------------------------------------------------------------
	// كتابة — مسار الخادم
	// ---------------------------------------------------------------------

	/**
	 * يستبدل السجلّ من مصفوفة civilizations في حمولة الخادم.
	 * يعيد true إن حلّت حمولة صالحة (سجلّ واحد على الأقل بمعرّف ونبذة).
	 *
	 * **لا يستبدل بحمولة عرجاء:** استجابة فيها ids بلا story (نسخة backend
	 * قديمة) كانت ستمحو النصّ المدمج وتُترك الشاشة بلا حكاية. فالاستبدال
	 * ذرّي — إمّا سجلّ كامل أو يبقى القديم.
	 *
	 * **ليست UFUNCTION:** TArray<TSharedPtr<FJsonValue>> نوع غير منعكس، فـUHT
	 * يرفضه معاملاً لدالة منعكسة. نفس سبب بقاء
	 * URok2CivThemes::ParseThemeFromJson دالةً عادية.
	 */
	bool ApplyServerCivs(const TArray<TSharedPtr<FJsonValue>>& CivsArray);

	/** يملأ FRok2CivLore من كائن JSON واحد (نفس المُحلِّل للقرص والخادم) */
	static FRok2CivLore ParseFromJson(const TSharedPtr<FJsonObject>& Obj);

	/** يتحقق من اكتمال سجلّ: معرّف + اسم عربي + نبذة داخل المدى + تحية */
	static bool IsCompleteEntry(const FRok2CivLore& Entry);

protected:
	/** السجلات بترتيب المصدر */
	TArray<FRok2CivLore> Entries;

	/** يُعاد عند معرّف مجهول — عضو لا مؤقّت، فالمرجع المُعاد يجب أن يعيش */
	FRok2CivLore EmptyEntry;

	bool bLoaded = false;
	bool bFromServer = false;

	/** القرص أولاً، فإن تعذّر فالمدمج */
	void LoadFromDiskOrDefaults();

	/** يقرأ مصفوفة civilizations من نصّ JSON. يعيد عدد ما حُمِّل. */
	int32 LoadFromJsonString(const FString& JsonString);

	/**
	 * النسخة المدمجة — **مطابقة لـdata/civilizations.json حرفياً**.
	 * verify_civ_lore يقارن الاثنين نصّاً بنصّ، فالانحراف يفشل الحرس.
	 */
	void BuildDefaults();

	/** يضيف سجلاً إن كان مكتملاً وغير مكرَّر. يعيد true إن أُضيف. */
	bool AddEntry(const FRok2CivLore& Entry);
};
