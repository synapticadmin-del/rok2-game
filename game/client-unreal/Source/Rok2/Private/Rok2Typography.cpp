// Copyright ROK2. Unified Arabic/Latin typography system (P6-T2).

#include "Rok2Typography.h"
#include "Rok2Accessibility.h"
#include "Engine/Font.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Type, Log, All);

namespace Rok2TypeWeight
{
	const FName Black(TEXT("Black"));
	const FName Bold(TEXT("Bold"));
	const FName Regular(TEXT("Regular"));
}

namespace
{
	/**
	 * خط المحرك الافتراضي (Roboto) لا يضمن كل الأوزان بنفس الأسماء، وطلب
	 * Typeface غير موجود يجعل Slate يقع على أول وجه في الأصل — فيضيع التمييز
	 * بين عنوان ونص بصمت. لذلك عند العمل على خط المحرك نقصر الأوزان على
	 * Regular/Bold الموجودَين يقيناً: Black ⇒ Bold.
	 */
	FName ClampWeightForEngineFont(const FName Weight)
	{
		return (Weight == Rok2TypeWeight::Black) ? Rok2TypeWeight::Bold : Weight;
	}

	/**
	 * ملف الوجه لكل (وجه، وزن). Aref Ruqaa وCinzel لا يوفّران Black، فيُربط
	 * الوزن الأثقل بملف Bold — تمييز حقيقي مع صدق في المصدر، بدل اختراع وزن
	 * ليس في الخط.
	 */
	const TCHAR* FaceFileFor(ERok2Face Face, const FName Weight)
	{
		const bool bBlack = (Weight == Rok2TypeWeight::Black);
		const bool bBold = (Weight == Rok2TypeWeight::Bold) || bBlack;

		switch (Face)
		{
		case ERok2Face::Display:
			return bBold ? TEXT("ArefRuqaa-Bold") : TEXT("ArefRuqaa-Regular");
		case ERok2Face::Numeric:
			return bBold ? TEXT("Cinzel-Bold") : TEXT("Cinzel-Regular");
		case ERok2Face::Ui:
		default:
			return bBlack ? TEXT("Cairo-Black") : (bBold ? TEXT("Cairo-Bold") : TEXT("Cairo-Regular"));
		}
	}

	/** مسار أصل FontFace — /Game/Fonts/Faces/<name>.<name> كاصطلاح بقية الأصول. */
	FString FaceAssetPath(const TCHAR* FileName)
	{
		return FString::Printf(TEXT("/Game/Fonts/Faces/%s.%s"), FileName, FileName);
	}
}

URok2Typography* URok2Typography::Get()
{
	static URok2Typography* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2Typography>();
		Instance->AddToRoot();
	}
	return Instance;
}

FString URok2Typography::FaceAssetName(ERok2Face Face)
{
	switch (Face)
	{
	case ERok2Face::Display: return TEXT("Rok2Display");
	case ERok2Face::Numeric: return TEXT("Rok2Numeric");
	case ERok2Face::Ui:
	default:                 return TEXT("Rok2Ui");
	}
}

FString URok2Typography::FacePackagePath(ERok2Face Face)
{
	// الوجه المنطقي يُبنى من ثلاثة أصول FontFace لا من أصل Font واحد (انظر
	// ResolveFace)؛ يبقى هذا المسار معرّفاً للوجه في التوثيق والاختبار، ويشير
	// إلى وجه الوزن العادي وهو ما يقع عليه Slate عند طلب وزن غائب.
	return FaceAssetPath(FaceFileFor(Face, Rok2TypeWeight::Regular));
}

