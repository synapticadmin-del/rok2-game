// Copyright Rok2. World renderer impl.

#include "Rok2WorldRenderer.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2ProceduralAssets.h"
#include "Rok2ArtAssets.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

ARok2WorldRenderer::ARok2WorldRenderer()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> GroundFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (GroundFinder.Succeeded()) GroundMesh = GroundFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CityFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CityFinder.Succeeded()) CityMesh = CityFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PassFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (PassFinder.Succeeded()) PassMesh = PassFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> NodeFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (NodeFinder.Succeeded()) NodeMesh = NodeFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MarchFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (MarchFinder.Succeeded()) MarchMesh = MarchFinder.Object;

	GroundHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GroundHISM"));
	GroundHISM->SetupAttachment(RootComponent);
	if (GroundMesh) GroundHISM->SetStaticMesh(GroundMesh);

	CityHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CityHISM"));
	CityHISM->SetupAttachment(RootComponent);
	if (CityMesh) CityHISM->SetStaticMesh(CityMesh);

	PassHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("PassHISM"));
	PassHISM->SetupAttachment(RootComponent);
	if (PassMesh) PassHISM->SetStaticMesh(PassMesh);

	ResourceNodeHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ResourceNodeHISM"));
	ResourceNodeHISM->SetupAttachment(RootComponent);
	if (NodeMesh) ResourceNodeHISM->SetStaticMesh(NodeMesh);

	BarbarianNodeHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BarbarianNodeHISM"));
	BarbarianNodeHISM->SetupAttachment(RootComponent);
	if (NodeMesh) BarbarianNodeHISM->SetStaticMesh(NodeMesh);
}

void ARok2WorldRenderer::BeginPlay()
{
	Super::BeginPlay();

	URok2ProceduralAssets* Assets = URok2ProceduralAssets::Get();
	if (GroundHISM)
	{
		if (Assets) GroundHISM->SetMaterial(0, Assets->GetMaterial(ERok2MaterialType::GroundTile));
		GroundHISM->ClearInstances();
		// Spawn ground plane centered at Z = -100 with scale Z = 0.1 to avoid Z-fighting
		GroundHISM->AddInstance(FTransform(FRotator::ZeroRotator, FVector(60000.f, 60000.f, -100.f), FVector(1200.f, 1200.f, 0.1f)));
	}

	if (CityHISM && Assets) CityHISM->SetMaterial(0, Assets->GetMaterial(ERok2MaterialType::City));
	if (PassHISM && Assets) PassHISM->SetMaterial(0, Assets->GetMaterial(ERok2MaterialType::Pass));
	if (ResourceNodeHISM && Assets) ResourceNodeHISM->SetMaterial(0, Assets->GetMaterial(ERok2MaterialType::Node));
	if (BarbarianNodeHISM && Assets) BarbarianNodeHISM->SetMaterial(0, Assets->GetMaterial(ERok2MaterialType::Pass));

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(GetWorld()->GetAuthGameMode()))
	{
		Api = GM->Api;
		if (Api)
		{
			Api->OnWorldSnapshot.AddDynamic(this, &ARok2WorldRenderer::OnWorldSnapshotHandler);
		}
	}
}

void ARok2WorldRenderer::OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot)
{
	RefreshFromApi();
}

