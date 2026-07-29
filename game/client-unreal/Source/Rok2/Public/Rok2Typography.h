// Copyright ROK2. Unified Arabic/Latin typography system (P6-T2).
//
// نظام الخطوط الموحّد — ثلاثة أوجه و12 دوراً نصياً، من ui-ux-design-system.md §1:
//   «عنوان: خط عربي فخم (Cairo Black / Aref Ruqaa للعناوين الكبرى)
//    · أرقام/إنجليزي: Trajan-style serif أو Cinzel»
//
// المشكلة التي يحلّها: كل ودجت كان يكتب
//     FSlateFontInfo F = T->GetFont(); F.Size = 15; T->SetFont(F);
// أي أنه يورّث خط المحرك الافتراضي ويعدّل الحجم فقط — فلا سيطرة على عائلة
// الخط ولا على الوزن، والأحجام أرقام سحرية متفرقة (12/14/15/16/17/18/22/24)
// في ثمانية ملفات. النتيجة: العربي والأرقام بنفس الوجه المحايد، ولا تمييز
// بصري بين عنوان لوحة ورقم مورد ومؤقّت طابور.
//
// الاستخدام:
//     URok2Typography::ApplyFont(TitleText, ERok2TextRole::Title);
//     ResourceText->SetFont(URok2Typography::Font(ERok2TextRole::Numeric));
//
// **الخط لا يحمل لوناً.** الألوان تبقى مسؤولية كل ودجت (Rok2HudStyle وأخواته)
// لأن كثيراً منها ديناميكي (شارة الاتصال، الجرس، تعذّر التكلفة). ApplyFont
// يمسّ الخط وحده ولا يلمس SetColorAndOpacity أبداً.
//
// **التدهور اللطيف (graceful fallback)** — نفس اصطلاح URok2ArtAssets مع GLB:
// إن وُجد أصل خط حقيقي في Content/Fonts استُخدم، وإلا رجعنا إلى خط المحرك
// الافتراضي **بنفس الحجم والوزن**. فالسلم البصري (تراتب الأحجام والأوزان)
// يعمل فوراً بلا أي أصل خارجي، ويكسب الطابع «المخطوطة الملكية» تلقائياً لحظة
// إسقاط الملفات في Content/Fonts — دون تعديل سطر واحد في الودجات.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "Rok2Typography.generated.h"

class UFont;
class UTextBlock;

/** الأوجه الثلاثة من وثيقة UI §1 */
UENUM(BlueprintType)
enum class ERok2Face : uint8
{
	/** عربي فخم للعناوين الكبرى — Aref Ruqaa (شاشة البدء، اختيار الحضارة) */
	Display	UMETA(DisplayName = "Display (Aref Ruqaa)"),
	/** عربي واضح لواجهة التشغيل — Cairo (عناوين اللوحات والنصوص والأزرار) */
	Ui		UMETA(DisplayName = "UI (Cairo)"),
	/** أرقام ولاتيني بطابع Trajan — Cinzel (الموارد والمؤقّتات والأعداد) */
	Numeric	UMETA(DisplayName = "Numeric (Cinzel)")
};

/**
 * الأدوار النصية. الودجات تطلب **دوراً** لا حجماً — فالحجم والوزن والوجه
 * تُقرَّر في مكان واحد، وأي إعادة ضبط للسلم تسري على كل الشاشات مرة واحدة.
 */
UENUM(BlueprintType)
enum class ERok2TextRole : uint8
{
	/** أكبر عنوان في اللعبة — اسم اللعبة، اسم الحضارة عند الاختيار */
	Display,
	/** عنوان لوحة أو شاشة */
	Title,
	/** عنوان قسم داخل لوحة */
	Subtitle,
	/** عنوان مضغوط عريض — لوحات الـHUD الصغيرة وصفوف القوائم الكثيفة */
	TitleCompact,
	/** عنوان بطاقة أو فرع مواهب — عريض دون أن يكون عنوان قسم */
	CardTitle,
	/** نص الجسم العادي */
	Body,
	/** نص جسم أصغر — أسطر الإحصاءات والنجوم واللافتات الوصفية */
	BodySmall,
	/** نص داخل زر */
	Button,
	/** شارة عدّ، عنوان توست، طابع زمني */
	Caption,
	/** أصغر لافتة — تحت الأزرار الدائرية، سطور الطوابير، جسم التوست */
	Micro,
	/** رقم مورد في الشريط العلوي */
	Numeric,
	/** مؤقّت طابور أو عدّاد تنازلي */
	Timer
};

/**
 * درجات السلم — **مقاسة من الكود القائم لا مُختَرَعة**. الأحجام المستخدمة فعلاً
 * في 32 موضعاً كانت: 8×1، 11×5، 12×7، 13×3، 14×3، 15×5، 16×1، 17×1، 18×3،
 * 22×1، 24×2. الدرجات الثماني أدناه تستوعبها كلها بانحراف ±1px كأقصى تقدير،
 * فلا تتغيّر أبعاد أي شاشة قائمة.
 *
 * الاستثناء الوحيد المقصود: الحجم 8 (لافتة تحت الأزرار الدائرية الصغيرة) يصير
 * Micro=11. الزر قطره 58px ويحمل أيقونة 26px، فـ26+11 يبقى داخله بسهولة، و8px
 * للعربية غير مقروء أصلاً على الهاتف في لعبة تستهدف PC + Android.
 *
 * الفصل مقصود: الدرجات هنا **مقاسات**، والأدوار في ERok2TextRole **معانٍ**.
 * دور واحد قد يشترك مع آخر في الدرجة (Timer وNumeric على Compact) بلا تكرار رقم.
 */
