// Copyright ROK2. Unified procedural UI icon library (P6-T1) — implementation.
//
// يرسم كل أيقونة كـ UTexture2D 32×32 (أو الحجم المطلوب) بأشكال vector بسيطة:
// خطوط (DrawLine)، مستطيلات، دوائر، مثلثات، مع حواف ناعمة (alpha falloff).
// الرسم يتم مرة واحدة لكل id@size ويُخبأ — لا كلفة رسم متكررة في Tick.

#include "Rok2IconLibrary.h"
#include "Engine/Texture2D.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Math/Color.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Icons, Log, All);

// ---------------------------------------------------------------------------
// لوحة الألوان من ui-ux-design-system.md §1
// ---------------------------------------------------------------------------
namespace Rok2IconPalette
{
	static const FColor Ivory(245, 233, 208, 255);   // #F5E9D0
	static const FColor Transparent(0, 0, 0, 0);
}

// ---------------------------------------------------------------------------
// سياق الرسم: بكسلات RGBA + أدوات شكل بدائية
// ---------------------------------------------------------------------------
struct FRok2IconCanvas
{
	TArray<FColor> Px;
	int32 W = 32;
	int32 H = 32;

	void Clear()
	{
		Px.SetNumZeroed(W * H);
	}

	void Set(int32 X, int32 Y, FColor C)
	{
		if (X < 0 || Y < 0 || X >= W || Y >= H) return;
		FColor& D = Px[Y * W + X];
		// دمج alpha بسيط: المصدر فوق الوجهة
		if (C.A == 255) { D = C; return; }
		if (C.A == 0) return;
		const float A = C.A / 255.f;
		D.R = (uint8)FMath::RoundToInt(C.R * A + D.R * (1.f - A));
		D.G = (uint8)FMath::RoundToInt(C.G * A + D.G * (1.f - A));
		D.B = (uint8)FMath::RoundToInt(C.B * A + D.B * (1.f - A));
		D.A = (uint8)FMath::Min(255, D.A + C.A);
	}

	/** دائرة ممتلئة */
	void Disc(float Cx, float Cy, float R, FColor C)
	{
		const int32 X0 = FMath::FloorToInt(Cx - R - 1);
		const int32 X1 = FMath::CeilToInt(Cx + R + 1);
		const int32 Y0 = FMath::FloorToInt(Cy - R - 1);
		const int32 Y1 = FMath::CeilToInt(Cy + R + 1);
		for (int32 Y = Y0; Y <= Y1; ++Y)
		{
			for (int32 X = X0; X <= X1; ++X)
			{
				const float D = FMath::Sqrt(FMath::Square(X - Cx) + FMath::Square(Y - Cy));
				if (D <= R - 0.5f) Set(X, Y, C);
				else if (D <= R + 0.5f)
				{
					FColor Soft = C;
					Soft.A = (uint8)FMath::RoundToInt(C.A * (R + 0.5f - D));
					Set(X, Y, Soft);
				}
			}
		}
	}

	/** حلقة (محيط دائرة) */
	void Ring(float Cx, float Cy, float R, float Thickness, FColor C)
	{
		const float Inner = R - Thickness;
		const int32 X0 = FMath::FloorToInt(Cx - R - 1);
		const int32 X1 = FMath::CeilToInt(Cx + R + 1);
		const int32 Y0 = FMath::FloorToInt(Cy - R - 1);
		const int32 Y1 = FMath::CeilToInt(Cy + R + 1);
		for (int32 Y = Y0; Y <= Y1; ++Y)
		{
			for (int32 X = X0; X <= X1; ++X)
			{
				const float D = FMath::Sqrt(FMath::Square(X - Cx) + FMath::Square(Y - Cy));
				if (D >= Inner && D <= R) Set(X, Y, C);
			}
		}
	}

	/** خط من نقطة لنقطة بسماكة Thickness */
	void Line(float X1, float Y1, float X2, float Y2, float Thickness, FColor C)
	{
		const float Len = FMath::Sqrt(FMath::Square(X2 - X1) + FMath::Square(Y2 - Y1));
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Len * 2.f));
		const float Half = Thickness * 0.5f;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const float T = (float)i / (float)Steps;
			Disc(FMath::Lerp(X1, X2, T), FMath::Lerp(Y1, Y2, T), Half, C);
		}
	}

	/** مستطيل ممتلئ */
	void Rect(float X0, float Y0, float X1, float Y1, FColor C)
	{
		for (int32 Y = FMath::RoundToInt(Y0); Y <= FMath::RoundToInt(Y1); ++Y)
		{
			for (int32 X = FMath::RoundToInt(X0); X <= FMath::RoundToInt(X1); ++X)
			{
				Set(X, Y, C);
			}
		}
	}

	/** إطار مستطيل */
	void RectFrame(float X0, float Y0, float X1, float Y1, float Thickness, FColor C)
	{
		Line(X0, Y0, X1, Y0, Thickness, C);
		Line(X1, Y0, X1, Y1, Thickness, C);
		Line(X1, Y1, X0, Y1, Thickness, C);
		Line(X0, Y1, X0, Y0, Thickness, C);
	}

	/** مثلث ممتلئ (رؤوس A/B/C) */
	void Triangle(FVector2D A, FVector2D B, FVector2D Cpt, FColor C)
	{
		const float MinX = FMath::Min3(A.X, B.X, Cpt.X);
		const float MaxX = FMath::Max3(A.X, B.X, Cpt.X);
		const float MinY = FMath::Min3(A.Y, B.Y, Cpt.Y);
		const float MaxY = FMath::Max3(A.Y, B.Y, Cpt.Y);
		auto Sign = [](FVector2D P1, FVector2D P2, FVector2D P3)
		{
			return (P1.X - P3.X) * (P2.Y - P3.Y) - (P2.X - P3.X) * (P1.Y - P3.Y);
		};
		for (int32 Y = FMath::FloorToInt(MinY); Y <= FMath::CeilToInt(MaxY); ++Y)
		{
			for (int32 X = FMath::FloorToInt(MinX); X <= FMath::CeilToInt(MaxX); ++X)
			{
				const FVector2D P(X, Y);
				const float D1 = Sign(P, A, B);
				const float D2 = Sign(P, B, Cpt);
				const float D3 = Sign(P, Cpt, A);
				const bool bNeg = (D1 < 0) || (D2 < 0) || (D3 < 0);
				const bool bPos = (D1 > 0) || (D2 > 0) || (D3 > 0);
				if (!(bNeg && bPos)) Set(X, Y, C);
			}
		}
	}

	/** مضلع منتظم (للتاج والنجمة) */
	void RegularPoly(float Cx, float Cy, float R, int32 Sides, float RotationRad, FColor C)
	{
		TArray<FVector2D> V;
		for (int32 i = 0; i < Sides; ++i)
		{
			const float A = RotationRad + (2.f * PI * i) / Sides;
			V.Add(FVector2D(Cx + R * FMath::Cos(A), Cy + R * FMath::Sin(A)));
		}
		for (int32 i = 0; i < Sides; ++i)
		{
			const FVector2D& P1 = V[i];
			const FVector2D& P2 = V[(i + 1) % Sides];
			Line(P1.X, P1.Y, P2.X, P2.Y, 2.f, C);
		}
	}

	/** نجمة خماسية ممتلئة */
	void Star5(float Cx, float Cy, float R, FColor C)
	{
		TArray<FVector2D> Outer, Inner;
		for (int32 i = 0; i < 5; ++i)
		{
			const float AO = -PI / 2.f + (2.f * PI * i) / 5.f;
			const float AI = AO + PI / 5.f;
			Outer.Add(FVector2D(Cx + R * FMath::Cos(AO), Cy + R * FMath::Sin(AO)));
			Inner.Add(FVector2D(Cx + R * 0.42f * FMath::Cos(AI), Cy + R * 0.42f * FMath::Sin(AI)));
		}
		for (int32 i = 0; i < 5; ++i)
		{
			Triangle(FVector2D(Cx, Cy), Outer[i], Inner[i], C);
			Triangle(FVector2D(Cx, Cy), Inner[i], Outer[(i + 1) % 5], C);
		}
	}
};

