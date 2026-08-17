// Copyright ROK2. Shared UMG motion library (P6-T3) — implementation.
//
// محرك توين بسيط يُحدّث في الـ core ticker: كل حركة تحمل ودجتها ومدتها ومنحناها،
// وتُطبَّق على خصائص الرندر (Translation/Scale/Opacity/Pivot) كل إطار حتى تنتهي.
// الودجات محفوظة كـ TWeakObjectPtr فاختفاء الودجة (RemoveFromParent) لا يُسبب
// crash — الحركة تُسقط بهدوء عند أول إطار تجدها فيه غير صالحة.

#include "Rok2MotionLibrary.h"
#include "Rok2AudioManager.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Motion, Log, All);

// ---------------------------------------------------------------------------
// لوحة الألوان (وميض التأكيد) من ui-ux-design-system.md §1
// ---------------------------------------------------------------------------
namespace Rok2MotionPalette
{
	static const FLinearColor Gold(0.79f, 0.64f, 0.15f, 1.f);   // #C9A227
}

// ---------------------------------------------------------------------------
// وكيل ضغطة الزر
// ---------------------------------------------------------------------------
void URok2ButtonPressFx::HandlePressed()
{
	if (UWidget* W = Visual.Get())
	{
		URok2MotionLibrary::Play(W, ERok2Motion::Press);
	}
	if (bPlaySound)
	{
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::UiButtonClick);
		}
	}
}

void URok2ButtonPressFx::HandleReleased()
{
	if (UWidget* W = Visual.Get())
	{
		URok2MotionLibrary::Play(W, ERok2Motion::Release);
	}
}

// ---------------------------------------------------------------------------
// Singleton + ticker
// ---------------------------------------------------------------------------
URok2MotionLibrary* URok2MotionLibrary::Get()
{
	static URok2MotionLibrary* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2MotionLibrary>();
		Instance->AddToRoot();
	}
	Instance->EnsureTicker();
	return Instance;
}

void URok2MotionLibrary::EnsureTicker()
{
	if (TickerHandle.IsValid()) return;

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &URok2MotionLibrary::TickTweens), 0.f);
}

void URok2MotionLibrary::BeginDestroy()
{
	// لا نترك delegate مربوطاً بكائن يُهدَم على الـ core ticker
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Tweens.Reset();
	Super::BeginDestroy();
}

// ---------------------------------------------------------------------------
// منحنيات التسهيل
// ---------------------------------------------------------------------------
float URok2MotionLibrary::ApplyEase(float T, ERok2Ease Ease)
{
	T = FMath::Clamp(T, 0.f, 1.f);
	switch (Ease)
	{
	case ERok2Ease::Linear:
		return T;
	case ERok2Ease::OutCubic:
		return 1.f - FMath::Pow(1.f - T, 3.f);
	case ERok2Ease::InCubic:
		return T * T * T;
	case ERok2Ease::InOutCubic:
		return (T < 0.5f)
			? 4.f * T * T * T
			: 1.f - FMath::Pow(-2.f * T + 2.f, 3.f) / 2.f;
	case ERok2Ease::OutQuad:
		return 1.f - (1.f - T) * (1.f - T);
	case ERok2Ease::OutBack:
	{
		// تجاوز طفيف ثم استقرار — يعطي إحساس «فتح» النوافذ
		constexpr float C1 = 1.70158f;
		constexpr float C3 = C1 + 1.f;
		const float X = T - 1.f;
		return 1.f + C3 * X * X * X + C1 * X * X;
	}
	default:
		return T;
	}
}

float URok2MotionLibrary::DefaultDuration(ERok2Motion Motion)
{
	switch (Motion)
	{
	case ERok2Motion::Press:
	case ERok2Motion::Release:
		return Rok2MotionSpec::Fast;
	case ERok2Motion::GoldFlash:
		return Rok2MotionSpec::FlashDuration;
	case ERok2Motion::FadeOut:
	case ERok2Motion::SlideOutBottom:
	case ERok2Motion::ScaleOutCenter:
	case ERok2Motion::ToastOut:
		return Rok2MotionSpec::Std;
	case ERok2Motion::Pulse:
		return Rok2MotionSpec::Slow;
	default:
		return Rok2MotionSpec::Std;
	}
}

// ---------------------------------------------------------------------------
// إضافة الحركات
// ---------------------------------------------------------------------------
void URok2MotionLibrary::AddTween(const FRok2Tween& Tween)
{
	// تُستبدل أي حركة سابقة على نفس الودجة (لا تتراكم حركتان متضاربتان)
	for (int32 i = Tweens.Num() - 1; i >= 0; --i)
	{
		if (!Tweens[i].Target.IsValid() || Tweens[i].Target == Tween.Target)
		{
			Tweens.RemoveAt(i);
		}
	}
	Tweens.Add(Tween);
}

