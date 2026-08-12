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
	// نفس اصطلاح URok2ArtAssets::EditorPackagePath — /Game/<dir>/<name>.<name>
	const FString Name = FaceAssetName(Face);
	return FString::Printf(TEXT("/Game/Fonts/%s.%s"), *Name, *Name);
}

UFont* URok2Typography::ResolveFace(ERok2Face Face)
{
	const uint8 Key = static_cast<uint8>(Face);

	if (UFont** Cached = FaceCache.Find(Key))
	{
		return *Cached;
	}
	if (FaceMisses.Contains(Key))
	{
		return nullptr;	// حاولنا وفشلنا — لا نكرر التحميل
	}

	UFont* Loaded = LoadObject<UFont>(nullptr, *FacePackagePath(Face));
	if (Loaded)
	{
		FaceCache.Add(Key, Loaded);
		UE_LOG(LogRok2Type, Log, TEXT("Typography face '%s' loaded"), *FaceAssetName(Face));
		return Loaded;
	}

	// غير مستورد بعد — نبقى على خط المحرك بنفس الحجم والوزن (fallback)
	FaceMisses.Add(Key);
	UE_LOG(LogRok2Type, Verbose,
		TEXT("Typography face '%s' not imported yet (%s) — engine default font stays active"),
		*FaceAssetName(Face), *FacePackagePath(Face));
	return nullptr;
}

bool URok2Typography::HasFace(ERok2Face Face)
{
	return ResolveFace(Face) != nullptr;
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
	// P7-T7: كل الأحجام مضروبة في المقياس المركزي ولا تتجاوز السلم
	float Raw = Rok2TypeScale::Body;
	switch (Role)
	{
	case ERok2TextRole::Display:      Raw = Rok2TypeScale::Display;
	case ERok2TextRole::Title:        Raw = Rok2TypeScale::Title;
	case ERok2TextRole::Subtitle:     Raw = Rok2TypeScale::Subtitle;
	case ERok2TextRole::Button:       Raw = Rok2TypeScale::Button;
	case ERok2TextRole::Caption:      Raw = Rok2TypeScale::Caption;
	case ERok2TextRole::Micro:        Raw = Rok2TypeScale::Micro;
	case ERok2TextRole::TitleCompact: Raw = Rok2TypeScale::Compact;
	case ERok2TextRole::CardTitle:    Raw = Rok2TypeScale::Button;
	case ERok2TextRole::BodySmall:    Raw = Rok2TypeScale::Compact;
	// المورد والمؤقّت على الدرجة الكثيفة
	case ERok2TextRole::Numeric:      Raw = Rok2TypeScale::Compact;
	case ERok2TextRole::Timer:        Raw = Rok2TypeScale::Compact;
	case ERok2TextRole::Body:
	default:                          Raw = Rok2TypeScale::Body;
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

	if (UFont* Face = Get()->ResolveFace(FaceOf(Role)))
	{
		return FSlateFontInfo(Face, Size, Weight);
	}

	// fallback: خط المحرك بنفس الحجم والوزن — السلم البصري يعمل بلا أصول
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

	if (UFont* Loaded = Get()->ResolveFace(Face))
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
