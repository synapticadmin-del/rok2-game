// Copyright Rok2. Procedural assets impl.

#include "Rok2ProceduralAssets.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

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

	// Use DefaultLitMaterial which renders clean solid colors on ALL GPUs
	// WorldGridMaterial causes black-white noise on Intel HD 530
	BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"));
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	}
	if (!BaseMaterial)
	{
		// Ultimate fallback — guaranteed to exist
		BaseMaterial = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);
	}

	auto CreateDyn = [&](const FLinearColor& Color) -> UMaterialInstanceDynamic*
	{
		if (!BaseMaterial) return nullptr;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
		if (M)
		{
			// Set all possible color parameter names to guarantee color shows
			M->SetVectorParameterValue(TEXT("BaseColor"), Color);
			M->SetVectorParameterValue(TEXT("Base Color"), Color);
			M->SetVectorParameterValue(TEXT("Color"), Color);
			M->SetVectorParameterValue(TEXT("Tint"), Color);
			// Also set emissive for Unlit fallback visibility
			M->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 0.5f);
		}
		return M;
	};

	GroundTileMat = CreateDyn(FLinearColor(0.20f, 0.55f, 0.18f)); // Lush green grass
	BuildingMat   = CreateDyn(FLinearColor(0.40f, 0.50f, 0.70f)); // Royal blue stone
	PassMat       = CreateDyn(FLinearColor(0.95f, 0.55f, 0.15f)); // Amber gold pass
	NodeMat       = CreateDyn(FLinearColor(0.85f, 0.80f, 0.30f)); // Yellow resource node
	CityMat       = CreateDyn(FLinearColor(0.25f, 0.70f, 0.95f)); // Cyan city
	MountainMat   = CreateDyn(FLinearColor(0.35f, 0.30f, 0.25f)); // Dark mountain stone
	WaterMat      = CreateDyn(FLinearColor(0.10f, 0.35f, 0.65f)); // Deep blue water
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
		M->SetVectorParameterValue(TEXT("BaseColor"), Color);
		M->SetVectorParameterValue(TEXT("Base Color"), Color);
		M->SetVectorParameterValue(TEXT("Color"), Color);
		M->SetVectorParameterValue(TEXT("Tint"), Color);
		M->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 0.5f);
	}
	return M;
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