namespace Rok2TypeScale
{
	/** لافتات الواجهة الكثيفة — تحت الأزرار الدائرية، سطور الطوابير، جسم التوست */
	static constexpr float Micro    = 11.f;
	/** شارات العدّ، عناوين التوست، الطوابع الزمنية */
	static constexpr float Caption  = 12.f;
	/** أرقام الموارد والمؤقّتات وعناوين الـHUD — الطبقة الكثيفة فوق العالم */
	static constexpr float Compact  = 14.f;
	/** نص الجسم في اللوحات (Bottom Sheets) */
	static constexpr float Body     = 15.f;
	/** نص داخل الأزرار */
	static constexpr float Button   = 16.f;
	/** عنوان قسم داخل لوحة */
	static constexpr float Subtitle = 18.f;
	/** عنوان لوحة أو شاشة */
	static constexpr float Title    = 22.f;
	/** أكبر عنوان — شاشة البدء واختيار الحضارة */
	static constexpr float Display  = 24.f;

	/** المدى المقبول في الواجهة — حرس ضد قيمة شاذة */
	static constexpr float Min = 11.f;
	static constexpr float Max = 48.f;
}

// السلم تصاعدي فعلاً — لو عدّل أحدهم قيمة فأفسد التراتب، يفشل البناء لا الشكل
static_assert(Rok2TypeScale::Micro < Rok2TypeScale::Caption
	&& Rok2TypeScale::Caption < Rok2TypeScale::Compact
	&& Rok2TypeScale::Compact < Rok2TypeScale::Body
	&& Rok2TypeScale::Body < Rok2TypeScale::Button
	&& Rok2TypeScale::Button < Rok2TypeScale::Subtitle
	&& Rok2TypeScale::Subtitle < Rok2TypeScale::Title
	&& Rok2TypeScale::Title < Rok2TypeScale::Display,
	"Rok2TypeScale: سلم الأحجام يجب أن يبقى تصاعدياً (Micro < ... < Display)");

static_assert(Rok2TypeScale::Micro >= Rok2TypeScale::Min
	&& Rok2TypeScale::Display <= Rok2TypeScale::Max,
	"Rok2TypeScale: كل الدرجات داخل المدى المقبول [Min, Max]");

/** أوزان الخط — تُترجم إلى اسم Typeface داخل أصل الخط */
namespace Rok2TypeWeight
{
	/** أثقل وزن — للعناوين الكبرى فقط */
	extern ROK2_API const FName Black;
	/** وزن العناوين والأزرار */
	extern ROK2_API const FName Bold;
	/** وزن النص العادي */
	extern ROK2_API const FName Regular;
}

UCLASS(BlueprintType)
class ROK2_API URok2Typography : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) — تحلّ الأوجه عند أول طلب وتخبّئها */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Typography")
	static URok2Typography* Get();

	/**
	 * مسار أصل الخط المتوقع لوجه ما.
	 * إسقاط ملف بهذا الاسم في Content/Fonts يفعّل الوجه الحقيقي تلقائياً.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static FString FacePackagePath(ERok2Face Face);

	/** اسم أصل الخط لوجه (بدون مسار) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static FString FaceAssetName(ERok2Face Face);

	/** هل حُمّل أصل خط حقيقي لهذا الوجه؟ false ⇒ نعمل على خط المحرك الافتراضي */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Typography")
	bool HasFace(ERok2Face Face);

	/** الوجه المخصّص لدور نصي */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static ERok2Face FaceOf(ERok2TextRole Role);

	/** حجم دور نصي بالنقاط */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static float SizeOf(ERok2TextRole Role);

	/** وزن دور نصي (اسم Typeface) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static FName WeightOf(ERok2TextRole Role);

	/**
	 * FSlateFontInfo كامل لدور نصي — الوجه الحقيقي إن توفّر، وإلا خط المحرك
	 * بنفس الحجم والوزن.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static FSlateFontInfo Font(ERok2TextRole Role);

	/**
	 * يطبّق خط الدور على TextBlock. **لا يلمس اللون** — اللون يبقى لكل ودجت.
	 * آمن مع nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Typography")
	static void ApplyFont(UTextBlock* Text, ERok2TextRole Role);

	/**
	 * منفذ للنصوص ذات الحجم **المحسوب** (لا الثابت) — كالحرف الأول داخل بورتريه
	 * دائري حيث الحجم نسبة من قطر الدائرة. الدور لا يصلح هنا لأن الحجم يتغيّر
	 * مع الحاوية، لكن الوجه والوزن يبقيان من النظام فلا يشرد النص عن العائلة.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static FSlateFontInfo FontSized(ERok2Face Face, float Size, bool bBold = true);

	/** كل الأدوار (للاختبار والتوثيق) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Typography")
	static TArray<ERok2TextRole> AllRoles();

private:
	/** يحلّ وجهاً إلى أصل خط، ويخبّئ النتيجة (بما فيها الفشل) */
	UFont* ResolveFace(ERok2Face Face);

	/** خبأ الأوجه المحلولة — UPROPERTY ليحميها من الـGC */
	UPROPERTY()
	TMap<uint8, UFont*> FaceCache;

	/** أوجه حاولنا تحميلها وفشلت — لا نعيد المحاولة كل إطار */
	UPROPERTY()
	TSet<uint8> FaceMisses;
};