void URok2MotionLibrary::Play(UWidget* Target, ERok2Motion Motion, float Duration)
{
	PlayInternal(Target, Motion, Duration, FLinearColor(0.10f, 0.07f, 0.04f, 0.92f));
}

void URok2MotionLibrary::PlayInternal(UWidget* Target, ERok2Motion Motion, float Duration, FLinearColor BaseColor)
{
	if (!Target) return;

	URok2MotionLibrary* Self = Get();
	if (!Self) return;

	FRok2Tween T;
	T.Target = Target;
	T.Motion = Motion;
	T.Duration = (Duration > 0.f) ? Duration : DefaultDuration(Motion);
	T.BaseColor = BaseColor;

	// المحور في المنتصف مطلوب لكل حركة مقياس ليكون الفتح/الضغط من المركز
	bool bPivotCenter = false;

	switch (Motion)
	{
	case ERok2Motion::FadeIn:
		T.Ease = ERok2Ease::OutCubic;
		break;

	case ERok2Motion::FadeOut:
		T.Ease = ERok2Ease::InCubic;
		T.bRemoveOnFinish = true;
		break;

	case ERok2Motion::SlideInBottom:
		T.Ease = ERok2Ease::OutCubic;
		T.Offset = Rok2MotionSpec::SheetOffset;
		break;

	case ERok2Motion::SlideOutBottom:
		T.Ease = ERok2Ease::InCubic;
		T.Offset = Rok2MotionSpec::SheetOffset;
		T.bRemoveOnFinish = true;
		break;

	case ERok2Motion::SlideInRight:
		T.Ease = ERok2Ease::OutCubic;
		T.Offset = Rok2MotionSpec::SideOffset;
		break;

	case ERok2Motion::SlideInLeft:
		T.Ease = ERok2Ease::OutCubic;
		T.Offset = -Rok2MotionSpec::SideOffset;
		break;

	case ERok2Motion::ScaleInCenter:
		T.Ease = ERok2Ease::OutBack;
		T.FromScale = Rok2MotionSpec::ScaleInFrom;
		T.ToScale = 1.f;
		bPivotCenter = true;
		break;

	case ERok2Motion::ScaleOutCenter:
		T.Ease = ERok2Ease::InCubic;
		T.FromScale = 1.f;
		T.ToScale = Rok2MotionSpec::ScaleInFrom;
		T.bRemoveOnFinish = true;
		bPivotCenter = true;
		break;

	case ERok2Motion::ToastIn:
		T.Ease = ERok2Ease::OutCubic;
		T.Offset = Rok2MotionSpec::ToastOffset;
		break;

	case ERok2Motion::ToastOut:
		T.Ease = ERok2Ease::InCubic;
		T.Offset = Rok2MotionSpec::ToastOffset;
		T.bRemoveOnFinish = true;
		break;

	case ERok2Motion::Press:
		T.Ease = ERok2Ease::OutQuad;
		T.FromScale = 1.f;
		T.ToScale = Rok2MotionSpec::PressScale;
		bPivotCenter = true;
		break;

	case ERok2Motion::Release:
		T.Ease = ERok2Ease::OutBack;
		T.FromScale = Rok2MotionSpec::PressScale;
		T.ToScale = 1.f;
		bPivotCenter = true;
		break;

	case ERok2Motion::GoldFlash:
		T.Ease = ERok2Ease::OutQuad;
		break;

	case ERok2Motion::Pulse:
		T.Ease = ERok2Ease::InOutCubic;
		T.FromScale = 1.f;
		T.ToScale = 1.08f;
		bPivotCenter = true;
		break;

	default:
		break;
	}

	if (bPivotCenter)
	{
		Target->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}

	Self->AddTween(T);

	// إطار البداية يُرسم بنفس دالة التطبيق بدل ضبط «حالة البداية» يدوياً لكل حركة —
	// فمنطق «من أين تبدأ الحركة» موجود في مكان واحد ولا يمكن أن ينحرف عن منطق التحديث.
	ApplyTween(T, ApplyEase(0.f, T.Ease));
}

void URok2MotionLibrary::PlayFadeIn(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::FadeIn, Duration);
}

void URok2MotionLibrary::PlayFadeOut(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::FadeOut, Duration);
}

void URok2MotionLibrary::PlaySlideInBottom(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::SlideInBottom, Duration);
}

