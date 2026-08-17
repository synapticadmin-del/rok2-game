// Copyright ROK2. مصنع أسطح وأنماط أزرار الواجهة (P17-T1) — implementation.

#include "Rok2Surface.h"
#include "Rok2VisualTheme.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"

namespace
{
	/** تعتيم/تفتيح لون بالإبقاء على الشفافية — لحالات التحويم والضغط. */
	FLinearColor Shift(const FLinearColor& C, float Factor)
	{
		return FLinearColor(
			FMath::Clamp(C.R * Factor, 0.f, 1.f),
			FMath::Clamp(C.G * Factor, 0.f, 1.f),
			FMath::Clamp(C.B * Factor, 0.f, 1.f),
			C.A);
	}

	/** اللون المعطّل: يفقد التشبّع ويخفت — يُقرأ كـ«غير متاح» بلا نص إضافي. */
	FLinearColor Disabled(const FLinearColor& C)
	{
		const float Grey = 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B;
		return FLinearColor(
			FMath::Lerp(C.R, Grey, 0.75f) * 0.55f,
			FMath::Lerp(C.G, Grey, 0.75f) * 0.55f,
			FMath::Lerp(C.B, Grey, 0.75f) * 0.55f,
			C.A * 0.7f);
	}

	/** يبني نمط زر من فرشاة أساس + دالة تحويل لكل حالة. */
	FButtonStyle MakeFilledStyle(const FLinearColor& Fill, float Radius, const FLinearColor& Outline, float OutlineWidth)
	{
		FButtonStyle Style;
		Style.SetNormal(FSlateRoundedBoxBrush(Fill, Radius, Outline, OutlineWidth));
		Style.SetHovered(FSlateRoundedBoxBrush(Shift(Fill, 1.22f), Radius, Rok2Visual::GoldText(), OutlineWidth));
		Style.SetPressed(FSlateRoundedBoxBrush(Shift(Fill, 0.78f), Radius, Outline, OutlineWidth));
		Style.SetDisabled(FSlateRoundedBoxBrush(Disabled(Fill), Radius, Disabled(Outline), OutlineWidth));

		// الضغط يزيح المحتوى قليلاً للأسفل — إحساس زر حقيقي بلا حركة برمجية.
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, -1.f));
		return Style;
	}

	// ── نسيج الأسطح ──────────────────────────────────────────────────────────

	/** يحمّل نسيج سطح ويخبّئه، بما فيه الفشل — فلا تكرار تحميل كل إطار. */
	UTexture2D* LoadSurfaceTexture(const TCHAR* AssetName)
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Cache;
		static TSet<FString> Misses;

		const FString Key(AssetName);
		if (Misses.Contains(Key))
		{
			return nullptr;
		}
		if (TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Key))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
			Cache.Remove(Key);
		}

		const FString Path = FString::Printf(TEXT("/Game/Art/UISurfaces/%s.%s"), AssetName, AssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (!Texture)
		{
			Misses.Add(Key);
			return nullptr;
		}

		// نسيج الواجهة يُقصّ 9-slice ويتمدد، فأي تكرار أو mip يُظهر خطوطاً على
		// الحدود؛ التثبيت هنا يوفّر ضبطاً يدوياً لكل أصل في المحرر.
		Texture->AddToRoot();
		Cache.Add(Key, Texture);
		return Texture;
	}

	/**
	 * فرشاة 9-slice من نسيج، أو الفرشاة المسطّحة الاحتياطية عند غيابه.
	 * الهامش 0.25 يطابق ما رُسمت عليه الأصول: الزخرفة في الربع الخارجي
	 * فتبقى بحجمها، والوسط وحده يتمدد.
	 */
	FSlateBrush TexturedBox(const TCHAR* AssetName, const FVector2D& NativeSize, const FSlateBrush& Fallback, float MarginFraction = 0.25f)
	{
		UTexture2D* Texture = LoadSurfaceTexture(AssetName);
		if (!Texture)
		{
			return Fallback;
		}

		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(MarginFraction);
		Brush.ImageSize = NativeSize;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		return Brush;
	}

	/** يحمّل جلد زر من Content/Art/UIButtons ويخبّئه (بما فيه الفشل). */
	UTexture2D* LoadButtonSkin(const TCHAR* SkinId)
	{
		static TMap<FString, TWeakObjectPtr<UTexture2D>> Cache;
		static TSet<FString> Misses;

		const FString Key(SkinId);
		if (Misses.Contains(Key))
		{
			return nullptr;
		}
		if (TWeakObjectPtr<UTexture2D>* Found = Cache.Find(Key))
		{
			if (Found->IsValid())
			{
				return Found->Get();
			}
			Cache.Remove(Key);
		}

		const FString Path = FString::Printf(TEXT("/Game/Art/UIButtons/%s.%s"), SkinId, SkinId);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (!Texture)
		{
			Misses.Add(Key);
			return nullptr;
		}
		Texture->AddToRoot();
		Cache.Add(Key, Texture);
		return Texture;
	}

	/**
	 * نمط زر من جلد مستورد، أو النمط الملوّن الاحتياطي عند غيابه.
	 *
	 * الجلود الأربعة في Content/Art/UIButtons كان مستهلكها الوحيد
	 * `URok2CityWidget` داخل ألواح مطوية لا تُعرض — أي أن كل زر في اللعبة كان
	 * مستطيلاً ملوّناً. مكانها هنا: كل زر أساسي/ثانوي/خَطِر/ناجح يكسبها معاً.
	 */
	FButtonStyle SkinnedStyle(const TCHAR* SkinId, const FButtonStyle& Fallback)
	{
		UTexture2D* Skin = LoadButtonSkin(SkinId);
		if (!Skin)
		{
			return Fallback;
		}
		return Rok2Surface::TexturedSkinButton(Skin);
	}
}

