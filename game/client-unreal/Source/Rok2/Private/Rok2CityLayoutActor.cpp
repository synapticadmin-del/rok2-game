// Copyright ROK2. City layout manager actor (P5-T1 / P5-T2) — implementation.

#include "Rok2CityLayoutActor.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2HexWallActor.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"

ARok2CityLayoutActor::ARok2CityLayoutActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GroundHexes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GroundHexes"));
	GroundHexes->SetupAttachment(Root);
	GroundHexes->SetMobility(EComponentMobility::Movable);

	HighlightHexes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HighlightHexes"));
	HighlightHexes->SetupAttachment(Root);
	HighlightHexes->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		HexTileMesh = CylinderFinder.Object;
	}
}

void ARok2CityLayoutActor::BeginPlay()
{
	Super::BeginPlay();

	if (HexTileMesh)
	{
		GroundHexes->SetStaticMesh(HexTileMesh);
		HighlightHexes->SetStaticMesh(HexTileMesh);
	}
	GroundHexes->SetCastShadow(false);
	HighlightHexes->SetCastShadow(false);
	if (GroundMaterial) GroundHexes->SetMaterial(0, GroundMaterial);

	BuildGround();
	SpawnWall();

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(GetWorld()->GetAuthGameMode()))
	{
		Api = GM->Api;
		if (Api)
		{
			Api->OnCityLoaded.AddDynamic(this, &ARok2CityLayoutActor::OnCityLoadedHandler);
		}
	}
}

int32 ARok2CityLayoutActor::RadiusForCityHallLevel(int32 L)
{
	if (L >= 22) return 12;
	if (L >= 16) return 10;
	if (L >= 10) return 8;
	if (L >= 5) return 6;
	return 5;
}

void ARok2CityLayoutActor::BuildGround()
{
	if (!HexTileMesh) return;
	GroundHexes->ClearInstances();

	// أرضية = سداسية ممتلئة داخل نصف القطر + 1 حلقة حديقة خارجية
	for (const FRok2HexCell& Cell : URok2HexGrid::FilledHexagon(CityRadiusCells + 1))
	{
		const FVector Loc = URok2HexGrid::HexToWorld(Cell, HexSize);
		const float D = FMath::Max3(FMath::Abs(Cell.Q), FMath::Abs(Cell.R), FMath::Abs(Cell.S()));
		const bool bGarden = (D == CityRadiusCells + 1); // حلقة خارجية = حديقة/عشب
		const float TileScale = (HexSize / 50.f) * 0.98f;
		FTransform T(FRotator::ZeroRotator, Loc + FVector(0, 0, bGarden ? 0.5f : 1.f), FVector(TileScale, TileScale, 0.08f));
		GroundHexes->AddInstance(T, true);
	}
}

void ARok2CityLayoutActor::SpawnWall()
{
	if (Wall) { Wall->Destroy(); Wall = nullptr; }
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Wall = GetWorld()->SpawnActor<ARok2HexWallActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Wall)
	{
		Wall->CityRadiusCells = CityRadiusCells;
		Wall->HexSize = HexSize;
		// P5-T2: تمرير حضارة اللاعب للسور
		if (Api && Api->HasPlayer())
		{
			Wall->CivId = Api->GetPlayer().Civ;
		}
		Wall->RebuildWall();
	}
}

void ARok2CityLayoutActor::ClearBuildings()
{
	for (auto& KV : Buildings)
	{
		if (KV.Value) KV.Value->Destroy();
	}
	Buildings.Empty();
}

FRok2HexCell ARok2CityLayoutActor::DefaultCellForIndex(int32 Index, int32 Total) const
{
	// توزيع حلقي لولبي حول المركز كتخطيط افتراضي
	if (Index == 0) return FRok2HexCell(0, 0); // city_hall في المركز
	const TArray<FRok2HexCell> Spiral = URok2HexGrid::FilledHexagon(CityRadiusCells - 1);
	int32 SpiralIdx = FMath::Min(Index, Spiral.Num() - 1);
	return Spiral[SpiralIdx];
}

bool ARok2CityLayoutActor::CanPlaceAt(const FString& BuildingId, ERok2Footprint Footprint, const FRok2HexCell& Cell, const FString& IgnoreBuildingId) const
{
	// احسب خلايا البصمة المقترحة
	TArray<FRok2HexCell> Cells;
	const int32 R = (Footprint == ERok2Footprint::Small) ? 0 : (Footprint == ERok2Footprint::Medium) ? 1 : 2;
	if (R == 0) Cells.Add(Cell);
	else if (R == 1) Cells = URok2HexGrid::Flower(Cell);
	else for (const FRok2HexCell& C : URok2HexGrid::FilledHexagon(2)) Cells.Add(FRok2HexCell(Cell.Q + C.Q, Cell.R + C.R));

	for (const FRok2HexCell& C : Cells)
	{
		// داخل السور؟
		if (!URok2HexGrid::IsInsideRadius(C, CityRadiusCells - 1)) return false;
		// تراكب مع مبنى آخر؟
		for (const auto& KV : Buildings)
		{
			if (KV.Key == IgnoreBuildingId || !KV.Value) continue;
			if (KV.Value->Occupies(C)) return false;
		}
	}
	return true;
}

void ARok2CityLayoutActor::OnCityLoadedHandler(const FRok2City& City)
{
	// حدّث نصف القطر من مستوى قاعة المدينة
	if (Api)
	{
		const TMap<FString, int32>& B = Api->GetBuildings();
		const int32* CH = B.Find(TEXT("city_hall"));
		const int32 NewRadius = RadiusForCityHallLevel(CH ? *CH : 1);
		if (NewRadius != CityRadiusCells)
		{
			CityRadiusCells = NewRadius;
			BuildGround();
			SpawnWall();
		}
	}
	RebuildFromApi();
}

