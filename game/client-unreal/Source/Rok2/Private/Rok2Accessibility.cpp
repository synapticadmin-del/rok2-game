// Copyright ROK2. مركز قابلية الوصول والتعريب (P7-T7) — implementation.
//
// يتبع AGENTS.md: كل الإعدادات مقروءة من المحرك/الـ CVar ولا تعتمد على أصل غائب،
// والألوان المعتمدة على التباين تحسب من WCAG AA (نسبة تباين ≥ 4.5:1 فوق #1A120B).

#include "Rok2Accessibility.h"
#include "HAL/PlatformApplicationMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Accessibility, Log, All);

namespace Rok2A11y
{
	// خلفيات HUD الداكنة (من Rok2HudStyle PanelBg #1A120B) — مقسومة على النسب
	static constexpr float RelLumDark = 0.0128f;
	// ألوان HUD المصبوغة ونسبة تباينها فوق الخلفية الداكنة (WCAG AA ≥ 4.5:1)
	static constexpr float Luminance(const FLinearColor& C)
	{
		return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B;
	}
	static float ContrastRatio(const FLinearColor& Fg, float BgLum)
	{
		const float L1 = Luminance(Fg);
		const float L2 = BgLum;
		const float Lighter = FMath::Max(L1, L2);
		const float Darker = FMath::Min(L1, L2);
		return (Lighter + 0.05f) / (Darker + 0.05f);
	}
	// لون مقروء: إذا كان اللون المصبوغ ضعيف التباين فنعوض بالإيفوري، وإلا يبقى
	static FLinearColor EnsureReadable(const FLinearColor& Color)
	{
		if (ContrastRatio(Color, RelLumDark) >= 4.5f) return Color;
		return FLinearColor(0.96f, 0.91f, 0.81f, FMath::Max(0.95f, Color.A));
	}
}

URok2Accessibility* URok2Accessibility::Get()
{
	static URok2Accessibility* Instance = nullptr;
	if (!Instance)
	{
		Instance = NewObject<URok2Accessibility>(GetTransientPackage());
		Instance->AddToRoot();
		// المقياس الأساسي يتبع إعدادات المحرك (GSlateApplication::Get().GetApplicationScale)
		Instance->UiScale = FMath::Clamp(
			FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(10.f, 10.f),
			0.85f, 1.6f);
	}
	return Instance;
}

bool URok2Accessibility::IsRtl() const
{
	// العربية RTL افتراضيًا — المحرك العربي يكتب النص من اليمين،
	// والترتيب الهيكلي للودجات يتبع هذا الفالغ في كل HUD/City/Report.
	return true;
}

float URok2Accessibility::GetUiScale() const
{
	return UiScale;
}

float URok2Accessibility::ScaledSize(float BaseSize) const
{
	return FMath::Clamp(BaseSize * UiScale, 12.f, 64.f);
}

float URok2Accessibility::GetScaledPx(float BasePx) const
{
	return BasePx * UiScale;
}

float URok2Accessibility::ScaledIconSize(float BaseSize) const
{
	// الأيقونات لا تصغر عن 18 حتى مع مقياس أقل من 1 — حد مقروءية أدنى
	return FMath::Max(18.f, FMath::RoundToInt(BaseSize * UiScale));
}

bool URok2Accessibility::IsHighContrast() const
{
	return bHighContrast;
}

FLinearColor URok2Accessibility::AccessibleTextFor(FLinearColor TintedColor) const
{
	if (bHighContrast)
	{
		return FLinearColor(0.96f, 0.91f, 0.81f, FMath::Max(0.95f, TintedColor.A));
	}
	return Rok2A11y::EnsureReadable(TintedColor);
}

FLinearColor URok2Accessibility::HighContrastForState(bool bGood, bool bNeutral)
{
	if (bNeutral) return FLinearColor(0.95f, 0.85f, 0.55f, 1.f);  // ذهبي فاتح AA
	if (bGood)     return FLinearColor(0.55f, 0.90f, 0.60f, 1.f);  // أخضر فاتح AA
	return FLinearColor(0.95f, 0.45f, 0.40f, 1.f);                // أحمر فاتح AA
}