// ---------------------------------------------------------------------------
// أسطح
//
// كل سطح نسيجي أولاً ومسطّح احتياطاً: `TexturedBox` يعيد فرشاة 9-slice من
// Content/Art/UISurfaces إن استُورد الأصل، وإلا فالفرشاة المستديرة الملوّنة
// كما كانت. التواقيع لم تتغيّر، فالودجات الـ36 التي تستدعي Panel/Sheet/Card
// كسبت النسيج بلا تعديل سطر فيها — والنسيج يبقى قراراً مركزياً كما اللون.
//
// الأحجام الأصلية هي أبعاد الـPNG في scripts/generate_ui_surface_assets.py؛
// `ImageSize` لا يقيّد حجم اللوح لكنه يحدد نسبة الهامش الثابت، فقيمة خاطئة
// تُشوّه الزخرفة.
// ---------------------------------------------------------------------------

FSlateBrush Rok2Surface::Panel()
{
	return TexturedBox(TEXT("panel_parchment"), FVector2D(256.f, 256.f),
		FSlateRoundedBoxBrush(Rok2Visual::Panel(), Rok2Radius::Panel, Rok2Visual::Edge(), 1.5f));
}

FSlateBrush Rok2Surface::Sheet()
{
	return TexturedBox(TEXT("panel_leather"), FVector2D(256.f, 256.f),
		FSlateRoundedBoxBrush(Rok2Visual::Panel(), Rok2Radius::Sheet, Rok2Visual::Edge(), 1.5f));
}

FSlateBrush Rok2Surface::Card()
{
	const FLinearColor SoftEdge(Rok2Visual::Edge().R, Rok2Visual::Edge().G, Rok2Visual::Edge().B, 0.28f);
	return TexturedBox(TEXT("card_stone"), FVector2D(128.f, 128.f),
		FSlateRoundedBoxBrush(Rok2Visual::Card(), Rok2Radius::Card, SoftEdge, 1.f));
}

FSlateBrush Rok2Surface::AccentCard(const FLinearColor& Accent)
{
	// البطاقة المبرَزة تحمل لون النُدرة/الحضارة في حافتها، وذلك مستحيل على
	// فرشاة نسيجية (اللون داخل الصورة). فتبقى مسطّحة عن قصد: التمييز اللوني
	// هو وظيفتها، والنسيج سيطمسه.
	return FSlateRoundedBoxBrush(Rok2Visual::Card(), Rok2Radius::Card, Accent, 2.f);
}

