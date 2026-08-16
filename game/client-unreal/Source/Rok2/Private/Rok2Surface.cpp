// Copyright ROK2. مصنع أسطح وأنماط أزرار الواجهة (P17-T1) — implementation.

#include "Rok2Surface.h"
#include "Rok2VisualTheme.h"
#include "Brushes/SlateRoundedBoxBrush.h"

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
}

// ---------------------------------------------------------------------------
// أسطح
// ---------------------------------------------------------------------------

FSlateBrush Rok2Surface::Panel()
{
	return FSlateRoundedBoxBrush(Rok2Visual::Panel(), Rok2Radius::Panel, Rok2Visual::Edge(), 1.5f);
}

FSlateBrush Rok2Surface::Sheet()
{
	return FSlateRoundedBoxBrush(Rok2Visual::Panel(), Rok2Radius::Sheet, Rok2Visual::Edge(), 1.5f);
}

FSlateBrush Rok2Surface::Card()
{
	const FLinearColor SoftEdge(Rok2Visual::Edge().R, Rok2Visual::Edge().G, Rok2Visual::Edge().B, 0.28f);
	return FSlateRoundedBoxBrush(Rok2Visual::Card(), Rok2Radius::Card, SoftEdge, 1.f);
}

FSlateBrush Rok2Surface::AccentCard(const FLinearColor& Accent)
{
	return FSlateRoundedBoxBrush(Rok2Visual::Card(), Rok2Radius::Card, Accent, 2.f);
}

FSlateBrush Rok2Surface::TopBar()
{
	// الشريط يلتصق بأعلى الشاشة، فاستدارة أعلاه ستكشف العالم خلفه. الحافة
	// الذهبية وحدها تفصله عن المشهد.
	return FSlateRoundedBoxBrush(Rok2Visual::Bar(), Rok2Radius::None, Rok2Visual::Edge(), 1.5f);
}

FSlateBrush Rok2Surface::Pill(const FLinearColor& Fill)
{
	return FSlateRoundedBoxBrush(Fill, Rok2Radius::Full);
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
// أنماط الأزرار
// ---------------------------------------------------------------------------

FButtonStyle Rok2Surface::PrimaryButton()
{
	return MakeFilledStyle(Rok2Visual::PrimaryAction(), Rok2Radius::Card, Rok2Visual::Gold(), 1.5f);
}

FButtonStyle Rok2Surface::SecondaryButton()
{
	return MakeFilledStyle(Rok2Visual::Card(), Rok2Radius::Card, Rok2Visual::Edge(), 1.5f);
}

FButtonStyle Rok2Surface::DangerButton()
{
	return MakeFilledStyle(Rok2Visual::Danger(), Rok2Radius::Card, Rok2Visual::DangerText(), 1.5f);
}

FButtonStyle Rok2Surface::SuccessButton()
{
	return MakeFilledStyle(Rok2Visual::Success(), Rok2Radius::Card, Rok2Visual::SuccessText(), 1.5f);
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
