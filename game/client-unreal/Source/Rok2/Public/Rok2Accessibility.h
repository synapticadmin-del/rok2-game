// Copyright ROK2. مركز قابلية الوصول والتعريب (P7-T7) — header.
//
// URok2Accessibility كائن singleton (CDO) يحتفظ بإعدادات العرض القابلة للضبط:
//   1. **RTL عربي فعلي** — IsRtl() يوجّه ترتيب الودجات الأفقية في كل الشاشات.
//   2. **مقياس واجهة** — UiScale مضروب في كل أحجام الخط والأيقونات.
//   3. **وضع تباين عالٍ** — HighContrast يرفع opacity الخلفيات ويثبت ألوان معتمدة على WCAG AA.
//   4. **مسميات الأيقونات** — LabelForIcon(IconId) يعيد النص البديل العربي لكل أيقونة.
//
// الإعدادات تُقرأ عند الإنشاء من GameUserSettings (UI Scale) وCVars،
// وتُغيَّر وقت التشغيل عبر Setters مع OnAccessibilityChanged.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rok2Accessibility.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRok2AccessibilityChanged);

UCLASS(BlueprintType, Blueprintable, Category = "Rok2|Accessibility")
class ROK2_API URok2Accessibility : public UObject
{
	GENERATED_BODY()
public:
	/** نسخة مشتركة تُثبَّت في الجذر (AddToRoot) عند أول طلب */
	static URok2Accessibility* Get();

	/** هل اتجاه الواجهة يمين-لليسار؟ الافتراضي العربي RTL — يتبع لغة المحرك */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	bool IsRtl() const;

	/** مقياس الواجهة: 1.0 أساسي، حتى 1.5 (مربوط بـ GSlateApplication UI Scale) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	float GetUiScale() const;

	/** حجم خط معدّل بمقياس الواجهة — لكل أدوار URok2Typography */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	float ScaledSize(float BaseSize) const;

	/** حجم بكسل معدّل بمقياس الواجهة — لكل الأبعاد الثابتة (SetSize/SetPosition) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	float GetScaledPx(float BasePx) const;

	/** حجم أيقونة معدّل بمقياس الواجهة — لا تقل عن الحد الأدنى المقروء */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	float ScaledIconSize(float BaseSize) const;

	/** وضع تباين عالٍ: خلفيات أعتق وألوان معيارية عالية التباين */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	bool IsHighContrast() const;

	/** لون نص معتمد على التباين فوق خلفية داكنة (AA) — يستبدل الألوان المصبوغة عند HighContrast */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	FLinearColor AccessibleTextFor(FLinearColor TintedColor) const;

	/** لون نص الحالة (نجاح/خطأ/معلومات) يُقرأ دون الاعتماد على اللون فقط */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	static FLinearColor HighContrastForState(bool bGood, bool bNeutral = false);

	/** النص البديل العربي لأيقونة — لكل أيقونات URok2IconLibrary المعروفة */
	UFUNCTION(BlueprintPure, Category = "Rok2|Accessibility")
	static FText LabelForIcon(const FString& IconId);

	/** تغيير المقياس وقت التشغيل — يبث OnAccessibilityChanged */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Accessibility")
	void SetUiScale(float NewScale);

	/** تبديل التباين العالي وقت التشغيل */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Accessibility")
	void SetHighContrast(bool bEnable);

	/** بث عند أي تغيير في الإعدادات — تستمع إليه الودجات المفتوحة */
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Accessibility")
	FOnRok2AccessibilityChanged OnAccessibilityChanged;

protected:
	UPROPERTY(Transient)
	float UiScale = 1.0f;

	UPROPERTY(Transient)
	bool bHighContrast = false;
};
