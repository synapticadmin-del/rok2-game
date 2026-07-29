// Copyright ROK2. Shared UMG motion library (P6-T3).
//
// مكتبة حركات الواجهة الموحدة — كل الشاشات تستخدمها بدل الحركات المحلية.
// المرجع: 07-game-design/ui-ux-design-system.md §9 «أنيميشن اللوحات عبر UMG
// Animations (Slide/Fade) — معيار موحد 0.25s ease-out» + §8/6 «رد فعل بصري
// فوري (<100ms)» + §1 «كل لوحة تنزلق من أسفل كـ Bottom Sheet · كل تأكيد له
// وميض ذهبي · لا قفزات جامدة».
//
// لماذا محرك توين بدل UMG Animation assets: كل ودجات العميل تُبنى إجرائياً في
// C++ (WidgetTree->ConstructWidget) وليست WBP assets، فلا توجد UWidgetAnimation
// لتشغيلها. المكتبة تحرّك خصائص الرندر نفسها التي تحرّكها UMG Animation
// (RenderTranslation / RenderScale / RenderOpacity / RenderTransformPivot)
// فتعطي نفس النتيجة البصرية مع بقاء الودجات إجرائية.
//
// الاستخدام النموذجي:
//   // لوحة سفلية (Bottom Sheet) عند الفتح
//   URok2MotionLibrary::PlaySlideInBottom(SheetBorder);
//   // نافذة تفتح من المركز
//   URok2MotionLibrary::PlayScaleInCenter(CardBorder);
//   // بطاقة إشعار تنبثق من الأسفل
//   URok2MotionLibrary::PlayToastIn(Card);
//   // ضغطة محسوسة (scale + صوت) — تُربط مرة واحدة عند بناء الزر
//   URok2MotionLibrary::BindPress(Btn, Circle);
//   // وميض ذهبي عند التأكيد
//   URok2MotionLibrary::PlayGoldFlash(Border);

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Containers/Ticker.h"
#include "Rok2MotionLibrary.generated.h"

class UWidget;
class UButton;

/** منحنيات التسهيل (easing) المتاحة. */
UENUM(BlueprintType)
enum class ERok2Ease : uint8
{
	Linear = 0,
	OutCubic,		// الافتراضي — ease-out المعياري من وثيقة UI
	InCubic,
	InOutCubic,
	OutQuad,
	OutBack			// تجاوز طفيف (overshoot) — للنوافذ التي تفتح من المركز
};

/** أنواع الحركات المدعومة. */
UENUM(BlueprintType)
enum class ERok2Motion : uint8
{
	FadeIn = 0,			// تلاشٍ للداخل (انتقال شاشة)
	FadeOut,			// تلاشٍ للخارج
	SlideInBottom,		// انزلاق من الأسفل — Bottom Sheet
	SlideOutBottom,		// انزلاق للأسفل عند الإغلاق
	SlideInRight,		// انزلاق من اليمين (RTL: الدخول الطبيعي)
	SlideInLeft,		// انزلاق من اليسار
	ScaleInCenter,		// نافذة تفتح من المركز (scale + fade)
	ScaleOutCenter,		// نافذة تُغلق نحو المركز
	ToastIn,			// بطاقة إشعار تنبثق من الأسفل (slide + fade)
	ToastOut,			// اختفاء بطاقة الإشعار
	Press,				// ضغطة محسوسة (تصغير سريع)
	Release,			// عودة من الضغطة
	GoldFlash,			// وميض ذهبي للتأكيد
	Pulse				// نبضة انتباه متكررة الاستخدام (شارات/تنبيهات)
};

/**
 * الثوابت المعيارية للحركة — مصدر واحد لكل الشاشات.
 * من ui-ux-design-system.md §9 (0.25s ease-out) و§8.6 (<100ms للمس).
 */
namespace Rok2MotionSpec
{
	/** المعيار الموحد لكل انتقالات اللوحات: 0.25s ease-out. */
	static constexpr float Std = 0.25f;

	/** ردّ فعل اللمس — أقل من 100ms كما تنص قاعدة التجربة الصارمة #6. */
	static constexpr float Fast = 0.09f;

	/** حركات الاختفاء/التلاشي الأطول قليلاً. */
	static constexpr float Slow = 0.40f;

	/** مسافة انزلاق اللوحة السفلية (px). */
	static constexpr float SheetOffset = 300.f;

	/** مسافة انبثاق بطاقة الإشعار من الأسفل (px). */
	static constexpr float ToastOffset = 64.f;

	/** مسافة الانزلاق الأفقي للشاشات (px). */
	static constexpr float SideOffset = 220.f;

