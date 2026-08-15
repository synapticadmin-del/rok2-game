// Copyright ROK2. Hexagonal city wall actor (P5-T1 / P5-T2) — implementation.

#include "Rok2HexWallActor.h"
#include "Rok2CivThemes.h"
#include "Rok2ProceduralAssets.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

ARok2HexWallActor::ARok2HexWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	WallSegments = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallSegments"));
	WallSegments->SetupAttachment(Root);
	WallSegments->SetMobility(EComponentMobility::Movable);

	Towers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Towers"));
	Towers->SetupAttachment(Root);
	Towers->SetMobility(EComponentMobility::Movable);

	Gate = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Gate"));
	Gate->SetupAttachment(Root);
	Gate->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		WallSegmentMesh = CubeFinder.Object;
		TowerMesh = CubeFinder.Object;
		GateMesh = CubeFinder.Object;
	}
}

void ARok2HexWallActor::BeginPlay()
{
	Super::BeginPlay();

	if (WallSegmentMesh) WallSegments->SetStaticMesh(WallSegmentMesh);
	if (TowerMesh) Towers->SetStaticMesh(TowerMesh);
	if (GateMesh) Gate->SetStaticMesh(GateMesh);

	WallSegments->SetCastShadow(false);
	Towers->SetCastShadow(false);
	Gate->SetCastShadow(false);

	RebuildWall();
}

ERok2WallTier ARok2HexWallActor::TierForLevel(int32 Level)
{
	if (Level >= 16) return ERok2WallTier::Citadel;
	if (Level >= 10) return ERok2WallTier::Fortified;
	if (Level >= 5) return ERok2WallTier::Stone;
	return ERok2WallTier::Wood;
}

float ARok2HexWallActor::TierScale() const
{
	switch (TierForLevel(WallLevel))
	{
	case ERok2WallTier::Wood: return 0.8f;
	case ERok2WallTier::Stone: return 1.0f;
	case ERok2WallTier::Fortified: return 1.2f;
	case ERok2WallTier::Citadel: return 1.4f;
	}
	return 1.f;
}

void ARok2HexWallActor::SetWallState(int32 InLevel, float InDurability01)
{
	WallLevel = FMath::Max(1, InLevel);
	Durability01 = FMath::Clamp(InDurability01, 0.f, 1.f);
	RebuildWall();
}

void ARok2HexWallActor::RebuildWall()
{
	if (!WallSegmentMesh || !TowerMesh || !GateMesh) return;

	WallSegments->ClearInstances();
	Towers->ClearInstances();
	Gate->ClearInstances();

	// خلايا السور = الحلقة عند نصف القطر
	const TArray<FRok2HexCell> WallCells = URok2HexGrid::Ring(CityRadiusCells);
	if (WallCells.Num() == 0) return;

	const float S = TierScale();
	const float SegW = HexSize * 1.9f * S;          // عرض كتلة السور
	const float SegH = 120.f * S;                   // ارتفاع السور
	const float GateDirDeg = -90.f;                 // البوابة على الضلع الجنوبي (قابلة للضبط)

	// زوايا السور (6) = خلايا الاتجاهات الستة عند نصف القطر
	const TArray<FRok2HexCell>& Dirs = URok2HexGrid::Directions();

	for (int32 i = 0; i < WallCells.Num(); ++i)
	{
		const FRok2HexCell& Cell = WallCells[i];
		const FVector Loc = URok2HexGrid::HexToWorld(Cell, HexSize);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Loc.Y, Loc.X));

		// هل هذه الخلية بوابة؟ (أقرب خلية لاتجاه البوابة)
		const bool bIsGate = FMath::Abs(FMath::Abs(AngleDeg - GateDirDeg)) < (180.f / WallCells.Num());

		// هل هي زاوية (برج)؟ كل خلية تقع عند مضاعفات 60°
		const float Mod = FMath::Fmod(FMath::Abs(AngleDeg), 60.f);
		const bool bIsCorner = (Mod < 8.f || Mod > 52.f);

		FRotator SegRot = FRotator(0.f, AngleDeg + 90.f, 0.f); // السور مماسّي للحلقة

		if (bIsGate)
		{
			// البوابة: كتلة أعرض وأطول قليلاً بلون مميز
			FTransform T(SegRot, Loc + FVector(0, 0, SegH * 0.5f), FVector(SegW * 0.012f, SegW * 0.012f, SegH * 0.014f));
			Gate->AddInstance(T, true);
		}
		else if (bIsCorner)
		{
			// برج عند الزاوية: أضيق وأطول
			FTransform T(FRotator::ZeroRotator, Loc + FVector(0, 0, SegH * 0.75f), FVector(SegW * 0.009f, SegW * 0.009f, SegH * 0.022f));
			Towers->AddInstance(T, true);
		}
		else
		{
			// مقطع سور عادي
			FTransform T(SegRot, Loc + FVector(0, 0, SegH * 0.5f), FVector(SegW * 0.011f, SegW * 0.011f, SegH * 0.010f));
			WallSegments->AddInstance(T, true);
		}
	}

	ApplyTierMaterials();
	ApplyCivTheme();
}

void ARok2HexWallActor::ApplyTierMaterials()
{
	// الوسم يبقى ليتمكن المستوى/الاختبارات من قراءة المرحلة والضرر.
	// التلوين الفعلي يجري في ApplyCivTheme التي تُنادى بعدها مباشرة.
	const float Damage = 1.f - Durability01;
	Tags.Add(FName(*FString::Printf(TEXT("wall_tier_%d"), (int32)TierForLevel(WallLevel))));
	Tags.Add(FName(*FString::Printf(TEXT("wall_damage_%d"), FMath::RoundToInt(Damage * 4.f))));
}

void ARok2HexWallActor::ApplyCivTheme()
{
	URok2CivThemes* Themes = URok2CivThemes::Get();
	if (!Themes) return;

	URok2ProceduralAssets* Mats = URok2ProceduralAssets::Get();
	if (!Mats) return;

	const FRok2CivTheme& Theme = Themes->GetTheme(CivId);

	// السور: لون الحضارة الأساسي (حجر/خشب)
	// البوابة: لون الحضارة الثانوي (ذهب/زخارف)
	// الأبراج: لون التمييز (Accent)
	//
	// المادة تُبنى فوق M_Rok2Base وهي مُعلَّمة used_with_instanced_static_meshes،
	// وإلا استبدلها المحرك بـ DefaultMaterial على HISM في بناء مُطبَّق.
	//
	// الضرر يُقرأ بصرياً: كلما نقصت المتانة مال اللون إلى الأحمر الداكن، فيرى
	// اللاعب حالة سوره من الخريطة قبل فتح أي لوحة.
	const float Damage = FMath::Clamp(1.f - Durability01, 0.f, 1.f);
	const FLinearColor DamageTint(0.42f, 0.13f, 0.10f);
	auto Weathered = [&](const FLinearColor& C)
	{
		return FMath::Lerp(C, DamageTint, Damage * 0.65f);
	};

	Mats->MakeTintedMaterialOn(WallSegments, 0, Weathered(Theme.Primary));
	Mats->MakeTintedMaterialOn(Gate, 0, Weathered(Theme.Secondary));
	Mats->MakeTintedMaterialOn(Towers, 0, Weathered(Theme.Accent));
}

void ARok2HexWallActor::OnWallCellClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	// لمس السور: يفتح بطاقة السور (ترقية/إصلاح) — ربط لاحقاً عبر CityWidget.
}