void ARok2WorldRenderer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Animate marches — موضع حسب التقدم الزمني + اتجاه الأيقونة ناحية الهدف (P1-T3)
	int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
	for (const FRok2MarchEntity& M : CurrentMarches)
	{
		AActor** ActorPtr = SpawnedMarches.Find(M.Id);
		if (ActorPtr && *ActorPtr)
		{
			AActor* MarchActor = *ActorPtr;
			float Progress = 1.f;
			if (M.EtaMs > M.StartMs)
			{
				Progress = FMath::Clamp((float)(NowMs - M.StartMs) / (float)(M.EtaMs - M.StartMs), 0.f, 1.f);
			}
			FVector Start(M.FromX * WorldToUnrealScale, M.FromY * WorldToUnrealScale, MarchZ);
			FVector End(M.ToX * WorldToUnrealScale, M.ToY * WorldToUnrealScale, MarchZ);
			MarchActor->SetActorLocation(FMath::Lerp(Start, End, Progress));

			// وجّه الأيقونة ناحية الهدف (محور الـ Cone الافتراضي +X)
			if (!Start.Equals(End))
			{
				MarchActor->SetActorRotation((End - Start).Rotation());
			}
		}
	}

	RefreshTimer += DeltaSeconds;
	if (RefreshTimer > 3.f)
	{
		RefreshTimer = 0.f;
		if (Api && Api->HasPlayer()) RefreshFromApi();
	}
}

void ARok2WorldRenderer::ClearActors()
{
	for (AActor* A : SpawnedActors)
	{
		if (A) A->Destroy();
	}
	SpawnedActors.Empty();
	bArtHillsSpawned = false; // P2-T7: تُعاد زراعة المرتفعات مع إعادة الرسم
}

AActor* ARok2WorldRenderer::SpawnMarkerActor(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color)
{
	if (!Mesh) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* SM = GetWorld()->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator, P);
	if (!SM) return nullptr;
#if WITH_EDITOR
	SM->SetActorLabel(Label);
#endif
	UStaticMeshComponent* MeshC = SM->GetStaticMeshComponent();
	if (MeshC)
	{
		MeshC->SetStaticMesh(Mesh);
		MeshC->SetMobility(EComponentMobility::Movable);
		UMaterialInstanceDynamic* Dyn = MeshC->CreateAndSetMaterialInstanceDynamic(0);
		if (Dyn) Dyn->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return SM;
}

void ARok2WorldRenderer::SpawnMarker(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color)
{
	if (AActor* SM = SpawnMarkerActor(Mesh, Loc, Label, Color))
	{
		SpawnedActors.Add(SM);
	}
}

