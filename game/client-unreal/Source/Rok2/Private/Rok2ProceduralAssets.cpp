// Copyright Rok2. Procedural assets impl.

#include "Rok2ProceduralAssets.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Components/MeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Materials, Log, All);

const TCHAR* URok2ProceduralAssets::BaseMaterialPath()
{
	return TEXT("/Game/Art/Materials/M_Rok2Base.M_Rok2Base");
}

const TCHAR* URok2ProceduralAssets::UnlitMaterialPath()
{
	return TEXT("/Game/Art/Materials/M_Rok2Unlit.M_Rok2Unlit");
}

URok2ProceduralAssets* URok2ProceduralAssets::Get()
{
	static URok2ProceduralAssets* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2ProceduralAssets>();
		Instance->AddToRoot(); // prevent GC
		Instance->Init();
	}
	return Instance;
}

void URok2ProceduralAssets::Init()
{
	if (bInitialized) return;
	bInitialized = true;

	// مادة المشروع هي الوالد الوحيد الذي يملك البارامتر "Color". مواد المحرك
	// (DefaultMaterial / WorldGridMaterial) لا تملك أي VectorParameter، فالتلوين
	// عليها يفشل بصمت — لذلك نُحمّل مادتنا أولاً ونُبلّغ بوضوح عند غيابها.
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, BaseMaterialPath());
	UnlitMaterial = LoadObject<UMaterialInterface>(nullptr, UnlitMaterialPath());

	if (!BaseMaterial)
	{
		UE_LOG(LogRok2Materials, Error,
			TEXT("مادة المشروع %s غير موجودة — شغّل create_materials.py. الألوان لن تظهر مع مواد المحرك."),
			BaseMaterialPath());
		BaseMaterial = UnlitMaterial;
	}
	if (!UnlitMaterial)
	{
		UnlitMaterial = BaseMaterial;
	}
	if (!BaseMaterial)
	{
		BaseMaterial = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);
		UnlitMaterial = BaseMaterial;
	}

	GroundTileMat = CreateDynInternal(FLinearColor(0.20f, 0.55f, 0.18f)); // Lush green grass
	BuildingMat   = CreateDynInternal(FLinearColor(0.40f, 0.50f, 0.70f)); // Royal blue stone
	PassMat       = CreateDynInternal(FLinearColor(0.95f, 0.55f, 0.15f)); // Amber gold pass
	NodeMat       = CreateDynInternal(FLinearColor(0.85f, 0.80f, 0.30f)); // Yellow resource node
	CityMat       = CreateDynInternal(FLinearColor(0.25f, 0.70f, 0.95f)); // Cyan city
	MountainMat   = CreateDynInternal(FLinearColor(0.35f, 0.30f, 0.25f)); // Dark mountain stone
	WaterMat      = CreateDynInternal(FLinearColor(0.10f, 0.35f, 0.65f)); // Deep blue water
}

void URok2ProceduralAssets::EnsureInit()
{
	if (!bInitialized) Init();
}

UMaterialInstanceDynamic* URok2ProceduralAssets::CreateDynInternal(const FLinearColor& Color)
{
	if (!BaseMaterial) return nullptr;
	UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
	if (M)
	{
		M->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return M;
}

UMaterialInstanceDynamic* URok2ProceduralAssets::MakeTintedMaterialOn(UMeshComponent* Component, int32 ElementIndex, const FLinearColor& Color)
{
	if (!Component) return nullptr;
	EnsureInit();
	if (!BaseMaterial) return nullptr;

	UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(BaseMaterial, Component);
	if (!Dyn) return nullptr;

	Dyn->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(ElementIndex, Dyn);
	return Dyn;
}

FLinearColor URok2ProceduralAssets::SoftenTintForTexturedAsset(const FLinearColor& Color)
{
	// بارامتر لون الأصل المستورد يُضرب في النسيج، فلونٌ داكن مثل أساس اليابان
	// (#111111) يطفئ المجسم كله إلى أسود. نريد إيماءةً للحضارة لا استبدال النسيج:
	// نبدأ من الأبيض (= بلا تغيير) ونميل قليلاً نحو لون الحضارة، ثم نفرض حداً
	// أدنى للسطوع كي لا يعتم النسيج أبداً.
	const FLinearColor Hinted = FMath::Lerp(FLinearColor::White, Color, 0.30f);
	const float Luminance = 0.2126f * Hinted.R + 0.7152f * Hinted.G + 0.0722f * Hinted.B;
	const float MinLuminance = 0.72f;
	if (Luminance >= MinLuminance || Luminance <= KINDA_SMALL_NUMBER)
	{
		return FLinearColor(Hinted.R, Hinted.G, Hinted.B, 1.f);
	}

	const float Boost = MinLuminance / Luminance;
	return FLinearColor(
		FMath::Min(Hinted.R * Boost, 1.f),
		FMath::Min(Hinted.G * Boost, 1.f),
		FMath::Min(Hinted.B * Boost, 1.f),
		1.f);
}

UMaterialInstanceDynamic* URok2ProceduralAssets::TintExistingMaterialOn(UMeshComponent* Component, int32 ElementIndex, const FLinearColor& Color)
{
	if (!Component) return nullptr;

	UMaterialInterface* Existing = Component->GetMaterial(ElementIndex);
	if (!Existing) return nullptr;

	// مواد الأصول المستوردة (KayKit عبر glTF) تحمل نسيجاً حقيقياً؛ استبدالها
	// بمادة لون مسطّح يمحو كل التفاصيل. فنصبغ داخلها إن كانت تملك بارامتر لون،
	// وإلا نتركها كما هي — النسيج أهم من صبغة الحضارة.
	static const FName Candidates[] = {
		FName(TEXT("BaseColorFactor")),  // اسم بارامتر مستورد glTF
		FName(TEXT("Color")),
		FName(TEXT("BaseColor")),
		FName(TEXT("Tint"))
	};

	FLinearColor Ignored;
	FName Parameter = NAME_None;
	for (const FName& Candidate : Candidates)
	{
		if (Existing->GetVectorParameterValue(FMaterialParameterInfo(Candidate), Ignored))
		{
			Parameter = Candidate;
			break;
		}
	}
	if (Parameter.IsNone())
	{
		return nullptr;
	}

	// هنا CreateAndSetMaterialInstanceDynamic صحيح: الوالد هو مادة الأصل نفسها
	// وقد تحققنا أنها تملك البارامتر فعلاً.
	UMaterialInstanceDynamic* Dyn = Component->CreateAndSetMaterialInstanceDynamic(ElementIndex);
	if (Dyn)
	{
		Dyn->SetVectorParameterValue(Parameter, SoftenTintForTexturedAsset(Color));
	}
	return Dyn;
}

UMaterialInstanceDynamic* URok2ProceduralAssets::GetMaterial(ERok2MaterialType Type)
{
	EnsureInit();
	switch (Type)
	{
	case ERok2MaterialType::GroundTile: return GroundTileMat;
	case ERok2MaterialType::Building: return BuildingMat;
	case ERok2MaterialType::Pass: return PassMat;
	case ERok2MaterialType::Node: return NodeMat;
	case ERok2MaterialType::City: return CityMat;
	case ERok2MaterialType::Mountain: return MountainMat;
	case ERok2MaterialType::Water: return WaterMat;
	}
	return nullptr;
}

UMaterialInstanceDynamic* URok2ProceduralAssets::CreateTintedMaterial(const FLinearColor& Color)
{
	EnsureInit();
	return CreateDynInternal(Color);
}