// ---------------------------------------------------------------------------
// الرسامون: دالة واحدة لكل أيقونة، مرسومة على شبكة 32×32 منطقية ثم تُقاس للحجم
// المطلوب. الرسم بالإحداثيات المنطقية s = Size/32 (نستخدم الحجم الفعلي مباشرة).
// ---------------------------------------------------------------------------

#define ROK2_ICON_COMMON(FN) static void FN(FRok2IconCanvas& C, float S, FColor Col)

// طعام: كوب حساء (وعاء + بخار)
ROK2_ICON_COMMON(DrawFood)
{
	C.Disc(16*S/32.f, 20*S/32.f, 9*S/32.f, Col);          // جسم الوعاء
	C.Rect(4*S/32.f, 20*S/32.f, 28*S/32.f, 26*S/32.f, Col); // قاعدة
	C.Line(12*S/32.f, 6*S/32.f, 10*S/32.f, 10*S/32.f, 2*S/32.f, Col);  // بخار 1
	C.Line(16*S/32.f, 4*S/32.f, 16*S/32.f, 9*S/32.f, 2*S/32.f, Col);   // بخار 2
	C.Line(20*S/32.f, 6*S/32.f, 22*S/32.f, 10*S/32.f, 2*S/32.f, Col);  // بخار 3
}

// خشب: جذع مقطوع موضوع أفقياً (جسم + نهاية دائرية بحلقات نمو)
ROK2_ICON_COMMON(DrawWood)
{
	C.Rect(6*S/32.f, 12*S/32.f, 24*S/32.f, 22*S/32.f, Col);              // جسم الجذع
	C.Disc(24*S/32.f, 17*S/32.f, 6*S/32.f, Col);                         // المقطع
	C.Ring(24*S/32.f, 17*S/32.f, 4*S/32.f, 1.2f*S/32.f, FColor(60, 45, 20, 255));  // حلقة نمو 1
	C.Ring(24*S/32.f, 17*S/32.f, 2*S/32.f, 1.2f*S/32.f, FColor(60, 45, 20, 255));  // حلقة نمو 2
	C.Line(9*S/32.f, 15*S/32.f, 20*S/32.f, 15*S/32.f, 1.f*S/32.f, FColor(60, 45, 20, 200)); // عروق
	C.Line(8*S/32.f, 19*S/32.f, 19*S/32.f, 19*S/32.f, 1.f*S/32.f, FColor(60, 45, 20, 200));
}

// حجر: مكعب (مربع + ظل علوي)
ROK2_ICON_COMMON(DrawStone)
{
	C.Rect(8*S/32.f, 12*S/32.f, 24*S/32.f, 26*S/32.f, Col);
	C.Triangle(FVector2D(8*S/32.f, 12*S/32.f), FVector2D(24*S/32.f, 12*S/32.f), FVector2D(16*S/32.f, 5*S/32.f), Col);
}

// ذهب: عملة (قرص + إطار داخلي)
ROK2_ICON_COMMON(DrawGold)
{
	C.Disc(16*S/32.f, 16*S/32.f, 11*S/32.f, Col);
	C.Ring(16*S/32.f, 16*S/32.f, 8*S/32.f, 2.f*S/32.f, FColor(60, 45, 10, 255));
}

// جواهر: معيّن
ROK2_ICON_COMMON(DrawGems)
{
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(27*S/32.f, 16*S/32.f), FVector2D(16*S/32.f, 29*S/32.f), Col);
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(5*S/32.f, 16*S/32.f), FVector2D(16*S/32.f, 29*S/32.f), Col);
}

// طاقة AP: برق
ROK2_ICON_COMMON(DrawAp)
{
	C.Triangle(FVector2D(19*S/32.f, 2*S/32.f), FVector2D(10*S/32.f, 18*S/32.f), FVector2D(16*S/32.f, 18*S/32.f), Col);
	C.Triangle(FVector2D(13*S/32.f, 30*S/32.f), FVector2D(22*S/32.f, 14*S/32.f), FVector2D(16*S/32.f, 14*S/32.f), Col);
}

// بناء: مطرقة
ROK2_ICON_COMMON(DrawBuild)
{
	C.Line(9*S/32.f, 23*S/32.f, 21*S/32.f, 11*S/32.f, 3.5f*S/32.f, Col); // المقبض
	C.Rect(16*S/32.f, 3*S/32.f, 27*S/32.f, 11*S/32.f, Col);               // الرأس
}

// سيف
ROK2_ICON_COMMON(DrawSword)
{
	C.Line(6*S/32.f, 26*S/32.f, 22*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col); // النصل
	C.Triangle(FVector2D(22*S/32.f, 10*S/32.f), FVector2D(26*S/32.f, 4*S/32.f), FVector2D(25*S/32.f, 9*S/32.f), Col); // الرأس
	C.Line(9*S/32.f, 20*S/32.f, 14*S/32.f, 25*S/32.f, 3.f*S/32.f, Col);  // الحارس
	C.Line(5*S/32.f, 28*S/32.f, 8*S/32.f, 25*S/32.f, 3.5f*S/32.f, Col);  // القبضة
}

// درع
ROK2_ICON_COMMON(DrawShield)
{
	C.Line(16*S/32.f, 3*S/32.f, 27*S/32.f, 7*S/32.f, 2.5f*S/32.f, Col);
	C.Line(27*S/32.f, 7*S/32.f, 25*S/32.f, 20*S/32.f, 2.5f*S/32.f, Col);
	C.Line(25*S/32.f, 20*S/32.f, 16*S/32.f, 29*S/32.f, 2.5f*S/32.f, Col);
	C.Line(16*S/32.f, 29*S/32.f, 7*S/32.f, 20*S/32.f, 2.5f*S/32.f, Col);
	C.Line(7*S/32.f, 20*S/32.f, 5*S/32.f, 7*S/32.f, 2.5f*S/32.f, Col);
	C.Line(5*S/32.f, 7*S/32.f, 16*S/32.f, 3*S/32.f, 2.5f*S/32.f, Col);
}

// خوذة (قادة)
ROK2_ICON_COMMON(DrawHelmet)
{
	C.Ring(16*S/32.f, 17*S/32.f, 10*S/32.f, 3.f*S/32.f, Col);            // القبة
	C.Line(16*S/32.f, 7*S/32.f, 16*S/32.f, 2*S/32.f, 2.5f*S/32.f, Col);  // الريشة
	C.Line(6*S/32.f, 20*S/32.f, 6*S/32.f, 27*S/32.f, 3.f*S/32.f, Col);   // الخد أيسر
	C.Line(26*S/32.f, 20*S/32.f, 26*S/32.f, 27*S/32.f, 3.f*S/32.f, Col); // الخد أيمن
	C.Line(6*S/32.f, 27*S/32.f, 26*S/32.f, 27*S/32.f, 3.f*S/32.f, Col);  // الأساس
}

// حقيبة
ROK2_ICON_COMMON(DrawBag)
{
	C.Ring(16*S/32.f, 10*S/32.f, 5*S/32.f, 2.5f*S/32.f, Col);            // المقبض
	C.Rect(7*S/32.f, 12*S/32.f, 25*S/32.f, 28*S/32.f, Col);              // الجسم
	C.Line(11*S/32.f, 18*S/32.f, 21*S/32.f, 18*S/32.f, 2.f*S/32.f, FColor(60, 45, 20, 255)); // الجيب
}

