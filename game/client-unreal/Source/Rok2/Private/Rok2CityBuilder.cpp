#include "Rok2CityBuilder.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2BuildingDetailWidget.h"
#include "Rok2BlueprintLibrary.h"
#include "Rok2ProceduralAssets.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blueprint/UserWidget.h"

ARok2CityBuilder::ARok2CityBuilder()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	GroundTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GroundTiles"));
	GroundTiles->SetupAttachment(Root);
	GroundTiles->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (MeshFinder.Succeeded())
	{
		GroundTileMesh = MeshFinder.Object;
	}
}

void ARok2CityBuilder::BeginPlay()
{
	Super::BeginPlay();

	if (GroundTileMesh)
	{
		GroundTiles->SetStaticMesh(GroundTileMesh);
		UMaterialInterface* MatToUse = GroundMaterial;
		if (!MatToUse)
		{
			MatToUse = URok2ProceduralAssets::Get()->GetMaterial(ERok2MaterialType::GroundTile);
		}
		if (MatToUse) GroundTiles->SetMaterial(0, MatToUse);
		GroundTiles->SetCastShadow(false);
	}

	// Generate ground tile instances
	if (GroundTiles && GroundTileMesh)
	{
		GroundTiles->ClearInstances();
		int32 Half = GridSize / 2;
		for (int32 x = 0; x < GridSize; ++x)
		{
			for (int32 y = 0; y < GridSize; ++y)
			{
				FVector Loc((x - Half) * TileWorldSize, (y - Half) * TileWorldSize, 1.f);
				FTransform T(FQuat::Identity, Loc, FVector(TileWorldSize / 100.f, TileWorldSize / 100.f, 0.1f));
				GroundTiles->AddInstance(T, true);
			}
		}
	}

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(GetWorld()->GetAuthGameMode()))
	{
		Api = GM->Api;
		if (Api)
		{
			Api->OnCityLoaded.AddDynamic(this, &ARok2CityBuilder::OnCityLoadedHandler);
		}
	}
}

void ARok2CityBuilder::OnCityLoadedHandler(const FRok2City& City)
{
	Rebuild();
}

void ARok2CityBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshTimer += DeltaSeconds;
	if (RefreshTimer > 2.f)
	{
		RefreshTimer = 0.f;
		if (Api && Api->HasPlayer()) Rebuild();
	}
}

void ARok2CityBuilder::Rebuild()
{
	if (!Api) return;

	// Remove previous building actors
	for (auto& KV : SpawnedBuildings)
	{
		if (KV.Value) KV.Value->Destroy();
	}
	SpawnedBuildings.Empty();

	// Spawn placeholder per known building id arranged in a ring/grid
	const TArray<FString> Order = {
		TEXT("city_hall"), TEXT("farm"), TEXT("lumber_mill"), TEXT("quarry"), TEXT("goldmine"),
		TEXT("barracks"), TEXT("stable"), TEXT("archery_range"), TEXT("hospital"), TEXT("wall"), TEXT("storehouse")
	};
	int32 idx = 0;
	for (const FString& Id : Order)
	{
		int32 Level = Api->GetBuildings().Contains(Id) ? Api->GetBuildings()[Id] : 1;
		// place building
		float Angle = (float)idx / (float)Order.Num() * 2.f * PI;
		float Radius = (GridSize / 2 - 1) * TileWorldSize * 0.6f;
		FVector Loc(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 50.f);

		UStaticMesh* MeshToUse = GroundTileMesh;
		FVector Offset = FVector::ZeroVector;
		FRotator Rot = FRotator::ZeroRotator;
		for (const auto& V : BuildingVisuals)
		{
			if (V.BuildingId == Id && V.Mesh)
			{
				MeshToUse = V.Mesh;
				Offset = V.GridOffset;
				Rot = V.Rotation;
				break;
			}
		}

		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* SM = GetWorld()->SpawnActor<AStaticMeshActor>(Loc + Offset, Rot, P);
		if (SM)
		{
#if WITH_EDITOR
			SM->SetActorLabel(FString::Printf(TEXT("Bldg_%s_L%d"), *Id, Level));
#endif
			SM->Tags.Add(FName(*Id));
			SM->OnClicked.AddDynamic(this, &ARok2CityBuilder::OnBuildingClicked);

			UStaticMeshComponent* MeshC = SM->GetStaticMeshComponent();
			if (MeshC && MeshToUse)
			{
				MeshC->SetStaticMesh(MeshToUse);
				MeshC->SetWorldScale3D(FVector(0.8f, 0.8f, 1.f + Level * 0.1f));
				MeshC->SetMobility(EComponentMobility::Movable);
			}
		}
		SpawnedBuildings.Add(Id, SM);
		idx++;
	}
}

void ARok2CityBuilder::OnBuildingClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	if (Api && TouchedActor && TouchedActor->Tags.Num() > 0)
	{
		FString BId = TouchedActor->Tags[0].ToString();
		int32 Level = Api->GetBuildings().Contains(BId) ? Api->GetBuildings()[BId] : 1;
		if (URok2BuildingDetailWidget* DetailWidget = Cast<URok2BuildingDetailWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2BuildingDetailWidget::StaticClass())))
		{
			DetailWidget->SetupBuilding(Api, BId, Level);
			DetailWidget->AddToViewport(200);
		}
	}
}
