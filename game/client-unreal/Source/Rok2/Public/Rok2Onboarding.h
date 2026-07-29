// Copyright ROK2. First-minute onboarding — FTUE model (P6-T4).
//
// المواصفة (PLAN.md P6-T4): «Onboarding أول دقيقة (FTUE): خطوات مرشدة داخل
// اللعبة (بناء أول مزرعة ← تدريب أول جنود ← أول مسيرة جمع) ببطاقات إرشاد
// أنيقة + إبراز الزر المطلوب — نصوص عربية أدبية قصيرة بنَفَس قصصي».
//
// هذا الملف هو **النموذج** لا العرض: الخطوات ونصوصها وشروط إتمامها ومنطق
// التقدّم. البطاقة والحلقة الذهبية في Rok2OnboardingWidget — الفصل مقصود
// لأن منطق «أين اللاعب الآن؟» قابل للفحص وحده بلا ودجة ولا شاشة.
//
// ────────────────────────────────────────────────────────────────────────────
//  القرار المركزي: الخطوة **تُستنتج** من حالة السيرفر ولا تُخزَّن
// ────────────────────────────────────────────────────────────────────────────
// التنفيذ البديهي يحفظ «رقم الخطوة = 2» على القرص. لم أفعل ذلك لثلاثة أسباب
// من هذا المستودع بعينه:
//
//   ١) AGENTS.md §3: «الخادم هو السلطة». الحقائق الثلاث موجودة أصلاً عند
//      الخادم وتصل عبر مفوَّضات قائمة: هل يملك اللاعب farm، هل عنده جنود،
//      هل له مسيرة. تخزين نسخة ثانية منها محلياً يخلق مصدر حقيقة ثانياً.
//   ٢) لا توجد طبقة تخزين في الموديول كله (لا USaveGame ولا GConfig). اختراع
//      واحدة لأجل راية تعليمية = نظام فرعي يُصان + عطل جديد ممكن: الراية
//      تقول «انتهى» والمدينة خالية.
//   ٣) الاستنتاج يشتري سلوكاً حقيقياً بالمجان: الإرشاد يستأنف صحيحاً بعد قتل
//      التطبيق في منتصف خطوة، وبعد إعادة التثبيت، وعلى جهاز ثانٍ — لا راية
//      تتعارض مع الواقع.
//
// فالخطوة دالة على الحالة الحيّة:
//
//   لا farm ولا طابور بناء له            → BuildFarm
//   farm موجود، لا جنود ولا طابور تدريب   → TrainTroops
//   جنود موجودون، لا مسيرة للاعب          → GatherMarch
//   الثلاثة تحققت                         → Done (لا يظهر مرة أخرى)
//
// ────────────────────────────────────────────────────────────────────────────
//  الثغرة الواحدة في ذلك، والمِزلاج الذي يسدّها
// ────────────────────────────────────────────────────────────────────────────
// المُسنَد الصافي **يرتدّ**: محاربٌ قديم فقد جنوده كلهم في معركة سيُعرض عليه
// «درّب أول جنودك» من جديد — وهذا استصغار للاعب، وعطل لا ميزة.
//
// فالاستنتاج مغلَّف بمِزلاج **أحادي الاتجاه**:
//   • لا يُسلَّح الإرشاد إلا إذا كان اللاعب مبتدئاً فعلاً عند **أول** تقييم.
//   • بعد ذلك تتقدّم الخطوة إلى الأمام فقط (Ordinal تصاعدي) ولا ترتدّ أبداً.
//   • Done نهائية: متى لوحظت لا يعود الإرشاد في هذه الجلسة.
//   • اللاعب العائد يُصنَّف Done عند أول تقييم فلا تُبنى الطبقة أصلاً.
//
// وأول تقييم لا يجري قبل أن تصل حالة حقيقية (IsStateReady): مدينة فارغة قبل
// وصول البيانات تُشبه لاعباً جديداً تماماً، فالتقييم عليها كان سيُسلّح الإرشاد
// لمحاربٍ قديم — وهو الفخّ الذي يحرسه اختبار نفي مخصّص.
//
// ────────────────────────────────────────────────────────────────────────────
//  الطابور يُحتسب إتماماً
// ────────────────────────────────────────────────────────────────────────────
// إن كانت المزرعة **في الطابور** فالخطوة تمّت: اللاعب أدّى الإجراء والبناء
// يأخذ وقته. الإصرار على انتظار اكتمال البناء يعني بطاقةً تُلحّ بطلبٍ نُفِّذ.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rok2Onboarding.generated.h"

class URok2Api;
class UWidget;