TSharedPtr<const FCompositeFont> URok2Typography::ResolveFace(ERok2Face Face)
{
	const uint8 Key = static_cast<uint8>(Face);

	if (TSharedPtr<FStandaloneCompositeFont>* Cached = FaceCache.Find(Key))
	{
		return *Cached;
	}
	if (FaceMisses.Contains(Key))
	{
		return nullptr;	// حاولنا وفشلنا — لا نكرر التحميل
	}

	// الأوزان الثلاثة بأسمائها الحرفية: `WeightOf` يطلبها بالاسم، وغياب اسم
	// يجعل Slate يقع على أول وجه في القائمة (Regular) بلا شكوى.
	static const FName Weights[] = { Rok2TypeWeight::Regular, Rok2TypeWeight::Bold, Rok2TypeWeight::Black };

	FTypeface Typeface;
	TArray<UObject*> LoadedFaces;
	for (const FName Weight : Weights)
	{
		const TCHAR* FileName = FaceFileFor(Face, Weight);
		UObject* FaceAsset = LoadObject<UObject>(nullptr, *FaceAssetPath(FileName));
		if (!FaceAsset)
		{
			continue;
		}
		LoadedFaces.Add(FaceAsset);

		FTypefaceEntry& Entry = Typeface.Fonts.AddDefaulted_GetRef();
		Entry.Name = Weight;
		Entry.Font = FFontData(FaceAsset);
	}

	if (Typeface.Fonts.IsEmpty())
	{
		// غير مستورد بعد — نبقى على خط المحرك بنفس الحجم والوزن (fallback)
		FaceMisses.Add(Key);
		UE_LOG(LogRok2Type, Verbose,
			TEXT("Typography face '%s' not imported yet (%s) — engine default font stays active"),
			*FaceAssetName(Face), *FacePackagePath(Face));
		return nullptr;
	}

	TSharedRef<FStandaloneCompositeFont> Composite = MakeShared<FStandaloneCompositeFont>();
	Composite->DefaultTypeface = Typeface;

	// Cinzel لاتيني بحت، وAref Ruqaa يغطي العربية دون اللاتينية الكاملة. فوجه
	// الاحتياط هو Cairo الذي يحمل الاثنين: رقمٌ عربي داخل عنوان أو محرف عربي
	// في دور رقمي يُرسم بمحرف حقيقي بدل مربّع، وهو ما كان NotoNaskh يفعله في
	// المسار الاحتياطي القديم.
	if (Face != ERok2Face::Ui)
	{
		if (UObject* UiFallback = LoadObject<UObject>(nullptr, *FaceAssetPath(TEXT("Cairo-Regular"))))
		{
			LoadedFaces.Add(UiFallback);
			FTypefaceEntry& FallbackEntry = Composite->FallbackTypeface.Typeface.Fonts.AddDefaulted_GetRef();
			FallbackEntry.Name = Rok2TypeWeight::Regular;
			FallbackEntry.Font = FFontData(UiFallback);
		}
	}

	for (UObject* Asset : LoadedFaces)
	{
		FaceAssets.Add(Asset);
	}

	FaceCache.Add(Key, Composite);
	UE_LOG(LogRok2Type, Log, TEXT("Typography face '%s' built from %d FontFace asset(s)"),
		*FaceAssetName(Face), Typeface.Fonts.Num());
	return Composite;
}

bool URok2Typography::HasFace(ERok2Face Face)
{
	return ResolveFace(Face).IsValid();
}

ERok2Face URok2Typography::FaceOf(ERok2TextRole Role)
{
	switch (Role)
	{
	// العناوين الكبرى بالوجه الفخم
	case ERok2TextRole::Display:
		return ERok2Face::Display;

	// الأرقام والمؤقّتات بالوجه الرقمي — تمييز الرقم عن النص هو جوهر البند
	case ERok2TextRole::Numeric:
	case ERok2TextRole::Timer:
		return ERok2Face::Numeric;

	// بقية الواجهة بالوجه العربي الواضح
	case ERok2TextRole::Title:
	case ERok2TextRole::Subtitle:
	case ERok2TextRole::TitleCompact:
	case ERok2TextRole::CardTitle:
	case ERok2TextRole::Body:
	case ERok2TextRole::BodySmall:
	case ERok2TextRole::Button:
	case ERok2TextRole::Caption:
	case ERok2TextRole::Micro:
	default:
		return ERok2Face::Ui;
	}
}

float URok2Typography::SizeOf(ERok2TextRole Role)
{
	// P7-T7: كل الأحجام مضروبة في المقياس المركزي ولا تتجاوز السلم.
	// كل حالة تُنهي المسار بـ break؛ كانت الحالات بلا break فتسقط جميعها إلى
	// default وتعود بحجم Body، أي أن سلم الأحجام كله كان معطلاً في التشغيل.
	float Raw = Rok2TypeScale::Body;
	switch (Role)
	{
	case ERok2TextRole::Display:      Raw = Rok2TypeScale::Display;   break;
	case ERok2TextRole::Title:        Raw = Rok2TypeScale::Title;     break;
	case ERok2TextRole::Subtitle:     Raw = Rok2TypeScale::Subtitle;  break;
	case ERok2TextRole::Button:       Raw = Rok2TypeScale::Button;    break;
	case ERok2TextRole::Caption:      Raw = Rok2TypeScale::Caption;   break;
	case ERok2TextRole::Micro:        Raw = Rok2TypeScale::Micro;     break;
	case ERok2TextRole::TitleCompact: Raw = Rok2TypeScale::Compact;   break;
	case ERok2TextRole::CardTitle:    Raw = Rok2TypeScale::Button;    break;
	case ERok2TextRole::BodySmall:    Raw = Rok2TypeScale::Compact;   break;
	// المورد والمؤقّت على الدرجة الكثيفة
	case ERok2TextRole::Numeric:      Raw = Rok2TypeScale::Compact;   break;
	case ERok2TextRole::Timer:        Raw = Rok2TypeScale::Compact;   break;
	case ERok2TextRole::Body:
	default:                          Raw = Rok2TypeScale::Body;      break;
	}
	return FMath::Clamp(Raw * URok2Accessibility::Get()->GetUiScale(), Rok2TypeScale::Min, Rok2TypeScale::Max);
}

