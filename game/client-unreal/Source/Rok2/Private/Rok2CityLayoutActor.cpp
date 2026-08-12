// Copyright ROK2. City layout manager actor (P5-T1 / P5-T2) — implementation.

#include "Rok2CityLayoutActor.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2HexWallActor.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Rok2CityLayoutSaveGame.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

namespace Rok2CityLayout
{
	static ERok2BuildingFacade FacadeFromWire(const FString& Value)
	{
		if (Value == TEXT("ceremonial")) return ERok2BuildingFacade::Ceremonial;
		if (Value == TEXT("fortified")) return ERok2BuildingFacade::Fortified;
		return ERok2BuildingFacade::Standard;
	}

	static FString FacadeToWire(ERok2BuildingFacade Value)
	{
		switch (Value)
		{
		case ERok2BuildingFacade::Ceremonial: return TEXT("ceremonial");
		case ERok2BuildingFacade::Fortified: return TEXT("fortified");
		default: return TEXT("standard");
		}
	}
}

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
	// يضمن المستوى الأول مساحة كافية للقلعة (بصمة 2) ولمناطق مدنية وعسكرية مفصولة.
	if (L >= 22) return 12;
	if (L >= 16) return 10;
	if (L >= 10) return 8;
	if (L >= 5) return 7;
	return 6;
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
	// P4-T7: إخفاء/إعادة تدوير بدل Destroy — المباني تُعاد بناؤها عند كل city upsert
	// (تحديثات الموارد الحية P1-T5)، فكان churn عالي التكرار. المبنى المخفي يُعاد
	// استخدامه عند إعادة الزرع بنفس المعرف وإلا يُدمّر فعلياً في نهاية RebuildFromApi.
	for (auto& KV : Buildings)
	{
		if (KV.Value)
		{
			KV.Value->SetActorHiddenInGame(true);
			KV.Value->SetActorTickEnabled(false);
		}
	}
	RecycledBuildings = Buildings;
	Buildings.Empty();
}