// راية (أحداث)
ROK2_ICON_COMMON(DrawBanner)
{
	C.Line(7*S/32.f, 3*S/32.f, 7*S/32.f, 29*S/32.f, 2.5f*S/32.f, Col);   // السارية
	C.Triangle(FVector2D(7*S/32.f, 4*S/32.f), FVector2D(27*S/32.f, 8*S/32.f), FVector2D(7*S/32.f, 14*S/32.f), Col); // الراية
}

// مخطوط (تقارير)
ROK2_ICON_COMMON(DrawScroll)
{
	C.RectFrame(8*S/32.f, 6*S/32.f, 24*S/32.f, 26*S/32.f, 2.f*S/32.f, Col);
	C.Line(12*S/32.f, 12*S/32.f, 20*S/32.f, 12*S/32.f, 1.5f*S/32.f, Col);
	C.Line(12*S/32.f, 16*S/32.f, 20*S/32.f, 16*S/32.f, 1.5f*S/32.f, Col);
	C.Line(12*S/32.f, 20*S/32.f, 17*S/32.f, 20*S/32.f, 1.5f*S/32.f, Col);
}

// خريطة (بوصلة)
ROK2_ICON_COMMON(DrawMap)
{
	C.Ring(16*S/32.f, 16*S/32.f, 12*S/32.f, 2.5f*S/32.f, Col);
	C.Triangle(FVector2D(16*S/32.f, 6*S/32.f), FVector2D(19*S/32.f, 16*S/32.f), FVector2D(13*S/32.f, 16*S/32.f), Col);
	C.Disc(16*S/32.f, 16*S/32.f, 2*S/32.f, Col);
}

// تحرير (مسطرة وقلم)
ROK2_ICON_COMMON(DrawEdit)
{
	C.Line(7*S/32.f, 25*S/32.f, 25*S/32.f, 7*S/32.f, 3.f*S/32.f, Col);   // القلم
	C.Triangle(FVector2D(5*S/32.f, 28*S/32.f), FVector2D(8*S/32.f, 24*S/32.f), FVector2D(10*S/32.f, 27*S/32.f), Col); // السن
}

// جرس
ROK2_ICON_COMMON(DrawBell)
{
	C.Ring(16*S/32.f, 15*S/32.f, 8*S/32.f, 2.5f*S/32.f, Col);
	C.Rect(8*S/32.f, 15*S/32.f, 24*S/32.f, 22*S/32.f, Col);
	C.Line(6*S/32.f, 23*S/32.f, 26*S/32.f, 23*S/32.f, 2.5f*S/32.f, Col); // الحافة
	C.Disc(16*S/32.f, 27*S/32.f, 2.5f*S/32.f, Col);                      // اللسان
	C.Disc(16*S/32.f, 5*S/32.f, 2*S/32.f, Col);                          // العروة
}

// قفل (مناطق مقفلة)
ROK2_ICON_COMMON(DrawLock)
{
	C.Rect(9*S/32.f, 15*S/32.f, 23*S/32.f, 28*S/32.f, Col);              // الجسم
	C.Ring(16*S/32.f, 15*S/32.f, 6*S/32.f, 3.f*S/32.f, Col);             // القوس
	C.Disc(16*S/32.f, 21*S/32.f, 2*S/32.f, FColor(60, 45, 20, 255));     // الثقب
}

// تقويم (يوم الموسم)
ROK2_ICON_COMMON(DrawCalendar)
{
	C.RectFrame(5*S/32.f, 8*S/32.f, 27*S/32.f, 28*S/32.f, 2.5f*S/32.f, Col);
	C.Line(5*S/32.f, 14*S/32.f, 27*S/32.f, 14*S/32.f, 2.5f*S/32.f, Col); // خط الشهر
	C.Line(10*S/32.f, 4*S/32.f, 10*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col); // حلقة 1
	C.Line(22*S/32.f, 4*S/32.f, 22*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col); // حلقة 2
	C.Disc(12*S/32.f, 20*S/32.f, 1.8f*S/32.f, Col);
	C.Disc(20*S/32.f, 20*S/32.f, 1.8f*S/32.f, Col);
	C.Disc(12*S/32.f, 25*S/32.f, 1.8f*S/32.f, Col);
}

// ساعة رملية (طوابير)
ROK2_ICON_COMMON(DrawHourglass)
{
	C.Line(9*S/32.f, 4*S/32.f, 23*S/32.f, 4*S/32.f, 2.5f*S/32.f, Col);
	C.Line(9*S/32.f, 28*S/32.f, 23*S/32.f, 28*S/32.f, 2.5f*S/32.f, Col);
	C.Line(10*S/32.f, 4*S/32.f, 16*S/32.f, 16*S/32.f, 2.f*S/32.f, Col);
	C.Line(22*S/32.f, 4*S/32.f, 16*S/32.f, 16*S/32.f, 2.f*S/32.f, Col);
	C.Line(10*S/32.f, 28*S/32.f, 16*S/32.f, 16*S/32.f, 2.f*S/32.f, Col);
	C.Line(22*S/32.f, 28*S/32.f, 16*S/32.f, 16*S/32.f, 2.f*S/32.f, Col);
}

// قارورة بحث (أكاديمية/تقنية)
ROK2_ICON_COMMON(DrawFlask)
{
	C.Line(13*S/32.f, 3*S/32.f, 19*S/32.f, 3*S/32.f, 2.5f*S/32.f, Col);  // الفوهة
	C.Line(13*S/32.f, 3*S/32.f, 13*S/32.f, 12*S/32.f, 2.f*S/32.f, Col);
	C.Line(19*S/32.f, 3*S/32.f, 19*S/32.f, 12*S/32.f, 2.f*S/32.f, Col);
	C.Line(13*S/32.f, 12*S/32.f, 6*S/32.f, 27*S/32.f, 2.f*S/32.f, Col);
	C.Line(19*S/32.f, 12*S/32.f, 26*S/32.f, 27*S/32.f, 2.f*S/32.f, Col);
	C.Line(6*S/32.f, 27*S/32.f, 26*S/32.f, 27*S/32.f, 2.5f*S/32.f, Col);
	C.Triangle(FVector2D(11*S/32.f, 19*S/32.f), FVector2D(21*S/32.f, 19*S/32.f), FVector2D(16*S/32.f, 27*S/32.f), Col); // السائل
}

// مستشفى (صليب)
ROK2_ICON_COMMON(DrawCross)
{
	C.Rect(13*S/32.f, 5*S/32.f, 19*S/32.f, 27*S/32.f, Col);
	C.Rect(5*S/32.f, 13*S/32.f, 27*S/32.f, 19*S/32.f, Col);
}

// كشافة (منظار)
ROK2_ICON_COMMON(DrawScout)
{
	C.Ring(13*S/32.f, 13*S/32.f, 8*S/32.f, 3.f*S/32.f, Col);             // العدسة
	C.Line(19*S/32.f, 19*S/32.f, 27*S/32.f, 27*S/32.f, 3.5f*S/32.f, Col); // المقبض
}

// إغلاق ×
ROK2_ICON_COMMON(DrawClose)
{
	C.Line(8*S/32.f, 8*S/32.f, 24*S/32.f, 24*S/32.f, 3.f*S/32.f, Col);
	C.Line(24*S/32.f, 8*S/32.f, 8*S/32.f, 24*S/32.f, 3.f*S/32.f, Col);
}

// نجمة (ترقية مستوى)
ROK2_ICON_COMMON(DrawStar)
{
	C.Star5(16*S/32.f, 16*S/32.f, 11*S/32.f, Col);
}

