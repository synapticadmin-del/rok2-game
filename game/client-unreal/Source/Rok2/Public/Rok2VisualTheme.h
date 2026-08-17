#pragma once

#include "CoreMinimal.h"

/**
 * رموز ألوان ROK2 المشتركة. تستعملها واجهات UMG/Slate ولا تحمل منطق لعبة.
 *
 * المصدر التصميمي: design/01-visual/VISUAL_IDENTITY.md.
 * لا تستعمل لوناً وحده للدلالة على حالة؛ اجمعه دائماً مع رمز ونص وحالة تفاعل.
 *
 * ── طبقتان للألوان ─────────────────────────────────────────────────────────
 * لون الحشو (fill) ولون النص (text) ليسا واحداً على خلفية داكنة. الذهب الذي
 * يصلح لحشو زر (#C9A227) يهبط تحت 4.5:1 كنص فوق لوح داكن. لذلك لكل لون حالة
 * نسختان: `Gold()` للحشو و`GoldText()` للنص. كانت الودجات تحلّ هذا بأن تعرّف
 * لوحة محلية مفتّحة (Rok2SeasonStory::Gold مثلاً) — فتوالدت ست لوحات متوازية.
 */
namespace Rok2Visual
{
	// ── أسطح ومساحات ────────────────────────────────────────────────────────
	ROK2_API const FLinearColor& Ink();
	ROK2_API const FLinearColor& Panel();
	ROK2_API const FLinearColor& Card();
	/** شريط الحالة العلوي — أعتم من اللوح ليفصل الطبقة الثابتة عن العالم. */
	ROK2_API const FLinearColor& Bar();
	/** حافة ذهبية رقيقة حول الألواح والبطاقات. */
	ROK2_API const FLinearColor& Edge();
	ROK2_API const FLinearColor& Scrim();

	// ── حشو ──────────────────────────────────────────────────────────────────
	ROK2_API const FLinearColor& Gold();
	ROK2_API const FLinearColor& Ivory();
	ROK2_API const FLinearColor& Muted();
	ROK2_API const FLinearColor& TabInactive();
	ROK2_API const FLinearColor& PrimaryAction();
	ROK2_API const FLinearColor& Success();
	ROK2_API const FLinearColor& Danger();
	ROK2_API const FLinearColor& Information();

	// ── نص فوق خلفية داكنة (نسخ مفتّحة تحقق ≥4.5:1) ──────────────────────────
	ROK2_API const FLinearColor& GoldText();
	ROK2_API const FLinearColor& SuccessText();
	ROK2_API const FLinearColor& DangerText();
	ROK2_API const FLinearColor& InformationText();

	// ── تلوين الموارد في الشريط العلوي ───────────────────────────────────────
	ROK2_API const FLinearColor& ResourceFood();
	ROK2_API const FLinearColor& ResourceWood();
	ROK2_API const FLinearColor& ResourceStone();
	ROK2_API const FLinearColor& ResourceGold();
	ROK2_API const FLinearColor& ResourceGems();
	ROK2_API const FLinearColor& ResourceActionPoints();

	/** لون نُدرة (قائد/معدّة/منحوتة): 1=متقدم 2=نخبة 3=ملحمي 4=أسطوري. */
	ROK2_API FLinearColor RarityTier(int32 Tier);

	/**
	 * صبغة تعتيم لصورة فنية يعلوها نص أو محتوى (P24-T5).
	 *
	 * الصور الكبيرة — لوحات Splash وفصول الحكاية وجلود القادة — تُقرأ خلفياتٍ
	 * لا مواضيعَ، فتُعتَّم كي يبقى النص فوقها على تباين AA. الدرجات الثلاث
	 * تعكس كثافة ما يعلو الصورة، لا رقماً مخترعاً في كل ودجة:
	 *   1 = محتوى خفيف (بطاقة واحدة فوق Splash)
	 *   2 = محتوى متوسط (بورتريه ونص فوق جلد)
	 *   3 = نص كثيف (خط زمني كامل فوق لوحة فصل)
	 */
	ROK2_API FLinearColor ArtVeil(int32 ContentDensity);

	ROK2_API FLinearColor CivilizationAccent(const FString& CivilizationId);
}

/**
 * سلم المسافات على شبكة 8pt. الأرقام مقيسة من الاستخدام الفعلي: 227 موضع
 * `FMargin` كانت تستعمل 0/2/3/4/5/6/8/10/12/14/16/18/20/30 — الدرجات أدناه
 * تستوعبها كلها، والقيم الشاذة (3، 5، 10، 14، 18، 30) تُقرَّب إلى أقرب درجة.
 */
namespace Rok2Space
{
	static constexpr float None   = 0.f;
	/** خط شعري — حدود وفواصل لا مسافات. */
	static constexpr float Hair   = 2.f;
	static constexpr float XS     = 4.f;
	static constexpr float S      = 8.f;
	static constexpr float M      = 12.f;
	static constexpr float L      = 16.f;
	static constexpr float XL     = 20.f;
	static constexpr float XXL    = 24.f;
	static constexpr float Huge   = 32.f;
}

/** أنصاف أقطار الزوايا. الحد الأقصى يعطي شكل حبّة (pill) على أي ارتفاع. */
namespace Rok2Radius
{
	static constexpr float None  = 0.f;
	static constexpr float Card  = 8.f;
	static constexpr float Panel = 12.f;
	static constexpr float Sheet = 16.f;
	/** أكبر من أي ارتفاع واجهة — يجعل الشكل حبّة أو دائرة. */
	static constexpr float Full  = 4096.f;
}