/**
 * خطوات الدقيقة الأولى. **الترتيب العددي ذو معنى**: المِزلاج يقارن Ordinal
 * ليمنع الارتداد، فأي إدراج في المنتصف يجب أن يحفظ التصاعد.
 */
UENUM(BlueprintType)
enum class ERok2FtueStep : uint8
{
	/** غير مسلَّح — لم يُقيَّم بعد، أو الحالة لم تصل، أو لاعب قديم */
	None = 0,
	/** ابنِ أول مزرعة */
	BuildFarm = 1,
	/** درّب أول جنود */
	TrainTroops = 2,
	/** أرسل أول مسيرة جمع */
	GatherMarch = 3,
	/** انتهت الدقيقة الأولى — نهائية */
	Done = 4
};

/**
 * تعريف خطوة واحدة: نصّها ورمزها والزر الذي تُبرزه. كل ما تعرضه البطاقة
 * يأتي من هنا — فالودجة لا تحمل نصاً ولا تعرف ترتيب الخطوات.
 */
USTRUCT(BlueprintType)
struct FRok2FtueStepInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	ERok2FtueStep Step = ERok2FtueStep::None;

	/** رقم العرض 1..3 — للافتة «١ من ٣». صفر لغير الخطوات المرشدة. */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	int32 Ordinal = 0;

	/** عنوان أدبي قصير بنَفَس قصصي */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	FString Title;

	/** سطر الحكاية — النَفَس القصصي */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	FString Story;

	/**
	 * الإجراء الملموس. مفصول عن Story عن قصد: البطاقة الأدبية وحدها تُلهم ولا
	 * تُرشد، فاللاعب يقرأ جملةً جميلة ولا يعرف أين يضغط. السطران بدورين
	 * نصّيين مختلفين — الحكاية Body والإجراء Micro.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	FString Action;

	/** معرّف أيقونة إجرائية من URok2ArtAssets — لا إيموجي (قاعدة P6-T1) */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	FString IconId;

	/** الزر المطلوب إبرازه بالحلقة الذهبية — NAME_None إن لا زر له */
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|FTUE")
	FName AnchorId;
};

/**
 * أسماء المراسي ومعايير الإرشاد — مصدر واحد كما في Rok2MotionSpec.
 *
 * المرساة تُسجَّل من الودجة المالكة للزر (HUD/City) ويقرأها الإرشاد ليضع
 * الحلقة الذهبية فوقه. لماذا سجلّ مراسٍ بدل أن تعرف الطبقة أزرار الآخرين:
 * الزر المطلوب يسكن ودجةً أخرى، فبلا سجلّ ينتشر علم الإرشاد في عشر ودجات.
 */
// **extern لا تعريف هنا.** الثوابت غير البدائية (FName/FString) إن عُرِّفت في
// هيدر عام فإن `const` عند نطاق النطاق يعطي ربطاً داخلياً — فتُنشأ نسخة
// مستقلة ومُهيِّئ ساكن في **كل** وحدة ترجمة تُضمّن الهيدر. ومُهيِّئات FName
// الساكنة تحديداً تعتمد على جدول الأسماء في المحرك، وترتيب التهيئة بين وحدات
// الترجمة غير مضمون. فالتعريف الواحد في Rok2Onboarding.cpp، والهيدر يعلن فقط.
namespace Rok2FtueSpec
{
	/** زر البناء الدائري الكبير في الـHUD (أسفل يمين) */
	extern ROK2_API const FName AnchorBuild;

	/** زر التدريب في لوحة المدينة */
	extern ROK2_API const FName AnchorTrain;

	/** زر الخريطة في الـHUD — هدف خطوة المسيرة */
	extern ROK2_API const FName AnchorMap;

	/** عدد الخطوات المرشدة (لا يشمل None/Done) */
	static constexpr int32 GuidedStepCount = 3;

	/** معرّف مبنى المزرعة كما في data/buildings.json */
	extern ROK2_API const FString FarmBuildingId;

	/** نوع طابور البناء كما يخدمه الخادم (FRok2QueueEntry::Type) */
	extern ROK2_API const FString QueueTypeBuilding;

	/** نوع طابور التدريب */
	extern ROK2_API const FString QueueTypeTraining;
}

/**
 * نموذج الإرشاد. نسخة مشتركة (AddToRoot) على اصطلاح URok2Typography
 * وURok2MotionLibrary — لأن المِزلاج حالة جلسة يجب أن تنجو من إعادة بناء
 * الودجات، والودجات تُبنى وتُهدم بحرية في هذا المشروع.
 */