// جمجمة (قتلى)
ROK2_ICON_COMMON(DrawSkull)
{
	C.Disc(16*S/32.f, 14*S/32.f, 9*S/32.f, Col);                         // القحف
	C.Rect(11*S/32.f, 19*S/32.f, 21*S/32.f, 26*S/32.f, Col);             // الفك
	C.Disc(13*S/32.f, 13*S/32.f, 2.2f*S/32.f, FColor(60, 45, 20, 255));  // عين 1
	C.Disc(19*S/32.f, 13*S/32.f, 2.2f*S/32.f, FColor(60, 45, 20, 255));  // عين 2
}

// قطرة دم (جريح خطير)
ROK2_ICON_COMMON(DrawBlood)
{
	C.Triangle(FVector2D(16*S/32.f, 4*S/32.f), FVector2D(25*S/32.f, 20*S/32.f), FVector2D(7*S/32.f, 20*S/32.f), Col);
	C.Disc(16*S/32.f, 20*S/32.f, 8*S/32.f, Col);
}

// ضمادة (جريح خفيف)
ROK2_ICON_COMMON(DrawBandage)
{
	C.Rect(5*S/32.f, 12*S/32.f, 27*S/32.f, 20*S/32.f, Col);
	C.Disc(16*S/32.f, 16*S/32.f, 3*S/32.f, FColor(60, 45, 20, 255));
	C.Disc(10*S/32.f, 16*S/32.f, 1.5f*S/32.f, FColor(60, 45, 20, 255));
	C.Disc(22*S/32.f, 16*S/32.f, 1.5f*S/32.f, FColor(60, 45, 20, 255));
}

// كأس (نصر)
ROK2_ICON_COMMON(DrawTrophy)
{
	C.Rect(10*S/32.f, 5*S/32.f, 22*S/32.f, 16*S/32.f, Col);              // الكأس
	C.Ring(7*S/32.f, 10*S/32.f, 4*S/32.f, 2.f*S/32.f, Col);              // أذن 1
	C.Ring(25*S/32.f, 10*S/32.f, 4*S/32.f, 2.f*S/32.f, Col);             // أذن 2
	C.Line(16*S/32.f, 16*S/32.f, 16*S/32.f, 23*S/32.f, 3.f*S/32.f, Col); // الساق
	C.Line(10*S/32.f, 27*S/32.f, 22*S/32.f, 27*S/32.f, 3.f*S/32.f, Col); // القاعدة
	C.Line(12*S/32.f, 23*S/32.f, 20*S/32.f, 23*S/32.f, 3.f*S/32.f, Col);
}

// مصافحة (تعادل/تحالف)
ROK2_ICON_COMMON(DrawHandshake)
{
	C.Line(4*S/32.f, 12*S/32.f, 12*S/32.f, 18*S/32.f, 3.f*S/32.f, Col);  // ذراع 1
	C.Line(28*S/32.f, 12*S/32.f, 20*S/32.f, 18*S/32.f, 3.f*S/32.f, Col); // ذراع 2
	C.Line(12*S/32.f, 18*S/32.f, 20*S/32.f, 18*S/32.f, 3.f*S/32.f, Col); // الكفان
	C.Line(12*S/32.f, 18*S/32.f, 16*S/32.f, 24*S/32.f, 2.5f*S/32.f, Col);
	C.Line(20*S/32.f, 18*S/32.f, 16*S/32.f, 24*S/32.f, 2.5f*S/32.f, Col);
}

// تحديث (سهمان دائريان)
ROK2_ICON_COMMON(DrawRefresh)
{
	C.Ring(16*S/32.f, 16*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col);
	C.Triangle(FVector2D(24*S/32.f, 6*S/32.f), FVector2D(28*S/32.f, 14*S/32.f), FVector2D(19*S/32.f, 12*S/32.f), Col); // رأس السهم
}

// هدية (صناديق)
ROK2_ICON_COMMON(DrawGift)
{
	C.Rect(6*S/32.f, 14*S/32.f, 26*S/32.f, 27*S/32.f, Col);              // الصندوق
	C.Line(16*S/32.f, 14*S/32.f, 16*S/32.f, 27*S/32.f, 2.f*S/32.f, FColor(60, 45, 20, 255)); // الشريط
	C.Line(6*S/32.f, 18*S/32.f, 26*S/32.f, 18*S/32.f, 2.f*S/32.f, FColor(60, 45, 20, 255));
	C.Ring(11*S/32.f, 10*S/32.f, 3.5f*S/32.f, 2.f*S/32.f, Col);          // عقدة 1
	C.Ring(21*S/32.f, 10*S/32.f, 3.5f*S/32.f, 2.f*S/32.f, Col);          // عقدة 2
}

// قمح (مزرعة/اقتصاد)
ROK2_ICON_COMMON(DrawWheat)
{
	C.Line(16*S/32.f, 10*S/32.f, 16*S/32.f, 29*S/32.f, 2.5f*S/32.f, Col); // الساق
	for (int32 i = 0; i < 3; ++i)
	{
		const float Y = (10 + i * 5) * S/32.f;
		C.Triangle(FVector2D(16*S/32.f, Y), FVector2D(9*S/32.f, Y + 2*S/32.f), FVector2D(16*S/32.f, Y + 5*S/32.f), Col);
		C.Triangle(FVector2D(16*S/32.f, Y), FVector2D(23*S/32.f, Y + 2*S/32.f), FVector2D(16*S/32.f, Y + 5*S/32.f), Col);
	}
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(12*S/32.f, 9*S/32.f), FVector2D(20*S/32.f, 9*S/32.f), Col); // السنبلة العليا
}

// صندوق (مخزن)
ROK2_ICON_COMMON(DrawBox)
{
	C.Rect(5*S/32.f, 10*S/32.f, 27*S/32.f, 27*S/32.f, Col);
	C.Line(5*S/32.f, 10*S/32.f, 16*S/32.f, 4*S/32.f, 2.f*S/32.f, Col);
	C.Line(27*S/32.f, 10*S/32.f, 16*S/32.f, 4*S/32.f, 2.f*S/32.f, Col);
	C.Line(16*S/32.f, 4*S/32.f, 16*S/32.f, 10*S/32.f, 2.f*S/32.f, Col);
}

// بريد (ظرف)
ROK2_ICON_COMMON(DrawMail)
{
	C.RectFrame(5*S/32.f, 9*S/32.f, 27*S/32.f, 25*S/32.f, 2.5f*S/32.f, Col);
	C.Line(5*S/32.f, 9*S/32.f, 16*S/32.f, 18*S/32.f, 2.f*S/32.f, Col);
	C.Line(27*S/32.f, 9*S/32.f, 16*S/32.f, 18*S/32.f, 2.f*S/32.f, Col);
}

// عربة متجر
ROK2_ICON_COMMON(DrawCart)
{
	C.Line(4*S/32.f, 6*S/32.f, 8*S/32.f, 6*S/32.f, 2.5f*S/32.f, Col);    // المقبض
	C.Line(8*S/32.f, 6*S/32.f, 11*S/32.f, 21*S/32.f, 2.5f*S/32.f, Col);
	C.Line(11*S/32.f, 21*S/32.f, 26*S/32.f, 21*S/32.f, 2.5f*S/32.f, Col);
	C.Line(26*S/32.f, 21*S/32.f, 28*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col);
	C.Line(8*S/32.f, 10*S/32.f, 28*S/32.f, 10*S/32.f, 2.f*S/32.f, Col);
	C.Disc(14*S/32.f, 26*S/32.f, 2.5f*S/32.f, Col);                      // عجلة 1
	C.Disc(23*S/32.f, 26*S/32.f, 2.5f*S/32.f, Col);                      // عجلة 2
}

