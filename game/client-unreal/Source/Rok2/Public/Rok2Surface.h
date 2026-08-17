// Copyright ROK2. مصنع أسطح وأنماط أزرار الواجهة (P17-T1) — header.
//
// المشكلة: كل «حبّة» و«دائرة» و«بطاقة» في اللعبة كانت UBorder مربّعاً بفرشاة
// لون مسطّح — لا زوايا مستديرة ولا حافة، لأن `FSlateRoundedBoxBrush` لم تُستعمل
// ولا مرة في الوحدة. وكل زر كان يكتب حالاته بنفسه: 12 زراً تضبط Normal وحدها
// فلا رد فعل للمس، وثلاثة lambdas في الـHUD تكرّر نفس الثلاثيّة حرفياً.
//
// هذا الملف يجعل الشكل قراراً واحداً:
//     Border->SetBrush(Rok2Surface::Panel());
//     Btn->SetStyle(Rok2Surface::PrimaryButton());
//
// الأشكال من design/01-visual/VISUAL_IDENTITY.md §4: لوح بحافة ذهبية، بطاقة
// أدكن، حبّة كاملة الاستدارة، وزر أساسي بحشو ذهبي.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

/**
 * فرشاة/نمط جاهز لكل سطح في الواجهة. كل الدوال ترجع بالقيمة: الفرشاة بنية
 * صغيرة تُنسخ داخل الودجت، فلا حاجة لعمر مشترك ولا لـ singleton.
 */
namespace Rok2Surface
{
	// ── أسطح ────────────────────────────────────────────────────────────────

	/** لوح رئيسي: خلفية دافئة داكنة + حافة ذهبية رقيقة + زوايا 12. */
	ROK2_API FSlateBrush Panel();

	/** ورقة سفلية (Bottom Sheet): نفس اللوح بزوايا أوسع 16. */
	ROK2_API FSlateBrush Sheet();

	/** بطاقة داخل قائمة: أدكن من اللوح، حافة أخف، زوايا 8. */
	ROK2_API FSlateBrush Card();

	/** بطاقة مبرَزة بلون (نُدرة/حضارة/حالة) — الحافة تحمل اللون لا الحشو. */
	ROK2_API FSlateBrush AccentCard(const FLinearColor& Accent);

	/** شريط الحالة العلوي: بلا زوايا (يمتد بعرض الشاشة) + حافة سفلية ذهبية. */
	ROK2_API FSlateBrush TopBar();

	/** حبّة: استدارة كاملة. للأزرار الصغيرة والوسوم وشارات العدّ. */
	ROK2_API FSlateBrush Pill(const FLinearColor& Fill);

	/** حبّة بحافة ملوّنة — للوسم الذي يدلّ على حالة. */
	ROK2_API FSlateBrush OutlinedPill(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth = 1.5f);

	/** دائرة: نفس الحبّة، تُسمّى صريحاً لأن نية الاستخدام مختلفة (زر دائري). */
	ROK2_API FSlateBrush Circle(const FLinearColor& Fill);

	/** حجاب معتم يغلق ما تحته — خلف كل لوحة modal. */
	ROK2_API FSlateBrush Scrim();

	/** مقبض السحب أعلى الورقة السفلية. */
	ROK2_API FSlateBrush SheetHandle();

	/** مسار شريط تقدّم (الخلفية) وحشوه. */
	ROK2_API FSlateBrush ProgressTrack();
	ROK2_API FSlateBrush ProgressFill(const FLinearColor& Fill);

	// ── الزخرفة فوق السطح (P24-T3) ───────────────────────────────────────────
	//
	// الأسطح أعلاه صارت **نسيجية**: `Panel/Sheet/Card/TopBar/Pill` تعيد فرشاة
	// 9-slice من Content/Art/UISurfaces إن استُوردت، وتسقط إلى نفس اللون
	// المسطّح إن غابت. لم تتغيّر تواقيعها فلا تحتاج الودجات الـ36 تعديلاً —
	// النسيج قرار مركزي واحد كما كان اللون.
	//
	// أما ما يلي فطبقة **إضافية** لا بديلة: تُركَّب فوق اللوح داخل UOverlay.

	/**
	 * إطار ذهبي مزخرف شفّاف الوسط. يُركَّب فوق اللوح لا كخلفيته، فيبقى المحتوى
	 * مقروءاً تحته. يعيد فرشاة بلا مورد إن لم يُستورد الأصل — فلا يرسم شيئاً
	 * بدل أن يحجب اللوحة بمستطيل.
	 */
	ROK2_API FSlateBrush OrnateFrame();

	/** فاصل أفقي ذهبي بمعيّنات — بين أقسام اللوحة الواحدة. */
	ROK2_API FSlateBrush GoldDivider();

	/** هل نسيج الأسطح مستورد؟ للتشخيص والاختبار البنيوي. */
	ROK2_API bool HasSurfaceTextures();

	// ── أنماط أزرار ──────────────────────────────────────────────────────────
	//
	// كل نمط يغطي الحالات الأربع. الحالة المعطّلة ليست ترفاً: زر ترقية لا يملك
	// اللاعب تكلفتها كان يبدو كزر جاهز تماماً.

	/** الزر الأساسي: حشو ذهبي، للفعل الرئيسي في اللوحة (واحد فقط لكل لوحة). */
	ROK2_API FButtonStyle PrimaryButton();

	/** الزر الثانوي: بطاقة داكنة بحافة ذهبية. */
	ROK2_API FButtonStyle SecondaryButton();

	/** زر خَطِر: هجوم، طرد، حذف. */
	ROK2_API FButtonStyle DangerButton();

	/** زر إيجابي: تسريع، شفاء، جمع. */
	ROK2_API FButtonStyle SuccessButton();

	/** زر شفّاف فوق سطح ملوّن مسبقاً (الحلقات الدائرية في الـHUD).
	 *  يبقى الحشو للسطح تحته، ويضيف الزر تظليل تحويم/ضغط فقط. */
	ROK2_API FButtonStyle GhostButton();

	/** تبويب: حالتان بصريتان — نشط بحشو ذهبي خفيف، خامل شفّاف. */
	ROK2_API FButtonStyle TabButton(bool bActive);

	/** زر بلون مخصّص (حضارة/نُدرة) بنفس منطق الحالات الأربع. */
	ROK2_API FButtonStyle TintedButton(const FLinearColor& Fill);

	/** نمط زر من جلد نسيجي مستورد (9-slice)، بحالاته الأربع.
	 *  مضاعفات الحالة (تفتيح التحويم، تعتيم الضغط، خفوت التعطيل) مركزية هنا
	 *  بدل أن تُكتب أرقاماً في كل ملف يستعمل جلداً. */
	ROK2_API FButtonStyle TexturedSkinButton(UObject* SkinTexture, const FVector2D& SkinSize = FVector2D(128.f, 48.f));
}