	/** مقياس الزر لحظة الضغط (94% — محسوس دون أن يكون مزعجاً). */
	static constexpr float PressScale = 0.94f;

	/** المقياس الابتدائي للنافذة التي تفتح من المركز. */
	static constexpr float ScaleInFrom = 0.86f;

	/** مدة الوميض الذهبي للتأكيد. */
	static constexpr float FlashDuration = 0.45f;
}

// القيم الافتراضية في تصاريح UFUNCTION أدناه مكتوبة حرفياً (0.25f / 0.45f) لأن
// UHT يقرأ القيمة الافتراضية كنص ولا يفهم ثابتاً مُنَمّطاً — فيصير الافتراضي 0
// عند النداء من Blueprint. هذان الحرسان يمنعان انحراف الحرفي عن المعيار بصمت.
static_assert(Rok2MotionSpec::Std == 0.25f,
	"Rok2MotionSpec::Std changed - update the literal UFUNCTION defaults below.");
static_assert(Rok2MotionSpec::FlashDuration == 0.45f,
	"Rok2MotionSpec::FlashDuration changed - update the literal default in PlayGoldFlash.");

/**
 * وكيل ضغطة الزر — يربط OnPressed/OnReleased لزر واحد بحركة scale + صوت.
 * سبب وجوده: UButton::OnPressed دالة ديناميكية تحتاج UFUNCTION هدفاً؛ لا يمكن
 * ربط lambda. فننشئ وكيلاً صغيراً لكل زر تحفظه المكتبة من الـ GC.
 */
UCLASS()
class ROK2_API URok2ButtonPressFx : public UObject
{
	GENERATED_BODY()

public:
	/** الودجة التي تُحرَّك بصرياً (غالباً الـ Border الحاوي للزر). */
	UPROPERTY(Transient)
	TWeakObjectPtr<UWidget> Visual;

	/** هل يُشغَّل صوت النقرة عند الضغط؟ */
	bool bPlaySound = true;

	UFUNCTION()
	void HandlePressed();

	UFUNCTION()
	void HandleReleased();
};

/**
 * مكتبة الحركات المشتركة — نقطة واحدة لكل حركات الواجهة.
 * تعمل عبر توين يُحدّث في الـ core ticker (لا تحتاج NativeTick في كل ودجة).
 */