// حصان (إسطبل/فرسان)
ROK2_ICON_COMMON(DrawHorse)
{
	C.Disc(18*S/32.f, 18*S/32.f, 8*S/32.f, Col);                         // الجسم
	C.Line(10*S/32.f, 12*S/32.f, 7*S/32.f, 5*S/32.f, 3.f*S/32.f, Col);   // الرقبة
	C.Disc(7*S/32.f, 5*S/32.f, 3.5f*S/32.f, Col);                        // الرأس
	C.Line(13*S/32.f, 25*S/32.f, 12*S/32.f, 29*S/32.f, 2.5f*S/32.f, Col); // رجل 1
	C.Line(23*S/32.f, 25*S/32.f, 24*S/32.f, 29*S/32.f, 2.5f*S/32.f, Col); // رجل 2
}

// قوس (رماية)
ROK2_ICON_COMMON(DrawBow)
{
	C.Ring(16*S/32.f, 16*S/32.f, 12*S/32.f, 3.f*S/32.f, Col);            // القوس (قوس دائري)
	// نغطي نصف الحلقة بمثلث شفاف لا ينفع — نرسم وتراً
	C.Line(16*S/32.f, 4*S/32.f, 16*S/32.f, 28*S/32.f, 1.5f*S/32.f, Col); // الوتر
	C.Line(10*S/32.f, 16*S/32.f, 24*S/32.f, 16*S/32.f, 2.f*S/32.f, Col); // السهم
	C.Triangle(FVector2D(24*S/32.f, 16*S/32.f), FVector2D(19*S/32.f, 13*S/32.f), FVector2D(19*S/32.f, 19*S/32.f), Col); // رأس السهم
}

// خيمة (معسكر/كشافة)
ROK2_ICON_COMMON(DrawTent)
{
	C.Triangle(FVector2D(16*S/32.f, 5*S/32.f), FVector2D(4*S/32.f, 27*S/32.f), FVector2D(28*S/32.f, 27*S/32.f), Col);
	C.Triangle(FVector2D(16*S/32.f, 14*S/32.f), FVector2D(11*S/32.f, 27*S/32.f), FVector2D(21*S/32.f, 27*S/32.f), FColor(60, 45, 20, 255)); // المدخل
}

// برج
ROK2_ICON_COMMON(DrawTower)
{
	C.Rect(11*S/32.f, 10*S/32.f, 21*S/32.f, 28*S/32.f, Col);             // الجسم
	C.Rect(9*S/32.f, 6*S/32.f, 13*S/32.f, 10*S/32.f, Col);               // شرفة 1
	C.Rect(14*S/32.f, 4*S/32.f, 18*S/32.f, 10*S/32.f, Col);              // شرفة 2
	C.Rect(19*S/32.f, 6*S/32.f, 23*S/32.f, 10*S/32.f, Col);              // شرفة 3
}

// قلعة (city hall)
ROK2_ICON_COMMON(DrawCastle)
{
	C.Rect(7*S/32.f, 12*S/32.f, 25*S/32.f, 28*S/32.f, Col);              // الجسم
	C.Rect(5*S/32.f, 8*S/32.f, 9*S/32.f, 12*S/32.f, Col);                // شرفة 1
	C.Rect(11*S/32.f, 6*S/32.f, 15*S/32.f, 12*S/32.f, Col);              // شرفة 2
	C.Rect(17*S/32.f, 6*S/32.f, 21*S/32.f, 12*S/32.f, Col);              // شرفة 3
	C.Rect(23*S/32.f, 8*S/32.f, 27*S/32.f, 12*S/32.f, Col);              // شرفة 4
	C.Rect(14*S/32.f, 20*S/32.f, 18*S/32.f, 28*S/32.f, FColor(60, 45, 20, 255)); // الباب
}

// طوب (سور)
ROK2_ICON_COMMON(DrawBricks)
{
	for (int32 Row = 0; Row < 3; ++Row)
	{
		const float Y0 = (8 + Row * 7) * S/32.f;
		const float Off = (Row % 2 == 0) ? 0.f : 6*S/32.f;
		for (int32 ColIdx = 0; ColIdx < 2; ++ColIdx)
		{
			const float X0 = (5 + ColIdx * 13) * S/32.f + Off;
			C.Rect(X0, Y0, X0 + 11*S/32.f, Y0 + 5*S/32.f, Col);
		}
	}
}

// صخرة (محجر)
ROK2_ICON_COMMON(DrawRock)
{
	C.Triangle(FVector2D(16*S/32.f, 6*S/32.f), FVector2D(6*S/32.f, 26*S/32.f), FVector2D(26*S/32.f, 26*S/32.f), Col);
	C.Triangle(FVector2D(16*S/32.f, 12*S/32.f), FVector2D(11*S/32.f, 26*S/32.f), FVector2D(21*S/32.f, 26*S/32.f), FColor(60, 45, 20, 255));
}

// كأس بيرة (حانة)
ROK2_ICON_COMMON(DrawBeer)
{
	C.Rect(9*S/32.f, 10*S/32.f, 21*S/32.f, 27*S/32.f, Col);              // الكوب
	C.Ring(24*S/32.f, 16*S/32.f, 4*S/32.f, 2.5f*S/32.f, Col);            // المقبض
	C.Rect(9*S/32.f, 6*S/32.f, 21*S/32.f, 10*S/32.f, Col);               // الرغوة
	C.Disc(12*S/32.f, 4*S/32.f, 2*S/32.f, Col);
	C.Disc(17*S/32.f, 3*S/32.f, 2.5f*S/32.f, Col);
}

// ميزان (تجارة)
ROK2_ICON_COMMON(DrawScale)
{
	C.Line(16*S/32.f, 5*S/32.f, 16*S/32.f, 27*S/32.f, 2.5f*S/32.f, Col); // العمود
	C.Line(7*S/32.f, 9*S/32.f, 25*S/32.f, 9*S/32.f, 2.5f*S/32.f, Col);   // الكفة
	C.Line(7*S/32.f, 9*S/32.f, 5*S/32.f, 16*S/32.f, 1.5f*S/32.f, Col);   // خيط 1
	C.Line(7*S/32.f, 9*S/32.f, 9*S/32.f, 16*S/32.f, 1.5f*S/32.f, Col);
	C.Line(25*S/32.f, 9*S/32.f, 23*S/32.f, 16*S/32.f, 1.5f*S/32.f, Col); // خيط 2
	C.Line(25*S/32.f, 9*S/32.f, 27*S/32.f, 16*S/32.f, 1.5f*S/32.f, Col);
	C.Ring(7*S/32.f, 17*S/32.f, 3.5f*S/32.f, 2.f*S/32.f, Col);           // كفة 1
	C.Ring(25*S/32.f, 17*S/32.f, 3.5f*S/32.f, 2.f*S/32.f, Col);          // كفة 2
	C.Line(11*S/32.f, 27*S/32.f, 21*S/32.f, 27*S/32.f, 2.5f*S/32.f, Col); // القاعدة
}

// تاج (عرش/ملك)
ROK2_ICON_COMMON(DrawCrown)
{
	C.Triangle(FVector2D(6*S/32.f, 24*S/32.f), FVector2D(6*S/32.f, 12*S/32.f), FVector2D(12*S/32.f, 18*S/32.f), Col);
	C.Triangle(FVector2D(12*S/32.f, 18*S/32.f), FVector2D(16*S/32.f, 8*S/32.f), FVector2D(20*S/32.f, 18*S/32.f), Col);
	C.Triangle(FVector2D(20*S/32.f, 18*S/32.f), FVector2D(26*S/32.f, 12*S/32.f), FVector2D(26*S/32.f, 24*S/32.f), Col);
	C.Rect(6*S/32.f, 24*S/32.f, 26*S/32.f, 28*S/32.f, Col);              // الأساس
	C.Disc(6*S/32.f, 10*S/32.f, 2*S/32.f, Col);
	C.Disc(16*S/32.f, 6*S/32.f, 2*S/32.f, Col);
	C.Disc(26*S/32.f, 10*S/32.f, 2*S/32.f, Col);
}