void URok2MotionLibrary::PlayScaleInCenter(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::ScaleInCenter, Duration);
}

void URok2MotionLibrary::PlayToastIn(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::ToastIn, Duration);
}

void URok2MotionLibrary::PlayToastOut(UWidget* Target, float Duration)
{
	Play(Target, ERok2Motion::ToastOut, Duration);
}

void URok2MotionLibrary::PlayGoldFlash(UWidget* Target, FLinearColor BaseColor, float Duration)
{
	PlayInternal(Target, ERok2Motion::GoldFlash, Duration, BaseColor);
}

void URok2MotionLibrary::PlayPress(UWidget* Target, bool bWithSound)
{
	Play(Target, ERok2Motion::Press);
	if (bWithSound)
	{
		if (URok2AudioManager* Audio = URok2AudioManager::Get())
		{
			Audio->PlaySfx(ERok2AudioType::UiButtonClick);
		}
	}
}

// ---------------------------------------------------------------------------
// ربط الأزرار
// ---------------------------------------------------------------------------
void URok2MotionLibrary::BindPress(UButton* Button, UWidget* Visual, bool bWithSound)
{
	if (!Button) return;

	URok2MotionLibrary* Self = Get();
	if (!Self) return;

	URok2ButtonPressFx* Fx = NewObject<URok2ButtonPressFx>(Self);
	Fx->Visual = Visual ? Visual : Cast<UWidget>(Button);
	Fx->bPlaySound = bWithSound;

	Button->OnPressed.AddDynamic(Fx, &URok2ButtonPressFx::HandlePressed);
	Button->OnReleased.AddDynamic(Fx, &URok2ButtonPressFx::HandleReleased);

	Self->PressProxies.Add(Fx);
}

// ---------------------------------------------------------------------------
// إيقاف
// ---------------------------------------------------------------------------
void URok2MotionLibrary::StopAll(UWidget* Target)
{
	if (!Target) return;

	URok2MotionLibrary* Self = Get();
	if (!Self) return;

	for (int32 i = Self->Tweens.Num() - 1; i >= 0; --i)
	{
		if (!Self->Tweens[i].Target.IsValid() || Self->Tweens[i].Target == Target)
		{
			Self->Tweens.RemoveAt(i);
		}
	}

	Target->SetRenderTranslation(FVector2D::ZeroVector);
	Target->SetRenderScale(FVector2D(1.f, 1.f));
	Target->SetRenderOpacity(1.f);
}

// ---------------------------------------------------------------------------
// التحديث
// ---------------------------------------------------------------------------
void URok2MotionLibrary::PrunePressProxies()
{
	for (int32 i = PressProxies.Num() - 1; i >= 0; --i)
	{
		URok2ButtonPressFx* Fx = Cast<URok2ButtonPressFx>(PressProxies[i]);
		if (!Fx || !Fx->Visual.IsValid())
		{
			PressProxies.RemoveAt(i);
		}
	}
}

bool URok2MotionLibrary::TickTweens(float DeltaTime)
{
	// تنظيف دوري لوكلاء الضغطة (كل 10 ثوانٍ) — الشبكات تُعاد بناؤها كثيراً
	PruneTimer += DeltaTime;
	if (PruneTimer >= 10.f)
	{
		PruneTimer = 0.f;
		PrunePressProxies();
	}

	for (int32 i = Tweens.Num() - 1; i >= 0; --i)
	{
		FRok2Tween& T = Tweens[i];

		// الودجة اختفت (RemoveFromParent / GC) — نُسقط الحركة بهدوء
		UWidget* W = T.Target.Get();
		if (!W)
		{
			Tweens.RemoveAt(i);
			continue;
		}

		T.Elapsed += DeltaTime;
		const float Linear = (T.Duration > 0.f)
			? FMath::Clamp(T.Elapsed / T.Duration, 0.f, 1.f)
			: 1.f;
		const float Eased = ApplyEase(Linear, T.Ease);

		ApplyTween(T, Eased);

		if (Linear >= 1.f)
		{
			// نُسقط الحركة من المصفوفة قبل RemoveFromParent: الأخيرة قد تُشغّل مساراً
			// يعود فينادي Play/StopAll فيعيد تخصيص Tweens — وحينها كان RemoveAt(i)
			// سيصيب عنصراً خاطئاً أو يتجاوز الحدود.
			const bool bRemoveWidget = T.bRemoveOnFinish;
			Tweens.RemoveAt(i);
			if (bRemoveWidget)
			{
				W->RemoveFromParent();

				// إعادة خصائص الرندر بعد الإزالة. الودجة خارج الشاشة فلا يُرى
				// شيء، لكن هذا يمنع عطلاً حقيقياً: اللوحات التي يملكها GameMode
				// تُنشأ مرة وتُعاد للعرض مراراً (`if (!IsInViewport()) AddToViewport`)،
				// وحركة الخروج تنتهي بشفافية 0 — فالفتح الثاني لقائمة البناء أو
				// الدردشة أو التقارير كان يضيف لوحة شفافة تماماً بلا أي مسار
				// يعيد الشفافية. الإغلاق بزر الرجوع (P18-T5) يمرّ بنفس المسار،
				// فبدون هذا كان الرجوع يُخفي اللوحة إلى الأبد.
				W->SetRenderOpacity(1.f);
				W->SetRenderTranslation(FVector2D::ZeroVector);
				W->SetRenderScale(FVector2D(1.f, 1.f));
			}
		}
	}

	return true;	// يبقى الـ ticker مسجّلاً
}