FRok2HexCell ARok2CityLayoutActor::DefaultCellForBuilding(const FString& BuildingId, int32 Index) const
{
	// تخطيط المناطق الافتراضي: قلعة مركزية، ثم نطاق مدني، ثم قطاعات جيش/اقتصاد.
	// الإحداثيات مختارة لتترك مسافات فاصلة بين البصمات المتوسطة والقلعة الكبيرة.
	static const TMap<FString, FRok2HexCell> ZonedCells = {
		{ TEXT("city_hall"), FRok2HexCell(0, 0) },
		{ TEXT("tavern"), FRok2HexCell(-3, 3) },
		{ TEXT("trading_post"), FRok2HexCell(3, -3) },
		{ TEXT("academy"), FRok2HexCell(-4, 4) },
		{ TEXT("alliance_center"), FRok2HexCell(4, -4) },
		{ TEXT("barracks"), FRok2HexCell(-4, 0) },
		{ TEXT("stable"), FRok2HexCell(0, -4) },
		{ TEXT("archery_range"), FRok2HexCell(4, 0) },
		{ TEXT("siege_workshop"), FRok2HexCell(0, 4) },
		{ TEXT("hospital"), FRok2HexCell(0, 4) },
		{ TEXT("farm"), FRok2HexCell(-3, 2) },
		{ TEXT("lumber_mill"), FRok2HexCell(2, -3) },
		{ TEXT("quarry"), FRok2HexCell(3, -1) },
		{ TEXT("goldmine"), FRok2HexCell(-1, 3) },
		{ TEXT("storehouse"), FRok2HexCell(3, 2) },
		// مركز بوابة رمزي: لا يحل محل السور السداسي الفعلي.
		{ TEXT("wall"), FRok2HexCell(4, -2) },
		{ TEXT("builders_hut"), FRok2HexCell(-2, -1) }
	};

	if (const FRok2HexCell* Zoned = ZonedCells.Find(BuildingId))
	{
		return *Zoned;
	}

	// احتياط منظم لمبانٍ مستقبلية: حلقات بعيدة عن مركز القلعة، لا عودة إلى الخلية (0,0).
	const TArray<FRok2HexCell> Ring = URok2HexGrid::Ring(FMath::Max(3, CityRadiusCells - 2));
	return Ring.IsValidIndex(Index % Ring.Num()) ? Ring[Index % Ring.Num()] : FRok2HexCell(CityRadiusCells - 2, 0);
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
	TMap<FString, FRok2BuildingPlacement> SavedPlacements;
	const FRok2City& AuthoritativeCity = Api->GetCity();
	if (AuthoritativeCity.LayoutPlacements.Num() > 0)
	{
		for (const FRok2CityLayoutPlacement& ServerPlacement : AuthoritativeCity.LayoutPlacements)
		{
			FRok2BuildingPlacement Placement;
			Placement.BuildingId = ServerPlacement.BuildingId;
			Placement.Q = ServerPlacement.Q;
			Placement.R = ServerPlacement.R;
			Placement.RotationSteps = ServerPlacement.RotationSteps;
			Placement.Facade = Rok2CityLayout::FacadeFromWire(ServerPlacement.Facade);
			SavedPlacements.Add(Placement.BuildingId, Placement);
		}
	}
	else
	{
		// ترحيل لطيف: التخزين المحلي احتياطي لجلسات لم تحفظ بعد نسخة خادمية.
		SavedPlacements = LoadLocalLayout();
	}
	TArray<FString> Order = {
		TEXT("city_hall"), TEXT("tavern"), TEXT("academy"), TEXT("trading_post"), TEXT("alliance_center"),
		TEXT("farm"), TEXT("lumber_mill"), TEXT("quarry"), TEXT("goldmine"), TEXT("storehouse"),
		TEXT("barracks"), TEXT("stable"), TEXT("archery_range"), TEXT("siege_workshop"), TEXT("hospital"),
		TEXT("wall"), TEXT("builders_hut")
	};

	int32 idx = 0;
	for (const FString& Id : Order)
	{
		const int32* OwnedLevel = ApiBuildings.Find(Id);
		// لا نزرع مبنى مستقبلياً على أنه مملوك؛ الخادم لا يقبل حفظ مبنى غير مملوك.
		if (!OwnedLevel || *OwnedLevel <= 0) continue;
		const int32 Level = *OwnedLevel;
		const FRok2BuildingPlacement* Saved = SavedPlacements.Find(Id);
		const FRok2HexCell Cell = Saved ? FRok2HexCell(Saved->Q, Saved->R) : DefaultCellForBuilding(Id, idx);

		// P4-T7: إعادة استخدام المبنى المخفي بنفس المعرف إن وُجد (تجديد سريع)، وإلا زرع جديد
		ARok2BuildingActor* B = nullptr;
		bool bReused = false;
		if (ARok2BuildingActor** Recycled = RecycledBuildings.Find(Id))
		{
			B = *Recycled;
			RecycledBuildings.Remove(Id);
			bReused = true;
			B->SetActorHiddenInGame(false);
			B->SetActorTickEnabled(true);
		}
		else
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			B = GetWorld()->SpawnActor<ARok2BuildingActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		}
		if (B)
		{
				B->SetupWithCiv(Id, Level, Cell, HexSize, PlayerCiv);
				if (Saved)
				{
					B->RotationSteps = FMath::Abs(Saved->RotationSteps) % 6;
					B->SetActorRotation(FRotator(0.f, B->RotationSteps * 60.f, 0.f));
					B->SetFacade(Saved->Facade);
				}

				// P5-T6: حركة بناء عند الزرع الجديد فقط (المُعاد استخدامه يظهر مباشرة — لا تشويش)
			if (!bReused) B->PlayBuildAnimation();

			// أصل فني إن توفر (KayKit) — وإلا يبقى placeholder
			if (URok2ArtAssets* Art = URok2ArtAssets::Get())
			{
				if (UStaticMesh* ArtMesh = Art->LoadMesh(Id))
				{
					B->Mesh->SetStaticMesh(ArtMesh);
					for (const FRok2ArtEntry& E : Art->GetCatalog())
					{
						if (E.Id == Id) { B->Mesh->SetWorldScale3D(FVector(E.Scale)); break; }
					}
					// يضبط راية الأصل الفني ثم يعيد تطبيق الثيم (تلوين خفيف)
					B->MarkUsingArtAsset();
				}
			}

							// المبنى قد يأتي من pool بعد إعادة بناء؛ أزل الربط السابق قبل إعادة إضافته.
				B->OnClicked.RemoveDynamic(this, &ARok2CityLayoutActor::OnAnyBuildingClicked);
				B->OnClicked.AddDynamic(this, &ARok2CityLayoutActor::OnAnyBuildingClicked);

			Buildings.Add(Id, B);
		}
		idx++;
	}

	// P4-T7: مبانٍ مُعاد تدويرها ولم تُستعمل (اختفت من حالة الـ API) — تدمير فعلي
	for (auto& KV : RecycledBuildings)
	{
		if (KV.Value) KV.Value->Destroy();
	}
	RecycledBuildings.Empty();
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