// بنّاء (خوذة عمل)
ROK2_ICON_COMMON(DrawBuilder)
{
	C.Ring(16*S/32.f, 16*S/32.f, 11*S/32.f, 3.f*S/32.f, Col);            // القبة
	C.Line(4*S/32.f, 18*S/32.f, 28*S/32.f, 18*S/32.f, 3.f*S/32.f, Col);  // الحافة
	C.Line(16*S/32.f, 5*S/32.f, 16*S/32.f, 18*S/32.f, 2.f*S/32.f, FColor(60, 45, 20, 255)); // الخط الأوسط
}

// شارة اتصال (دائرة متصلة)
ROK2_ICON_COMMON(DrawConn)
{
	C.Disc(16*S/32.f, 16*S/32.f, 8*S/32.f, Col);
	C.Ring(16*S/32.f, 16*S/32.f, 12*S/32.f, 2.f*S/32.f, Col);
}

// مقبض مطرقة عرضية (تسريع ⏩ → سهم سريع)
ROK2_ICON_COMMON(DrawSpeedup)
{
	C.Triangle(FVector2D(6*S/32.f, 8*S/32.f), FVector2D(6*S/32.f, 24*S/32.f), FVector2D(16*S/32.f, 16*S/32.f), Col);
	C.Triangle(FVector2D(16*S/32.f, 8*S/32.f), FVector2D(16*S/32.f, 24*S/32.f), FVector2D(26*S/32.f, 16*S/32.f), Col);
}

// تاج ملكي صغير للاعب (governor)
ROK2_ICON_COMMON(DrawGovernor)
{
	C.Disc(16*S/32.f, 12*S/32.f, 5*S/32.f, Col);                         // الرأس
	C.Ring(16*S/32.f, 26*S/32.f, 9*S/32.f, 4.f*S/32.f, Col);             // الكتفان
}

// لوحة نقاط/إحصاء
ROK2_ICON_COMMON(DrawStats)
{
	C.Line(8*S/32.f, 26*S/32.f, 8*S/32.f, 18*S/32.f, 3.5f*S/32.f, Col);
	C.Line(16*S/32.f, 26*S/32.f, 16*S/32.f, 10*S/32.f, 3.5f*S/32.f, Col);
	C.Line(24*S/32.f, 26*S/32.f, 24*S/32.f, 5*S/32.f, 3.5f*S/32.f, Col);
}

// أثر قدم (حركة)
ROK2_ICON_COMMON(DrawMove)
{
	C.Disc(11*S/32.f, 11*S/32.f, 4*S/32.f, Col);
	C.Disc(21*S/32.f, 21*S/32.f, 4*S/32.f, Col);
	C.Disc(14*S/32.f, 6*S/32.f, 1.8f*S/32.f, Col);
	C.Disc(8*S/32.f, 8*S/32.f, 1.8f*S/32.f, Col);
	C.Disc(24*S/32.f, 16*S/32.f, 1.8f*S/32.f, Col);
	C.Disc(18*S/32.f, 26*S/32.f, 1.8f*S/32.f, Col);
}

// شرارة/بريق (passive skill)
ROK2_ICON_COMMON(DrawSparkle)
{
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(18*S/32.f, 14*S/32.f), FVector2D(14*S/32.f, 14*S/32.f), Col);
	C.Triangle(FVector2D(16*S/32.f, 29*S/32.f), FVector2D(18*S/32.f, 18*S/32.f), FVector2D(14*S/32.f, 18*S/32.f), Col);
	C.Triangle(FVector2D(3*S/32.f, 16*S/32.f), FVector2D(14*S/32.f, 14*S/32.f), FVector2D(14*S/32.f, 18*S/32.f), Col);
	C.Triangle(FVector2D(29*S/32.f, 16*S/32.f), FVector2D(18*S/32.f, 14*S/32.f), FVector2D(18*S/32.f, 18*S/32.f), Col);
	C.Disc(16*S/32.f, 16*S/32.f, 3*S/32.f, Col);
}

// خوذة قتال (فروع المواهب الحمراء)
ROK2_ICON_COMMON(DrawCombat)
{
	C.Line(8*S/32.f, 24*S/32.f, 24*S/32.f, 8*S/32.f, 3.f*S/32.f, Col);
	C.Line(8*S/32.f, 8*S/32.f, 24*S/32.f, 24*S/32.f, 3.f*S/32.f, Col);   // سيفان متقاطعان
}

// خاتم (إكسسوار)
ROK2_ICON_COMMON(DrawRing)
{
	C.Ring(16*S/32.f, 19*S/32.f, 8*S/32.f, 3.f*S/32.f, Col);
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(12*S/32.f, 10*S/32.f), FVector2D(20*S/32.f, 10*S/32.f), Col); // الحجر
}

// حذاء (boots)
ROK2_ICON_COMMON(DrawBoots)
{
	C.Rect(9*S/32.f, 5*S/32.f, 17*S/32.f, 20*S/32.f, Col);               // الساق
	C.Rect(9*S/32.f, 20*S/32.f, 26*S/32.f, 27*S/32.f, Col);              // القدم
}

// سهم ترقية (➔)
ROK2_ICON_COMMON(DrawArrowRight)
{
	C.Line(5*S/32.f, 16*S/32.f, 25*S/32.f, 16*S/32.f, 3.f*S/32.f, Col);
	C.Triangle(FVector2D(25*S/32.f, 16*S/32.f), FVector2D(17*S/32.f, 9*S/32.f), FVector2D(17*S/32.f, 23*S/32.f), Col);
}

// برق مهارة (skill up)
ROK2_ICON_COMMON(DrawSkillUp)
{
	DrawAp(C, S, Col);
	C.Line(20*S/32.f, 6*S/32.f, 28*S/32.f, 6*S/32.f, 2.5f*S/32.f, Col);  // علامة +
	C.Line(24*S/32.f, 2*S/32.f, 24*S/32.f, 10*S/32.f, 2.5f*S/32.f, Col);
}

// منجم ذهب (pickaxe)
ROK2_ICON_COMMON(DrawPickaxe)
{
	C.Line(10*S/32.f, 22*S/32.f, 22*S/32.f, 10*S/32.f, 3.f*S/32.f, Col); // المقبض
	C.Triangle(FVector2D(16*S/32.f, 4*S/32.f), FVector2D(27*S/32.f, 8*S/32.f), FVector2D(21*S/32.f, 13*S/32.f), Col); // الرأس المنحني
	C.Triangle(FVector2D(16*S/32.f, 4*S/32.f), FVector2D(5*S/32.f, 8*S/32.f), FVector2D(11*S/32.f, 13*S/32.f), Col);
}

// ساعة (⏱️ وقت)
ROK2_ICON_COMMON(DrawClock)
{
	C.Ring(16*S/32.f, 17*S/32.f, 11*S/32.f, 2.5f*S/32.f, Col);
	C.Line(16*S/32.f, 17*S/32.f, 16*S/32.f, 9*S/32.f, 2.f*S/32.f, Col);  // عقرب دقائق
	C.Line(16*S/32.f, 17*S/32.f, 21*S/32.f, 17*S/32.f, 2.f*S/32.f, Col); // عقرب ساعات
	C.Disc(16*S/32.f, 17*S/32.f, 1.8f*S/32.f, Col);
	C.Line(13*S/32.f, 4*S/32.f, 19*S/32.f, 4*S/32.f, 2.5f*S/32.f, Col);  // زر التاج
}