FName URok2Typography::WeightOf(ERok2TextRole Role)
{
	switch (Role)
	{
	// العنوان الأكبر وحده يستحق أثقل وزن
	case ERok2TextRole::Display:
		return Rok2TypeWeight::Black;

	// العناوين والأزرار والأرقام البارزة — عريض ليقرأ من لمحة
	case ERok2TextRole::Title:
	case ERok2TextRole::Subtitle:
	case ERok2TextRole::TitleCompact:
	case ERok2TextRole::CardTitle:
	case ERok2TextRole::Button:
	case ERok2TextRole::Numeric:
		return Rok2TypeWeight::Bold;

	// النص والمؤقّت والحاشية — عادي، فالمؤقّت يتغيّر كل ثانية ولا يجب أن يصرخ
	case ERok2TextRole::Body:
	case ERok2TextRole::BodySmall:
	case ERok2TextRole::Timer:
	case ERok2TextRole::Caption:
	case ERok2TextRole::Micro:
	default:
		return Rok2TypeWeight::Regular;
	}
}

FSlateFontInfo URok2Typography::Font(ERok2TextRole Role)
{
	const float Size = SizeOf(Role);
	const FName Weight = WeightOf(Role);

	if (TSharedPtr<const FCompositeFont> Face = Get()->ResolveFace(FaceOf(Role)))
	{
		return FSlateFontInfo(Face, Size, Weight);
	}

	// الخط الاحتياطي هو خط المحرك المركّب (FCoreStyle::GetDefaultFont)، وليس
	// Roboto وحده كما قد يبدو: `FLegacySlateFontInfoCache` يسجّل فيه وجهاً
	// فرعياً للعربية على `NotoNaskhArabicUI-Regular.ttf` يغطي نطاقات Arabic
	// وArabicPresentationForms* بلا شرط ثقافة (Arabic ليست داخل `if (GIsEditor)`
	// كما اليابانية). فالنص العربي يُرسم بمحارف حقيقية مع تشكيل HarfBuzz، ولا
	// تظهر مربّعات — والملف مُحزَّم في APK فعلاً (تحقّقت من
	// Manifest_UFSFiles_Android.txt).
	//
	// ما يفقده هذا المسار هو الطابع فقط: Naskh وجه واحد بوزن Regular، فلا فرق
	// بصري بين Display وBody غير الحجم. يبقى هذا المسار حياً لأن حذف أصول
	// Content/Fonts/Faces يجب أن يُخفت الواجهة لا أن يمحو نصها.
	return FCoreStyle::GetDefaultFontStyle(ClampWeightForEngineFont(Weight), Size);
}

void URok2Typography::ApplyFont(UTextBlock* Text, ERok2TextRole Role)
{
	if (!Text)
	{
		return;
	}
	Text->SetFont(Font(Role));
}

FSlateFontInfo URok2Typography::FontSized(ERok2Face Face, float Size, bool bBold)
{
	const FName Weight = bBold ? Rok2TypeWeight::Bold : Rok2TypeWeight::Regular;
	const float Clamped = FMath::Clamp(Size, Rok2TypeScale::Min, Rok2TypeScale::Max);

	if (TSharedPtr<const FCompositeFont> Loaded = Get()->ResolveFace(Face))
	{
		return FSlateFontInfo(Loaded, Clamped, Weight);
	}
	return FCoreStyle::GetDefaultFontStyle(ClampWeightForEngineFont(Weight), Clamped);
}

TArray<ERok2TextRole> URok2Typography::AllRoles()
{
	return {
		ERok2TextRole::Display,
		ERok2TextRole::Title,
		ERok2TextRole::Subtitle,
		ERok2TextRole::TitleCompact,
		ERok2TextRole::CardTitle,
		ERok2TextRole::Body,
		ERok2TextRole::BodySmall,
		ERok2TextRole::Button,
		ERok2TextRole::Caption,
		ERok2TextRole::Micro,
		ERok2TextRole::Numeric,
		ERok2TextRole::Timer
	};
}