FText URok2Accessibility::LabelForIcon(const FString& IconId)
{
	// النص البديل العربي لكل أيقونة إجرائية — يُستخدم في ToolTip وScreen Reader مستقبلًا
	if (IconId == TEXT("food"))          return FText::FromString(TEXT("طعام"));
	if (IconId == TEXT("wood") || IconId == TEXT("wood_log")) return FText::FromString(TEXT("خشب"));
	if (IconId == TEXT("stone"))         return FText::FromString(TEXT("حجر"));
	if (IconId == TEXT("gold"))          return FText::FromString(TEXT("ذهب"));
	if (IconId == TEXT("gems"))          return FText::FromString(TEXT("جواهر"));
	if (IconId == TEXT("ap"))            return FText::FromString(TEXT("نقاط عمل"));
	if (IconId == TEXT("build") || IconId == TEXT("hammer")) return FText::FromString(TEXT("بناء"));
	if (IconId == TEXT("sword"))         return FText::FromString(TEXT("هجوم"));
	if (IconId == TEXT("shield") || IconId == TEXT("alliance")) return FText::FromString(TEXT("تحالف"));
	if (IconId == TEXT("helmet") || IconId == TEXT("commanders")) return FText::FromString(TEXT("قادة"));
	if (IconId == TEXT("bag") || IconId == TEXT("items")) return FText::FromString(TEXT("حقيبة"));
	if (IconId == TEXT("banner") || IconId == TEXT("events")) return FText::FromString(TEXT("أحداث"));
	if (IconId == TEXT("scroll") || IconId == TEXT("reports")) return FText::FromString(TEXT("تقارير"));
	if (IconId == TEXT("map"))           return FText::FromString(TEXT("خريطة"));
	if (IconId == TEXT("edit"))          return FText::FromString(TEXT("تعديل"));
	if (IconId == TEXT("bell"))          return FText::FromString(TEXT("إشعارات"));
	if (IconId == TEXT("lock"))          return FText::FromString(TEXT("مقفل"));
	if (IconId == TEXT("calendar"))      return FText::FromString(TEXT("يوم الموسم"));
	if (IconId == TEXT("hourglass") || IconId == TEXT("queue")) return FText::FromString(TEXT("طابور"));
	if (IconId == TEXT("flask") || IconId == TEXT("research")) return FText::FromString(TEXT("بحث"));
	if (IconId == TEXT("cross") || IconId == TEXT("hospital")) return FText::FromString(TEXT("مستشفى"));
	if (IconId == TEXT("scout"))         return FText::FromString(TEXT("استكشاف"));
	if (IconId == TEXT("close"))         return FText::FromString(TEXT("إغلاق"));
	if (IconId == TEXT("star"))          return FText::FromString(TEXT("نجمة"));
	if (IconId == TEXT("skull"))         return FText::FromString(TEXT("خسارة"));
	if (IconId == TEXT("blood"))         return FText::FromString(TEXT("إصابات"));
	if (IconId == TEXT("bandage"))       return FText::FromString(TEXT("علاج"));
	if (IconId == TEXT("trophy"))        return FText::FromString(TEXT("انتصار"));
	if (IconId == TEXT("handshake"))     return FText::FromString(TEXT("اتفاق"));
	if (IconId == TEXT("refresh"))       return FText::FromString(TEXT("تحديث"));
	if (IconId == TEXT("governor"))      return FText::FromString(TEXT("الحاكم"));
	if (IconId == TEXT("clock"))         return FText::FromString(TEXT("الوقت"));
	if (IconId == TEXT("sparkle"))       return FText::FromString(TEXT("ميزة"));
	if (IconId == TEXT("conn"))          return FText::FromString(TEXT("حالة الاتصال"));
	if (IconId == TEXT("crown"))         return FText::FromString(TEXT("حضارتي"));
	if (IconId == TEXT("speed") || IconId == TEXT("speedup")) return FText::FromString(TEXT("تسريع"));
	if (IconId == TEXT("build_idle"))   return FText::FromString(TEXT("البنّاء خامل — لا طابور بناء نشط"));
	if (IconId == TEXT("heal"))          return FText::FromString(TEXT("علاج"));
	if (IconId == TEXT("train"))         return FText::FromString(TEXT("تدريب"));
	if (IconId == TEXT("flag") || IconId == TEXT("rally")) return FText::FromString(TEXT("رالي التحالف"));
	if (IconId == TEXT("dispatch"))      return FText::FromString(TEXT("إرسال المسيرة"));
	if (IconId == TEXT("redirect"))      return FText::FromString(TEXT("تحويل المسيرة"));
	// P7-T7: مسميات أزرار الدردشة والإشعارات
	if (IconId == TEXT("chat") || IconId == TEXT("chat_kingdom")) return FText::FromString(TEXT("دردشة المملكة"));
	if (IconId == TEXT("chat_alliance")) return FText::FromString(TEXT("دردشة التحالف"));
	if (IconId == TEXT("chat_sender"))   return FText::FromString(TEXT("اسم المرسِل ملوّنٌ حسب حضارته"));
	if (IconId == TEXT("send"))          return FText::FromString(TEXT("إرسال"));
	if (IconId == TEXT("minimize"))      return FText::FromString(TEXT("تصغير"));
	return FText::FromString(TEXT("أيقونة"));
}

void URok2Accessibility::SetUiScale(float NewScale)
{
	UiScale = FMath::Clamp(NewScale, 0.85f, 1.6f);
	OnAccessibilityChanged.Broadcast();
}

void URok2Accessibility::SetHighContrast(bool bEnable)
{
	bHighContrast = bEnable;
	OnAccessibilityChanged.Broadcast();
}