// لوح فنون (زخرفة 🎨)
ROK2_ICON_COMMON(DrawArt)
{
	C.Ring(16*S/32.f, 16*S/32.f, 11*S/32.f, 2.5f*S/32.f, Col);           // لوحة الرسام
	C.Disc(11*S/32.f, 12*S/32.f, 2.f*S/32.f, Col);
	C.Disc(18*S/32.f, 10*S/32.f, 2.f*S/32.f, Col);
	C.Disc(23*S/32.f, 16*S/32.f, 2.f*S/32.f, Col);
	C.Disc(13*S/32.f, 21*S/32.f, 2.5f*S/32.f, FColor(60, 45, 20, 255));  // ثقب الإبهام
}

// نصب (monument)
ROK2_ICON_COMMON(DrawMonument)
{
	C.Triangle(FVector2D(16*S/32.f, 3*S/32.f), FVector2D(12*S/32.f, 24*S/32.f), FVector2D(20*S/32.f, 24*S/32.f), Col); // المسلة
	C.Rect(9*S/32.f, 24*S/32.f, 23*S/32.f, 28*S/32.f, Col);              // القاعدة
}

// فرشاة/أداة (siege 🛠️)
ROK2_ICON_COMMON(DrawWrench)
{
	C.Line(9*S/32.f, 23*S/32.f, 23*S/32.f, 9*S/32.f, 3.f*S/32.f, Col);   // المقبض
	C.Ring(23*S/32.f, 9*S/32.f, 5*S/32.f, 2.5f*S/32.f, Col);             // الفك
	C.Triangle(FVector2D(27*S/32.f, 4*S/32.f), FVector2D(29*S/32.f, 9*S/32.f), FVector2D(24*S/32.f, 7*S/32.f), Col);
}

#undef ROK2_ICON_COMMON

// ---------------------------------------------------------------------------
// URok2IconLibrary
// ---------------------------------------------------------------------------

typedef void (*FRok2IconDrawFn)(FRok2IconCanvas&, float, FColor);

static FRok2IconDrawFn GIconDrawFnFor(const FString& Id)
{
	static TMap<FString, FRok2IconDrawFn> Map;
	if (Map.Num() == 0)
	{
		Map.Add(TEXT("food"),       &DrawFood);
		Map.Add(TEXT("wood"),       &DrawWood);
		Map.Add(TEXT("wood_log"),   &DrawWood);
		Map.Add(TEXT("stone"),      &DrawStone);
		Map.Add(TEXT("gold"),       &DrawGold);
		Map.Add(TEXT("gems"),       &DrawGems);
		Map.Add(TEXT("ap"),         &DrawAp);
		Map.Add(TEXT("build"),      &DrawBuild);
		Map.Add(TEXT("hammer"),     &DrawBuild);
		Map.Add(TEXT("sword"),      &DrawSword);
		Map.Add(TEXT("shield"),     &DrawShield);
		Map.Add(TEXT("helmet"),     &DrawHelmet);
		Map.Add(TEXT("commanders"), &DrawHelmet);
		Map.Add(TEXT("bag"),        &DrawBag);
		Map.Add(TEXT("items"),      &DrawBag);
		Map.Add(TEXT("banner"),     &DrawBanner);
		Map.Add(TEXT("events"),     &DrawBanner);
		Map.Add(TEXT("scroll"),     &DrawScroll);
		Map.Add(TEXT("reports"),    &DrawScroll);
		Map.Add(TEXT("map"),        &DrawMap);
		Map.Add(TEXT("edit"),       &DrawEdit);
		Map.Add(TEXT("bell"),       &DrawBell);
		Map.Add(TEXT("lock"),       &DrawLock);
		Map.Add(TEXT("calendar"),   &DrawCalendar);
		Map.Add(TEXT("hourglass"),  &DrawHourglass);
		Map.Add(TEXT("queue"),      &DrawHourglass);
		Map.Add(TEXT("flask"),      &DrawFlask);
		Map.Add(TEXT("research"),   &DrawFlask);
		Map.Add(TEXT("cross"),      &DrawCross);
		Map.Add(TEXT("hospital"),   &DrawCross);
		Map.Add(TEXT("scout"),      &DrawScout);
		Map.Add(TEXT("close"),      &DrawClose);
		Map.Add(TEXT("star"),       &DrawStar);
		Map.Add(TEXT("skull"),      &DrawSkull);
		Map.Add(TEXT("blood"),      &DrawBlood);
		Map.Add(TEXT("bandage"),    &DrawBandage);
		Map.Add(TEXT("trophy"),     &DrawTrophy);
		Map.Add(TEXT("handshake"),  &DrawHandshake);
		Map.Add(TEXT("alliance"),   &DrawShield);
		Map.Add(TEXT("refresh"),    &DrawRefresh);
		Map.Add(TEXT("gift"),       &DrawGift);
		Map.Add(TEXT("chests"),     &DrawGift);
		Map.Add(TEXT("wheat"),      &DrawWheat);
		Map.Add(TEXT("farm"),       &DrawWheat);
		Map.Add(TEXT("box"),        &DrawBox);
		Map.Add(TEXT("storehouse"), &DrawBox);
		Map.Add(TEXT("mail"),       &DrawMail);
		Map.Add(TEXT("courier"),    &DrawMail);
		Map.Add(TEXT("cart"),       &DrawCart);
		Map.Add(TEXT("shop"),       &DrawCart);
		Map.Add(TEXT("horse"),      &DrawHorse);
		Map.Add(TEXT("stable"),     &DrawHorse);
		Map.Add(TEXT("bow"),        &DrawBow);
		Map.Add(TEXT("archery"),    &DrawBow);
		Map.Add(TEXT("tent"),       &DrawTent);
		Map.Add(TEXT("scout_camp"), &DrawTent);
		Map.Add(TEXT("tower"),      &DrawTower);
		Map.Add(TEXT("watchtower"), &DrawTower);
		Map.Add(TEXT("castle"),     &DrawCastle);
		Map.Add(TEXT("city_hall"),  &DrawCastle);
		Map.Add(TEXT("bricks"),     &DrawBricks);
		Map.Add(TEXT("wall"),       &DrawBricks);
		Map.Add(TEXT("rock"),       &DrawRock);
		Map.Add(TEXT("quarry"),     &DrawRock);
		Map.Add(TEXT("beer"),       &DrawBeer);
		Map.Add(TEXT("tavern"),     &DrawBeer);
		Map.Add(TEXT("scale"),      &DrawScale);
		Map.Add(TEXT("trading"),    &DrawScale);
		Map.Add(TEXT("crown"),      &DrawCrown);
		Map.Add(TEXT("throne"),     &DrawCrown);
		Map.Add(TEXT("builder"),    &DrawBuilder);
		Map.Add(TEXT("conn"),       &DrawConn);
		Map.Add(TEXT("speedup"),    &DrawSpeedup);
		Map.Add(TEXT("governor"),   &DrawGovernor);
		Map.Add(TEXT("stats"),      &DrawStats);
		Map.Add(TEXT("move"),       &DrawMove);
		Map.Add(TEXT("sparkle"),    &DrawSparkle);
		Map.Add(TEXT("passive"),    &DrawSparkle);
		Map.Add(TEXT("combat"),     &DrawCombat);
		Map.Add(TEXT("ring"),       &DrawRing);
		Map.Add(TEXT("accessory"),  &DrawRing);
		Map.Add(TEXT("boots"),      &DrawBoots);
		Map.Add(TEXT("arrow"),      &DrawArrowRight);
		Map.Add(TEXT("skillup"),    &DrawSkillUp);
		Map.Add(TEXT("pickaxe"),    &DrawPickaxe);
		Map.Add(TEXT("goldmine"),   &DrawPickaxe);
		Map.Add(TEXT("lumber"),     &DrawWood);
		Map.Add(TEXT("clock"),      &DrawClock);
		Map.Add(TEXT("time"),       &DrawClock);
		Map.Add(TEXT("art"),        &DrawArt);
		Map.Add(TEXT("decor"),      &DrawArt);
		Map.Add(TEXT("monument"),   &DrawMonument);
		Map.Add(TEXT("wrench"),     &DrawWrench);
		Map.Add(TEXT("siege"),      &DrawWrench);
		Map.Add(TEXT("academy"),    &DrawFlask);
		Map.Add(TEXT("barracks"),   &DrawSword);
		Map.Add(TEXT("train"),      &DrawSword);
		Map.Add(TEXT("heal"),       &DrawCross);
		Map.Add(TEXT("upgrade"),    &DrawBuild);
		Map.Add(TEXT("march"),      &DrawSword);
		Map.Add(TEXT("attack"),     &DrawSword);
		Map.Add(TEXT("defense"),    &DrawShield);
		Map.Add(TEXT("zone"),       &DrawMap);
		Map.Add(TEXT("kingdom"),    &DrawCrown);
		Map.Add(TEXT("vip"),        &DrawCrown);
		Map.Add(TEXT("level"),      &DrawStar);
		Map.Add(TEXT("xp"),         &DrawStar);
	}
	const FRok2IconDrawFn* Found = Map.Find(Id);
	return Found ? *Found : nullptr;
}