UCLASS(BlueprintType)
class ROK2_API URok2Onboarding : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	static URok2Onboarding* Get();

	// ---------------------------------------------------------------------
	// التعريفات (ثابتة، بلا حالة)
	// ---------------------------------------------------------------------

	/** الترتيب العددي — أساس منع الارتداد في المِزلاج */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static int32 OrdinalOf(ERok2FtueStep Step);

	/** تعريف الخطوة كاملاً (نص/رمز/مرساة) */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static FRok2FtueStepInfo StepInfo(ERok2FtueStep Step);

	/** الخطوات المرشدة الثلاث بترتيبها */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static TArray<ERok2FtueStep> GuidedSteps();

	/** نص التتويج عند إتمام الدقيقة الأولى (theme-and-values §5 Ceremony) */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static FRok2FtueStepInfo CompletionInfo();

	// ---------------------------------------------------------------------
	// المُسنَدات — دوال صافية على حالة الـApi
	// ---------------------------------------------------------------------

	/**
	 * هل وصلت حالة حقيقية يصحّ التقييم عليها؟ مدينة قبل وصول البيانات تُشبه
	 * لاعباً جديداً، والتقييم عليها يُسلّح الإرشاد لمحاربٍ قديم.
	 * الدليل: لاعب موجود + City.UpdatedAt غير صفري (يملؤه ParseCity من
	 * updated_at، فصفره يعني «لم تصل حمولة مدينة بعد»).
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static bool IsStateReady(const URok2Api* Api);

	/** مزرعة مبنية أو في الطابور */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static bool HasFarm(const URok2Api* Api);

	/** جنود موجودون أو تدريب في الطابور */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static bool HasTroops(const URok2Api* Api);

	/**
	 * مسيرة للاعب نفسه. تُلتقط لحظة إنشائها لأن UpsertMarch يبثّ
	 * OnWorldSnapshot — فلا سباق مع إتمام المسيرة داخل الجلسة.
	 */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static bool HasOwnMarch(const URok2Api* Api);

	/** الخطوة الخام من الحالة — بلا مِزلاج، فقد ترتدّ. للفحص والاستنتاج. */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	static ERok2FtueStep DeriveRawStep(const URok2Api* Api);

	// ---------------------------------------------------------------------
	// المِزلاج (حالة الجلسة)
	// ---------------------------------------------------------------------

	/**
	 * يقيّم الخطوة ويطبّق المِزلاج. يُنادى من مفوَّضات الـApi القائمة
	 * (OnCityLoaded / OnWorldSnapshot) — لا Tick.
	 * يعيد الخطوة المعروضة بعد المِزلاج.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	ERok2FtueStep Evaluate(const URok2Api* Api);

	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	ERok2FtueStep GetCurrentStep() const { return CurrentStep; }

	/** هل الإرشاد مسلَّح لهذا اللاعب؟ (لاعب قديم = false فلا تُبنى الطبقة) */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	bool IsArmed() const { return bArmed; }

	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	bool HasEvaluated() const { return bEvaluatedOnce; }

	/** هل هناك خطوة مرشدة تُعرض الآن؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2|FTUE")
	bool IsShowingGuidance() const;

	/** إعادة التسليح — للتطوير والاختبار اليدوي، لا يناديها مسار اللعب */
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	void Reset();

	// ---------------------------------------------------------------------
	// سجلّ المراسي
	// ---------------------------------------------------------------------

	/**
	 * تسجّل زراً كمرساة إبراز. TWeakObjectPtr لأن شبكات الـHUD تُعاد بناؤها
	 * فمؤشر خام كان سيتدلّى.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	void RegisterAnchor(FName AnchorId, UWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	void ClearAnchor(FName AnchorId);

	/** يعيد الودجة إن كانت لا تزال حيّة، وإلا nullptr */
	UWidget* ResolveAnchor(FName AnchorId) const;

	/** مرساة الخطوة المعروضة حالياً (nullptr إن لا إبراز) */
	UWidget* ResolveCurrentAnchor() const;

private:
	/** الخطوة المعروضة بعد المِزلاج */
	ERok2FtueStep CurrentStep = ERok2FtueStep::None;

	/** هل جرى أول تقييم على حالة جاهزة؟ */
	bool bEvaluatedOnce = false;

	/** هل اللاعب مبتدئ فعلاً؟ يُقرَّر مرة واحدة عند أول تقييم جاهز. */
	bool bArmed = false;

	/**
	 * المراسي المسجَّلة. TMap لأن العدد صغير ثابت (3) والقراءة بالاسم.
	 * TWeakObjectPtr يحمي من تدلّي المؤشر عند إعادة بناء الشبكات.
	 */
	TMap<FName, TWeakObjectPtr<UWidget>> Anchors;
};