bool ARok2CityLayoutActor::SetBuildingFacade(const FString& BuildingId, ERok2BuildingFacade NewFacade)
{
	ARok2BuildingActor** Found = Buildings.Find(BuildingId);
	if (!Found || !*Found || (*Found)->bIsStatic)
	{
		return false;
	}

	(*Found)->SetFacade(NewFacade);
	OnLayoutChanged.Broadcast();
	return true;
}

TArray<FRok2BuildingPlacement> ARok2CityLayoutActor::GetLayoutPlacements() const
{
	TArray<FRok2BuildingPlacement> Out;
	for (const auto& KV : Buildings)
	{
		if (!KV.Value) continue;
		// طبقة العرض لا تضيف مباني مستقبلية إلى الحمولة السلطوية.
		if (Api && Api->HasPlayer() && !Api->GetBuildings().Contains(KV.Key)) continue;
		FRok2BuildingPlacement P;
		P.BuildingId = KV.Key;
		P.Q = KV.Value->AnchorCell.Q;
		P.R = KV.Value->AnchorCell.R;
		P.RotationSteps = KV.Value->RotationSteps;
		P.Facade = KV.Value->Facade;
		Out.Add(P);
	}
	return Out;
}

FString ARok2CityLayoutActor::GetLocalLayoutSlotName() const
{
	const FString PlayerId = Api && Api->HasPlayer() ? Api->GetPlayer().Id : TEXT("guest");
	return FString::Printf(TEXT("Rok2_CityLayout_%s"), *PlayerId);
}

TMap<FString, FRok2BuildingPlacement> ARok2CityLayoutActor::LoadLocalLayout() const
{
	TMap<FString, FRok2BuildingPlacement> Out;
	if (!Api || !Api->HasPlayer())
	{
		return Out;
	}

	if (URok2CityLayoutSaveGame* Save = Cast<URok2CityLayoutSaveGame>(UGameplayStatics::LoadGameFromSlot(GetLocalLayoutSlotName(), 0)))
	{
		if (Save->SchemaVersion == 1 && Save->PlayerId == Api->GetPlayer().Id)
		{
			for (const FRok2BuildingPlacement& Placement : Save->Placements)
			{
				Out.Add(Placement.BuildingId, Placement);
			}
		}
	}
	return Out;
}

void ARok2CityLayoutActor::SaveAcceptedLayoutLocally(const TArray<FRok2BuildingPlacement>& Placements)
{
	URok2CityLayoutSaveGame* Save = Cast<URok2CityLayoutSaveGame>(UGameplayStatics::CreateSaveGameObject(URok2CityLayoutSaveGame::StaticClass()));
	if (!Save || !Api || !Api->HasPlayer())
	{
		return;
	}

	Save->SchemaVersion = 1;
	Save->PlayerId = Api->GetPlayer().Id;
	Save->Placements = Placements;
	UGameplayStatics::SaveGameToSlot(Save, GetLocalLayoutSlotName(), 0);
}

void ARok2CityLayoutActor::SaveLayoutToServer()
{
	if (!Api || !Api->HasPlayer())
	{
		return;
	}

	const TArray<FRok2BuildingPlacement> Placements = GetLayoutPlacements();
	TArray<FRok2CityLayoutPlacement> Payload;
	for (const FRok2BuildingPlacement& Placement : Placements)
	{
		FRok2CityLayoutPlacement WirePlacement;
		WirePlacement.BuildingId = Placement.BuildingId;
		WirePlacement.Q = Placement.Q;
		WirePlacement.R = Placement.R;
		WirePlacement.RotationSteps = FMath::Abs(Placement.RotationSteps) % 6;
		WirePlacement.Facade = Rok2CityLayout::FacadeToWire(Placement.Facade);
		Payload.Add(MoveTemp(WirePlacement));
	}

	TWeakObjectPtr<ARok2CityLayoutActor> WeakThis(this);
	Api->SaveCityLayout(Payload, [WeakThis, Placements](bool bAccepted)
	{
		if (bAccepted && WeakThis.IsValid())
		{
			WeakThis->SaveAcceptedLayoutLocally(Placements);
		}
	});

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