URok2IconLibrary* URok2IconLibrary::Get()
{
	static URok2IconLibrary* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2IconLibrary>();
		Instance->AddToRoot();
	}
	Instance->BuildCatalog();
	return Instance;
}

void URok2IconLibrary::BuildCatalog()
{
	if (bCatalogBuilt) return;
	bCatalogBuilt = true;

	// المعرّفات الأساسية المدعومة — تُستخدم من الاختبار البنيوي للتحقق من التغطية
	KnownIds = {
		TEXT("food"), TEXT("wood"), TEXT("stone"), TEXT("gold"), TEXT("gems"), TEXT("ap"),
		TEXT("build"), TEXT("sword"), TEXT("shield"), TEXT("helmet"), TEXT("bag"), TEXT("banner"),
		TEXT("scroll"), TEXT("map"), TEXT("edit"), TEXT("bell"), TEXT("lock"), TEXT("calendar"),
		TEXT("hourglass"), TEXT("flask"), TEXT("cross"), TEXT("scout"), TEXT("close"), TEXT("star"),
		TEXT("skull"), TEXT("blood"), TEXT("bandage"), TEXT("trophy"), TEXT("handshake"), TEXT("refresh"),
		TEXT("gift"), TEXT("wheat"), TEXT("box"), TEXT("mail"), TEXT("cart"), TEXT("horse"),
		TEXT("bow"), TEXT("tent"), TEXT("tower"), TEXT("castle"), TEXT("bricks"), TEXT("rock"),
		TEXT("beer"), TEXT("scale"), TEXT("crown"), TEXT("builder"), TEXT("conn"), TEXT("speedup"),
		TEXT("governor"), TEXT("stats"), TEXT("move"), TEXT("sparkle"), TEXT("combat"), TEXT("ring"),
		TEXT("boots"), TEXT("arrow"), TEXT("skillup"), TEXT("pickaxe"), TEXT("clock"), TEXT("art"),
		TEXT("monument"), TEXT("wrench")
	};
}

bool URok2IconLibrary::HasIcon(const FString& IconId) const
{
	return GIconDrawFnFor(IconId) != nullptr;
}

TArray<FString> URok2IconLibrary::GetIconIds() const
{
	return KnownIds;
}

UTexture2D* URok2IconLibrary::RenderIcon(const FString& IconId, int32 Size)
{
	FRok2IconDrawFn Fn = GIconDrawFnFor(IconId);
	if (!Fn) return nullptr;

	Size = FMath::Clamp(Size, 16, 128);

	FRok2IconCanvas Canvas;
	Canvas.W = Size;
	Canvas.H = Size;
	Canvas.Clear();

	// نرسم بالعاجي دائماً — التلوين النهائي عبر صبغة الـ FSlateBrush
	Fn(Canvas, (float)Size, Rok2IconPalette::Ivory);

	UTexture2D* Tex = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Tex) return nullptr;

#if WITH_EDITORONLY_DATA
	Tex->MipGenSettings = TMGS_NoMipmaps;
#endif
	Tex->SRGB = true;
	Tex->Filter = TF_Bilinear;

	void* MipData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, Canvas.Px.GetData(), Size * Size * sizeof(FColor));
	Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
	Tex->UpdateResource();

	Tex->AddToRoot(); // الأيقونة مخبأة مدى حياة الجلسة — لا GC
	return Tex;
}

FSlateBrush URok2IconLibrary::GetBrush(const FString& IconId, float Size, FLinearColor Tint)
{
	BuildCatalog();

	const int32 TexSize = FMath::RoundToInt(Size <= 24.f ? 32 : Size <= 32.f ? 32 : 64);
	const FString Key = FString::Printf(TEXT("%s@%d"), *IconId, TexSize);

	UTexture2D* Tex = Cache.FindRef(Key);
	if (!Tex)
	{
		Tex = RenderIcon(IconId, TexSize);
		if (Tex)
		{
			Cache.Add(Key, Tex);
		}
		else
		{
			UE_LOG(LogRok2Icons, Verbose, TEXT("Unknown icon id '%s' — empty tinted brush returned"), *IconId);
		}
	}

	FSlateBrush Brush;
	if (Tex)
	{
		Brush.SetResourceObject(Tex);
	}
	Brush.ImageSize = FVector2D(Size, Size);
	Brush.TintColor = FSlateColor(Tint);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	return Brush;
}

UWidget* URok2IconLibrary::MakeIconImage(UWidgetTree* Tree, const FString& IconId, float Size, FLinearColor Tint)
{
	if (!Tree) return nullptr;
	UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	Img->SetBrush(URok2IconLibrary::Get()->GetBrush(IconId, Size, Tint));
	Img->SetDesiredSizeOverride(FVector2D(Size, Size));
	return Img;
}

UWidget* URok2IconLibrary::MakeIconLabel(UWidgetTree* Tree, const FString& IconId, const FText& Label, FLinearColor Color, int32 FontSize)
{
	if (!Tree) return nullptr;
	UHorizontalBox* Box = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UImage* Img = Cast<UImage>(MakeIconImage(Tree, IconId, (float)ERok2IconSize::Small * (FontSize >= 14 ? 1.f : 0.85f), Color));
	if (Img)
	{
		UHorizontalBoxSlot* IconSlot = Box->AddChildToHorizontalBox(Img);
		IconSlot->SetPadding(FMargin(0, 0, 4, 0));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	UTextBlock* Txt = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Txt->SetText(Label);
	Txt->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo F = Txt->GetFont();
	F.Size = FontSize;
	Txt->SetFont(F);
	UHorizontalBoxSlot* TxtSlot = Box->AddChildToHorizontalBox(Txt);
	TxtSlot->SetVerticalAlignment(VAlign_Center);

	return Box;
}

FSlateBrush URok2IconLibrary::BrushFromArtAssets(const FString& IconId, float Size, FLinearColor Tint)
{
	return URok2IconLibrary::Get()->GetBrush(IconId, Size, Tint);
}