void ARok2WorldRenderer::RefreshFromApi()
{
	if (!Api) return;

	ClearActors();

	const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

	FVector CamLoc = FVector::ZeroVector;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (AActor* VT = PC->GetViewTarget())
		{
			CamLoc = VT->GetActorLocation();
		}
	}
	// LOD distance handling for a 1200x1200 world grid at 100 scale (10,000,000 units range)
	float RenderDistanceSq = 1000000.f * 1000000.f;

	if (CityHISM) CityHISM->ClearInstances();
	if (PassHISM) PassHISM->ClearInstances();
	if (ResourceNodeHISM) ResourceNodeHISM->ClearInstances();
	if (BarbarianNodeHISM) BarbarianNodeHISM->ClearInstances();

	for (const FRok2CityEntity& C : W.Cities)
	{
		FVector Loc(C.X * WorldToUnrealScale, C.Y * WorldToUnrealScale, CityZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P2-T7: قلعة KayKit حقيقية إن توفرت (زرقاء لي/لتحالفي، حمراء للأعداء) — وإلا الـ HISM المكعّب
		UStaticMesh* CastleMesh = nullptr;
		float CastleScale = 2.2f;
		const bool bMineOrAlly = (C.PlayerId == Api->GetPlayer().Id) ||
			(!Api->GetPlayer().AllianceId.IsEmpty() && C.AllianceId == Api->GetPlayer().AllianceId);
		if (URok2ArtAssets* Art = URok2ArtAssets::Get())
		{
			CastleMesh = Art->LoadMesh(bMineOrAlly ? TEXT("city_hall") : TEXT("city_enemy"));
		}
		if (CastleMesh)
		{
			SpawnMarker(CastleMesh, Loc, FString::Printf(TEXT("CityArt_%s"), *C.PlayerId), FLinearColor::White);
			if (AActor* Last = SpawnedActors.Num() ? SpawnedActors.Last() : nullptr)
			{
				Last->SetActorScale3D(FVector(CastleScale));
			}
		}
		else if (CityHISM)
		{
			CityHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector::OneVector));
		}
	}

	for (const FRok2PassEntity& P : W.Passes)
	{
		FVector Loc(P.X * WorldToUnrealScale, P.Y * WorldToUnrealScale, PassZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		if (PassHISM)
		{
			PassHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(1.5f, 1.5f, 1.5f)));
		}
	}

	for (const FRok2NodeEntity& N : W.Nodes)
	{
		FVector Loc(N.X * WorldToUnrealScale, N.Y * WorldToUnrealScale, NodeZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		if (N.Kind == TEXT("barb"))
		{
			if (BarbarianNodeHISM) BarbarianNodeHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(0.8f, 0.8f, 0.8f)));
		}
		else
		{
			if (ResourceNodeHISM) ResourceNodeHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(0.8f, 0.8f, 0.8f)));
		}
	}

	// P2-T7: مرتفعات KayKit عند زوايا مناطق Zone1 (مرة واحدة — علامات حدود بصرية)
	if (!bArtHillsSpawned)
	{
		if (URok2ArtAssets* Art = URok2ArtAssets::Get())
		{
			if (UStaticMesh* Hills = Art->LoadMesh(TEXT("hills")))
			{
				for (const FRok2CityEntity& C : W.Cities)
				{
					// تلال خلف كل مدينة كنقطة ارتكاز بصرية للتضاريس
					FVector HillLoc(C.X * WorldToUnrealScale + 350.f, C.Y * WorldToUnrealScale + 350.f, CityZ);
					SpawnMarker(Hills, HillLoc, TEXT("ArtHill"), FLinearColor::White);
					if (AActor* Last = SpawnedActors.Num() ? SpawnedActors.Last() : nullptr)
					{
						Last->SetActorScale3D(FVector(2.0f));
					}
				}
				bArtHillsSpawned = true;
			}
		}
	}

	CurrentMarches = W.Marches;
	TSet<FString> ActiveMarches;
	const FString MyId = Api->GetPlayer().Id;
	const FString MyAlliance = Api->GetPlayer().AllianceId;
	for (const FRok2MarchEntity& M : CurrentMarches)
	{
		// لا نرسم المسيرات المنتهية
		if (M.State == TEXT("returned") || M.State == TEXT("cancelled") || M.State == TEXT("arrived")) continue;

		ActiveMarches.Add(M.Id);
		if (!SpawnedMarches.Contains(M.Id))
		{
			// اللون: أخضر لي، أزرق لتحالفي، أحمر للأعداء؛ العائدة رمادية-زرقاء
			FLinearColor Col;
			if (M.OwnerPlayerId == MyId)
			{
				Col = FLinearColor(0.2f, 0.9f, 0.3f);
			}
			else if (!MyAlliance.IsEmpty() && M.AllianceId == MyAlliance)
			{
				Col = FLinearColor(0.2f, 0.5f, 1.0f);
			}
			else
			{
				Col = FLinearColor(0.95f, 0.2f, 0.15f);
			}
			if (M.State == TEXT("returning"))
			{
				Col = FLinearColor(0.55f, 0.65f, 0.75f);
			}

			AActor* NewMarch = SpawnMarkerActor(
				MarchMesh,
				FVector(M.FromX * WorldToUnrealScale, M.FromY * WorldToUnrealScale, MarchZ),
				FString::Printf(TEXT("March_%s"), *M.Id), Col);
			if (NewMarch)
			{
				NewMarch->SetActorScale3D(FVector(0.6f, 0.6f, 0.6f));
				SpawnedMarches.Add(M.Id, NewMarch);
			}
		}
	}
	
	TArray<FString> ToRemove;
	for (const auto& KV : SpawnedMarches)
	{
		if (!ActiveMarches.Contains(KV.Key))
		{
			if (KV.Value) KV.Value->Destroy();
			ToRemove.Add(KV.Key);
		}
	}
	for (const FString& K : ToRemove) SpawnedMarches.Remove(K);
}