FSlateBrush Rok2Surface::TopBar()
{
	// الشريط يلتصق بأعلى الشاشة، فاستدارة أعلاه ستكشف العالم خلفه. الحافة
	// الذهبية وحدها تفصله عن المشهد.
	//
	// الهامش 0.16 لا 0.25: الأصل 96px ارتفاعاً، فهامش الربع يثبّت 24px أعلى
	// و24px أسفل ويسحق الوسط على شريط 48px.
	return TexturedBox(TEXT("bar_wood"), FVector2D(256.f, 96.f),
		FSlateRoundedBoxBrush(Rok2Visual::Bar(), Rok2Radius::None, Rok2Visual::Edge(), 1.5f), 0.16f);
}

FSlateBrush Rok2Surface::Pill(const FLinearColor& Fill)
{
	// الحبّة الملوّنة صريحاً (شارة عدّ، وسم حالة) تبقى لونها؛ الحبّة المحايدة
	// التي تأخذ لون السطح هي وحدها من تكسب نسيج البرونز — وإلا لَطُمس أحمر
	// التنبيه أو أخضر النجاح تحت نسيج واحد.
	const bool bNeutral = Fill.Equals(Rok2Visual::Card(), 0.02f) || Fill.Equals(Rok2Visual::Panel(), 0.02f);
	FSlateBrush Flat = FSlateRoundedBoxBrush(Fill, Rok2Radius::Full);
	if (!bNeutral)
	{
		return Flat;
	}
	// نصف قطر الحبّة 31 من 64 ارتفاعاً — هامش 0.45 يحفظ الاستدارة كاملة.
	return TexturedBox(TEXT("pill_bronze"), FVector2D(128.f, 64.f), Flat, 0.45f);
}

FSlateBrush Rok2Surface::OutlinedPill(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth)
{
	return FSlateRoundedBoxBrush(Fill, Rok2Radius::Full, Outline, OutlineWidth);
}

FSlateBrush Rok2Surface::Circle(const FLinearColor& Fill)
{
	return FSlateRoundedBoxBrush(Fill, Rok2Radius::Full, Rok2Visual::Edge(), 2.f);
}

FSlateBrush Rok2Surface::Scrim()
{
	return FSlateRoundedBoxBrush(Rok2Visual::Scrim(), Rok2Radius::None);
}

FSlateBrush Rok2Surface::SheetHandle()
{
	return FSlateRoundedBoxBrush(Rok2Visual::Muted(), Rok2Radius::Full);
}

FSlateBrush Rok2Surface::ProgressTrack()
{
	return FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.05f, 0.03f, 0.9f), Rok2Radius::Full);
}

FSlateBrush Rok2Surface::ProgressFill(const FLinearColor& Fill)
{
	return FSlateRoundedBoxBrush(Fill, Rok2Radius::Full);
}

// ---------------------------------------------------------------------------
// الزخرفة فوق السطح (P24-T3)
// ---------------------------------------------------------------------------

FSlateBrush Rok2Surface::OrnateFrame()
{
	// وسط الإطار شفّاف، فلا فرشاة مسطّحة تصلح احتياطاً: عند غياب الأصل نعيد
	// فرشاة بلا مورد وصبغة شفافة — لا ترسم شيئاً بدل أن تحجب اللوحة بمستطيل.
	FSlateBrush Empty;
	Empty.TintColor = FSlateColor(FLinearColor::Transparent);
	return TexturedBox(TEXT("frame_ornate"), FVector2D(256.f, 256.f), Empty, 0.28f);
}

FSlateBrush Rok2Surface::GoldDivider()
{
	FSlateBrush Fallback = FSlateRoundedBoxBrush(Rok2Visual::Edge(), Rok2Radius::None);
	UTexture2D* Texture = LoadSurfaceTexture(TEXT("divider_gold"));
	if (!Texture)
	{
		return Fallback;
	}
	// الفاصل يتمدد أفقياً وحده، والمعيّنات في وسطه لا في زواياه — هامش رأسي
	// معدوم يمدّه بلا سحق المعيّنات.
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.Margin = FMargin(0.12f, 0.f, 0.12f, 0.f);
	Brush.ImageSize = FVector2D(128.f, 12.f);
	Brush.TintColor = FSlateColor(FLinearColor::White);
	return Brush;
}

bool Rok2Surface::HasSurfaceTextures()
{
	return LoadSurfaceTexture(TEXT("panel_parchment")) != nullptr;
}