void ARok2CityLayoutActor::RebuildFromApi()
{
	if (!Api) return;
	ClearBuildings();

	// P5-T2: جلب حضارة اللاعب لتمريرها لكل مبنى
	const FString PlayerCiv = Api->HasPlayer() ? Api->GetPlayer().Civ : TEXT("rome");

	const TMap<FString, int32>& ApiBuildings = Api->GetBuildings();
	TArray<FString> Order = { TEXT("city_hall"), TEXT("farm"), TEXT("lumber_mill"), TEXT("quarry"), TEXT("goldmine"), TEXT("barracks"), TEXT("stable"), TEXT("archery_range"), TEXT("hospital"), TEXT("wall"), TEXT("storehouse") };

	int32 idx = 0;
	for (const FString& Id : Order)
	{
		const int32 Level = ApiBuildings.Contains(Id) ? ApiBuildings[Id] : 1;
		const FRok2HexCell Cell = DefaultCellForIndex(idx, Order.Num());

		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ARok2BuildingActor* B = GetWorld()->SpawnActor<ARok2BuildingActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		if (B)
		{
			B->SetupWithCiv(Id, Level, Cell, HexSize, PlayerCiv);

			// أصل فني إن توفر (KayKit) — وإلا يبقى placeholder
			if (URok2ArtAssets* Art = URok2ArtAssets::Get())
			{
				if (UStaticMesh* ArtMesh = Art->LoadMesh(Id))
				{
					B->Mesh->SetStaticMesh(ArtMesh);
					B->bUsingArtAsset = true;
					for (const FRok2ArtEntry& E : Art->GetCatalog())
					{
						if (E.Id == Id) { B->Mesh->SetWorldScale3D(FVector(E.Scale)); break; }
					}
					// إعادة تطبيق الثيم للأصل الفني (تلوين خفيف)
					B->ApplyCivTheme();
				}
			}

			B->OnClicked.AddDynamic(this, &ARok2CityLayoutActor::OnAnyBuildingClicked);
			Buildings.Add(Id, B);
		}
		idx++;
	}
}

void ARok2CityLayoutActor::SetEditMode(bool bEnable)
{
	bEditMode = bEnable;
	if (!bEnable)
	{
		HighlightHexes->ClearInstances();
		SaveLayoutToServer();
	}
}

bool ARok2CityLayoutActor::TryMoveBuilding(const FString& BuildingId, const FRok2HexCell& NewCell)
{
	ARok2BuildingActor** Found = Buildings.Find(BuildingId);
	if (!Found || !*Found) return false;
	ARok2BuildingActor* B = *Found;
	if (B->bIsStatic) return false; // city_hall ثابت

	if (!CanPlaceAt(BuildingId, B->Footprint, NewCell, BuildingId)) return false;

	B->AnchorCell = NewCell;
	B->SetActorLocation(URok2HexGrid::HexToWorld(NewCell, HexSize));
	OnLayoutChanged.Broadcast();
	return true;
}

void ARok2CityLayoutActor::RotateBuilding(const FString& BuildingId)
{
	ARok2BuildingActor** Found = Buildings.Find(BuildingId);
	if (!Found || !*Found) return;
	ARok2BuildingActor* B = *Found;
	B->RotationSteps = (B->RotationSteps + 1) % 6;
	B->SetActorRotation(FRotator(0.f, B->RotationSteps * 60.f, 0.f));
	OnLayoutChanged.Broadcast();
}

TArray<FRok2BuildingPlacement> ARok2CityLayoutActor::GetLayoutPlacements() const
{
	TArray<FRok2BuildingPlacement> Out;
	for (const auto& KV : Buildings)
	{
		if (!KV.Value) continue;
		FRok2BuildingPlacement P;
		P.BuildingId = KV.Key;
		P.Q = KV.Value->AnchorCell.Q;
		P.R = KV.Value->AnchorCell.R;
		P.RotationSteps = KV.Value->RotationSteps;
		Out.Add(P);
	}
	return Out;
}

void ARok2CityLayoutActor::SaveLayoutToServer()
{
	// ملاحظة للـ backend: endpoint مقترح POST /v1/city/layout يستقبل GetLayoutPlacements().
	// يُربط عبر URok2Api عند إضافة الدالة (خارج نطاق ملف العميل هذا — يُسجَّل في PLAN).
	OnLayoutChanged.Broadcast();
}

void ARok2CityLayoutActor::OnAnyBuildingClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	if (!TouchedActor) return;
	const ARok2BuildingActor* B = Cast<ARok2BuildingActor>(TouchedActor);
	if (B)
	{
		OnBuildingPicked.Broadcast(B->BuildingId);
	}
}

void ARok2CityLayoutActor::RefreshHighlights(const FString& ForBuildingId)
{
	HighlightHexes->ClearInstances();
	ARok2BuildingActor** Found = Buildings.Find(ForBuildingId);
	if (!Found || !*Found) return;
	ARok2BuildingActor* B = *Found;

	// أظهر كل الخلايا الصالحة للوضع (إبراز أخضر/أحمر يُدار بالمادة عبر CustomData لاحقاً)
	for (const FRok2HexCell& Cell : URok2HexGrid::FilledHexagon(CityRadiusCells - 1))
	{
		if (CanPlaceAt(ForBuildingId, B->Footprint, Cell, ForBuildingId))
		{
			const FVector Loc = URok2HexGrid::HexToWorld(Cell, HexSize);
			const float TileScale = (HexSize / 50.f) * 0.5f;
			FTransform T(FRotator::ZeroRotator, Loc + FVector(0, 0, 6.f), FVector(TileScale, TileScale, 0.05f));
			HighlightHexes->AddInstance(T, true);
		}
	}
}