void URok2MotionLibrary::ApplyTween(const FRok2Tween& Tween, float Eased)
{
	UWidget* W = Tween.Target.Get();
	if (!W) return;

	switch (Tween.Motion)
	{
	case ERok2Motion::FadeIn:
		W->SetRenderOpacity(Eased);
		break;

	case ERok2Motion::FadeOut:
		W->SetRenderOpacity(1.f - Eased);
		break;

	case ERok2Motion::SlideInBottom:
		W->SetRenderTranslation(FVector2D(0.f, (1.f - Eased) * Tween.Offset));
		break;

	case ERok2Motion::SlideOutBottom:
		W->SetRenderTranslation(FVector2D(0.f, Eased * Tween.Offset));
		W->SetRenderOpacity(1.f - Eased);
		break;

	case ERok2Motion::SlideInRight:
	case ERok2Motion::SlideInLeft:
		W->SetRenderTranslation(FVector2D((1.f - Eased) * Tween.Offset, 0.f));
		break;

	case ERok2Motion::ScaleInCenter:
	case ERok2Motion::ScaleOutCenter:
	{
		const float S = FMath::Lerp(Tween.FromScale, Tween.ToScale, Eased);
		W->SetRenderScale(FVector2D(S, S));
		// التلاشي أسرع من المقياس ليبدو الفتح نظيفاً
		const float Op = (Tween.Motion == ERok2Motion::ScaleInCenter)
			? FMath::Clamp(Eased * 1.6f, 0.f, 1.f)
			: 1.f - Eased;
		W->SetRenderOpacity(Op);
		break;
	}

	case ERok2Motion::ToastIn:
		W->SetRenderTranslation(FVector2D(0.f, (1.f - Eased) * Tween.Offset));
		W->SetRenderOpacity(Eased);
		break;

	case ERok2Motion::ToastOut:
		W->SetRenderTranslation(FVector2D(0.f, Eased * Tween.Offset));
		W->SetRenderOpacity(1.f - Eased);
		break;

	case ERok2Motion::Press:
	case ERok2Motion::Release:
	{
		const float S = FMath::Lerp(Tween.FromScale, Tween.ToScale, Eased);
		W->SetRenderScale(FVector2D(S, S));
		break;
	}

	case ERok2Motion::Pulse:
	{
		// ذهاب وعودة داخل نفس المدة (نبضة كاملة)
		const float Tri = 1.f - FMath::Abs(Eased * 2.f - 1.f);
		const float S = FMath::Lerp(Tween.FromScale, Tween.ToScale, Tri);
		W->SetRenderScale(FVector2D(S, S));
		break;
	}

	case ERok2Motion::GoldFlash:
	{
		// وميض ذهبي يخبو: يصعد سريعاً ثم يعود للون الأصلي (BaseColor)
		const float Tri = 1.f - FMath::Abs(Eased * 2.f - 1.f);
		if (UBorder* B = Cast<UBorder>(W))
		{
			// lerp خطّي (لا HSV) ليبقى المسار متوقّعاً من البرونز الداكن للذهب
			B->SetBrushColor(FMath::Lerp(Tween.BaseColor, Rok2MotionPalette::Gold, Tri * 0.7f));
		}
		else if (UImage* Img = Cast<UImage>(W))
		{
			Img->SetColorAndOpacity(FMath::Lerp(FLinearColor::White, Rok2MotionPalette::Gold, Tri));
		}
		else
		{
			// أي ودجة أخرى: نبضة شفافية خفيفة بدل اللون
			W->SetRenderOpacity(1.f - Tri * 0.35f);
		}
		break;
	}

	default:
		break;
	}
}