// ---------------------------------------------------------------------------
// أنماط الأزرار
//
// كل نمط جلدٌ مستورد أولاً ولونٌ مسطّح احتياطاً. الجلود الأربعة في
// Content/Art/UIButtons كان مستهلكها الوحيد `URok2CityWidget` داخل ألواح مطوية
// لا تُعرض — أي أن كل زر في اللعبة كان مستطيلاً ملوّناً بينما الجلود على القرص.
// ---------------------------------------------------------------------------

FButtonStyle Rok2Surface::PrimaryButton()
{
	return SkinnedStyle(TEXT("button_primary_gold"),
		MakeFilledStyle(Rok2Visual::PrimaryAction(), Rok2Radius::Card, Rok2Visual::Gold(), 1.5f));
}

FButtonStyle Rok2Surface::SecondaryButton()
{
	return SkinnedStyle(TEXT("button_secondary_blue"),
		MakeFilledStyle(Rok2Visual::Card(), Rok2Radius::Card, Rok2Visual::Edge(), 1.5f));
}

FButtonStyle Rok2Surface::DangerButton()
{
	return SkinnedStyle(TEXT("button_danger_red"),
		MakeFilledStyle(Rok2Visual::Danger(), Rok2Radius::Card, Rok2Visual::DangerText(), 1.5f));
}

FButtonStyle Rok2Surface::SuccessButton()
{
	return SkinnedStyle(TEXT("button_success_green"),
		MakeFilledStyle(Rok2Visual::Success(), Rok2Radius::Card, Rok2Visual::SuccessText(), 1.5f));
}

FButtonStyle Rok2Surface::GhostButton()
{
	// السطح تحت الزر يحمل اللون؛ الزر يضيف طبقة تحويم/ضغط فقط. هذا يستبدل
	// الثلاثيّة المكرّرة حرفياً ثلاث مرات في الـHUD، ويضيف حالة معطّلة كانت غائبة.
	FButtonStyle Style;
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.f), Rok2Radius::Full));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, 1.f, 0.10f), Rok2Radius::Full));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.22f), Rok2Radius::Full));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.45f), Rok2Radius::Full));
	Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, -1.f));
	return Style;
}

FButtonStyle Rok2Surface::TabButton(bool bActive)
{
	if (bActive)
	{
		const FLinearColor ActiveFill(Rok2Visual::PrimaryAction().R, Rok2Visual::PrimaryAction().G, Rok2Visual::PrimaryAction().B, 0.85f);
		return MakeFilledStyle(ActiveFill, Rok2Radius::Card, Rok2Visual::Gold(), 1.5f);
	}

	const FLinearColor IdleFill(0.f, 0.f, 0.f, 0.f);
	FButtonStyle Style = MakeFilledStyle(IdleFill, Rok2Radius::Card, Rok2Visual::TabInactive(), 1.f);
	// التبويب الخامل شفّاف، فالتحويم يحتاج حشواً محسوساً لا تفتيح شفافية.
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, 1.f, 0.08f), Rok2Radius::Card, Rok2Visual::Gold(), 1.f));
	return Style;
}

FButtonStyle Rok2Surface::TintedButton(const FLinearColor& Fill)
{
	return MakeFilledStyle(Fill, Rok2Radius::Card, Rok2Visual::Edge(), 1.5f);
}

FButtonStyle Rok2Surface::TexturedSkinButton(UObject* SkinTexture, const FVector2D& SkinSize)
{
	if (!SkinTexture)
	{
		return SecondaryButton();
	}

	auto MakeSkin = [SkinTexture, SkinSize](const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(SkinTexture);
		// 9-slice: الحواف تبقى بحجمها والوسط يتمدد، فيصلح جلد واحد لكل عرض.
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(0.25f);
		Brush.ImageSize = FVector2D(SkinSize);
		Brush.TintColor = Tint;
		return Brush;
	};

	FButtonStyle Style;
	Style.SetNormal(MakeSkin(FLinearColor::White));
	Style.SetHovered(MakeSkin(FLinearColor(1.12f, 1.12f, 1.12f, 1.f)));
	Style.SetPressed(MakeSkin(FLinearColor(0.78f, 0.78f, 0.78f, 1.f)));
	Style.SetDisabled(MakeSkin(FLinearColor(0.42f, 0.42f, 0.42f, 0.7f)));
	Style.SetPressedPadding(FMargin(0.f, 1.f, 0.f, -1.f));
	return Style;
}