UCLASS(BlueprintType)
class ROK2_API URok2MotionLibrary : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) — تُسجّل الـ ticker عند أول طلب. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static URok2MotionLibrary* Get();

	/** يلغي تسجيل الـ ticker حتى لا يبقى delegate مربوطاً بكائن مهدوم. */
	virtual void BeginDestroy() override;

	// -----------------------------------------------------------------------
	// واجهة عامة — حركة واحدة على ودجة
	// -----------------------------------------------------------------------

	/**
	 * يشغّل حركة على ودجة. Duration = 0 يعني استخدام المدة المعيارية للحركة.
	 * آمن مع nullptr (لا يفعل شيئاً) — لا يُكسر بناء الواجهة.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void Play(UWidget* Target, ERok2Motion Motion, float Duration = 0.f);

	/** انتقال شاشة: تلاشٍ للداخل (0.25s ease-out). */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayFadeIn(UWidget* Target, float Duration = 0.25f);

	/**
	 * تسريح لوحة: تلاشٍ للخارج ثم إزالة الودجة من أبيها عند الانتهاء.
	 * الطريقة المعيارية لإغلاق أي لوحة — بدل RemoveFromParent المفاجئ الذي تمنعه
	 * القاعدة «لا قفزات جامدة» في ui-ux-design-system.md §1.
	 * تُنادى على الـ UserWidget نفسه فتخرج اللوحة وخلفيتها المعتمة معاً.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayFadeOut(UWidget* Target, float Duration = 0.25f);

	/** لوحة سفلية Bottom Sheet: انزلاق من الأسفل + تلاشٍ. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlaySlideInBottom(UWidget* Target, float Duration = 0.25f);

	/** نافذة تفتح من المركز: scale من 0.86 إلى 1 بتجاوز طفيف + تلاشٍ. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayScaleInCenter(UWidget* Target, float Duration = 0.25f);

	/** بطاقة إشعار تنبثق من الأسفل. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayToastIn(UWidget* Target, float Duration = 0.25f);

	/** اختفاء بطاقة الإشعار (نزول + تلاشٍ). */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayToastOut(UWidget* Target, float Duration = 0.25f);

	/**
	 * وميض ذهبي للتأكيد — على UBorder (BrushColor) أو UImage (ColorAndOpacity).
	 * BaseColor = اللون الأصلي الذي يعود إليه الوميض (لون اللوحة عادةً). نأخذه
	 * صريحاً لأن UBorder::GetBrushColor غير متاح في كل إصدارات UE5.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayGoldFlash(UWidget* Target,
		FLinearColor BaseColor = FLinearColor(0.10f, 0.07f, 0.04f, 0.92f),
		float Duration = 0.45f);

	/** ضغطة محسوسة فورية (تصغير ثم عودة) + صوت نقرة اختياري. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void PlayPress(UWidget* Target, bool bWithSound = true);

	// -----------------------------------------------------------------------
	// ربط الأزرار — الطريقة المفضّلة للضغطة المحسوسة
	// -----------------------------------------------------------------------

	/**
	 * يربط ضغطة محسوسة بزر: تصغير عند OnPressed وعودة عند OnReleased + صوت.
	 * Visual = الودجة التي تُحرَّك (إن كانت nullptr يُحرَّك الزر نفسه).
	 * تُستدعى مرة واحدة عند بناء الزر.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void BindPress(UButton* Button, UWidget* Visual = nullptr, bool bWithSound = true);

	// -----------------------------------------------------------------------
	// إدارة
	// -----------------------------------------------------------------------

	/** يوقف كل حركات ودجة ويعيد خصائص الرندر لأصلها. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Motion")
	static void StopAll(UWidget* Target);

	/** عدد الحركات النشطة الآن (للاختبار والتشخيص). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Motion")
	int32 GetActiveCount() const { return Tweens.Num(); }

	/** المدة المعيارية لنوع حركة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Motion")
	static float DefaultDuration(ERok2Motion Motion);

	/** يطبّق منحنى التسهيل على تقدّم خطي [0..1]. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Motion")
	static float ApplyEase(float T, ERok2Ease Ease);

protected:
	/** حركة واحدة نشطة. */
	struct FRok2Tween
	{
		TWeakObjectPtr<UWidget> Target;
		ERok2Motion Motion = ERok2Motion::FadeIn;
		ERok2Ease Ease = ERok2Ease::OutCubic;
		float Elapsed = 0.f;
		float Duration = Rok2MotionSpec::Std;
		float Offset = 0.f;			// مسافة الانزلاق إن كانت حركة انزلاق
		float FromScale = 1.f;		// مقياس البداية إن كانت حركة scale
		float ToScale = 1.f;		// مقياس النهاية
		bool bRemoveOnFinish = false;	// يُزال من الشجرة عند الانتهاء (حركات الخروج)
		FLinearColor BaseColor = FLinearColor(0.10f, 0.07f, 0.04f, 0.92f);	// لون العودة للوميض
	};

	/** الحركات النشطة. */
	TArray<FRok2Tween> Tweens;

	/** وكلاء ضغطات الأزرار — محفوظون من الـ GC. */
	UPROPERTY(Transient)
	TArray<UObject*> PressProxies;

	/**
	 * المنفّذ الفعلي لكل حركة — يمرّر BaseColor عبر المعامل بدل التعديل على آخر
	 * عنصر في Tweens بعد الإضافة (تعديل «آخر عنصر» يصيب حركة غريبة لو تغيّر مسار
	 * الإضافة يوماً).
	 */
	static void PlayInternal(UWidget* Target, ERok2Motion Motion, float Duration, FLinearColor BaseColor);

	/** يضيف حركة (يستبدل أي حركة سابقة من نفس النوع على نفس الودجة). */
	void AddTween(const FRok2Tween& Tween);

	/** يحدّث كل الحركات — يُستدعى من الـ ticker. */
	bool TickTweens(float DeltaTime);

	/**
	 * يزيل وكلاء الضغطة الذين اختفت ودجاتهم.
	 * ضروري لأن الشبكات تُعاد بناؤها (FillGrid عند تبديل التبويب) فيتراكم وكيل
	 * لكل زر مُنشأ؛ بدون تنظيف تكبر PressProxies بلا حدّ خلال الجلسة الطويلة.
	 */
	void PrunePressProxies();

	/** مؤقّت التنظيف الدوري لوكلاء الضغطة (ثوانٍ). */
	float PruneTimer = 0.f;

	/** يطبّق قيمة حركة على ودجتها عند تقدّم Eased [0..1]. */
	static void ApplyTween(const FRok2Tween& Tween, float Eased);

	/** مقبض الـ ticker. */
	FTSTicker::FDelegateHandle TickerHandle;

	/** يسجّل الـ ticker مرة واحدة. */
	void EnsureTicker();
};
