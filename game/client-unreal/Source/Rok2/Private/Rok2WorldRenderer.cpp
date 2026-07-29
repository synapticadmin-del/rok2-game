// Copyright Rok2. World renderer impl.

#include "Rok2WorldRenderer.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2ProceduralAssets.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Rok2FogOfWar.h"
#include "Rok2AudioManager.h"
#include "Rok2Perf.h"
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

	// P5-T5: تحديث الكشافة من نظام ضباب الحرب
	if (URok2FogOfWar* Fog = URok2FogOfWar::Get())
	{
		const int64 NowMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
		Fog->UpdateScouts(NowMs);
	}

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
	// P4-T7: إعادة للمسبح بدل Destroy — تختفي كلفة spawn/GC عند كل تحديث (كل 3ث).
	// التلال لا تُمسّ (throttled إلى تغيّر مجموعة المدن فعلياً — انظر ArtHillsKey).
	URok2Perf* Perf = URok2Perf::Get(this);
	for (AActor* A : SpawnedActors)
	{
		if (AStaticMeshActor* SM = Cast<AStaticMeshActor>(A))
		{
			if (Perf) Perf->ReleaseMarkerActor(SM);
			else if (A) A->Destroy();
		}
		else if (A) A->Destroy();
	}
	SpawnedActors.Empty();
	// bArtHillsSpawned لا يُعاد ضبطه هنا بعد الآن — التلال تُدار بمفتاح محتوى
	// (ArtHillsKey) وتبقى ثابتة عبر التحديثات ما لم تتغير مجموعة المدن.
}

AActor* ARok2WorldRenderer::SpawnMarkerActor(UStaticMesh* Mesh, const FVector& Loc, const FString& Label, const FLinearColor& Color)
{
	if (!Mesh) return nullptr;
	// P4-T7: من مسبح الأداء بدل SpawnActor الجديد (إعادة استخدام actor مخفي).
	AStaticMeshActor* SM = nullptr;
	if (URok2Perf* Perf = URok2Perf::Get(this))
	{
		SM = Perf->AcquireMarkerActor(GetWorld());
	}
	else
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SM = GetWorld()->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator, P);
	}
	if (!SM) return nullptr;
	SM->SetActorLocation(Loc);
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
	// P4-T7: مسافة الرسم من subsystem الأداء (كانت ثابتاً سحرياً 1000000² — LOD موحد قابل للضبط).
	float RenderDistanceSq = 1000000.f * 1000000.f;
	if (URok2Perf* Perf = URok2Perf::Get(this))
	{
		RenderDistanceSq = Perf->WorldRenderDistanceSq();
	}

	if (CityHISM) CityHISM->ClearInstances();
	if (PassHISM) PassHISM->ClearInstances();
	if (ResourceNodeHISM) ResourceNodeHISM->ClearInstances();
	if (BarbarianNodeHISM) BarbarianNodeHISM->ClearInstances();

	// P5-T2: جلب ثيم حضارة اللاعب لتلوين مدينته الخاصة
	URok2CivThemes* CivThemes = URok2CivThemes::Get();
	const FString MyCiv = Api->HasPlayer() ? Api->GetPlayer().Civ : TEXT("rome");
	const FRok2CivTheme& MyTheme = CivThemes->GetTheme(MyCiv);

	// P5-T5: جلب نظام ضباب الحرب
	URok2FogOfWar* Fog = URok2FogOfWar::Get();
	if (Fog && !Fog->IsExplored(Api->GetPlayer().X, Api->GetPlayer().Y))
	{
		// كشف المنطقة حول مدينة اللاعب تلقائياً (أول مرة)
		Fog->RevealArea(Api->GetPlayer().X, Api->GetPlayer().Y, Fog->CityRevealRadius, true);
	}

	for (const FRok2CityEntity& C : W.Cities)
	{
		FVector Loc(C.X * WorldToUnrealScale, C.Y * WorldToUnrealScale, CityZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P5-T5: لا نرسم مدناً في مناطق غير مكتشفة (إلا إذا كانت مدينة اللاعب أو حليفه)
		if (Fog && C.PlayerId != Api->GetPlayer().Id)
		{
			const double WorldX = C.X * WorldToUnrealScale;
			const double WorldY = C.Y * WorldToUnrealScale;
			if (!Fog->IsExplored(WorldX, WorldY))
			{
				continue; // مدينة في ضباب — لا نرسمها
			}
		}

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
			// P5-T2: لو كانت مدينتي، نستخدم لون حضارتي؛ وإلا نستخدم اللون الافتراضي للأصل
			FLinearColor CityColor = FLinearColor::White;
			if (C.PlayerId == Api->GetPlayer().Id)
			{
				CityColor = MyTheme.Primary; // لون حضارتي للمدينة الخاصة بي
			}
			SpawnMarker(CastleMesh, Loc, FString::Printf(TEXT("CityArt_%s"), *C.PlayerId), CityColor);
			if (AActor* Last = SpawnedActors.Num() ? SpawnedActors.Last() : nullptr)
			{
				Last->SetActorScale3D(FVector(CastleScale));
			}
		}
		else if (CityHISM)
		{
			// P5-T2: للـ placeholder، نضيف لون الحضارة عبر CustomData (لو المادة تدعم)
			// أو نستخدم SpawnMarker بلون الحضارة للمدينة الخاصة بي
			if (C.PlayerId == Api->GetPlayer().Id)
			{
				// مدينتي: نستخدم SpawnMarker بلون الحضارة بدلاً من HISM العادي
				AActor* CityActor = SpawnMarkerActor(CityMesh, Loc, FString::Printf(TEXT("City_%s"), *C.PlayerId), MyTheme.Primary);
			if (CityActor)
			{
				// P5-T6: حركة كشف (fade-in) عند ظهور المدينة بعد ضباب
				if (ARok2BuildingActor* BuildingActor = Cast<ARok2BuildingActor>(CityActor))
				{
					BuildingActor->PlayRevealAnimation();
				}
			}
			}
			else
			{
				CityHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector::OneVector));
			}
		}
	}

	for (const FRok2PassEntity& P : W.Passes)
	{
		FVector Loc(P.X * WorldToUnrealScale, P.Y * WorldToUnrealScale, PassZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P5-T5: لا نرسم ممرات في ضباب
		if (Fog && !Fog->IsExplored(P.X * WorldToUnrealScale, P.Y * WorldToUnrealScale))
		{
			continue;
		}

		if (PassHISM)
		{
			PassHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(1.5f, 1.5f, 1.5f)));
		}
	}

	for (const FRok2NodeEntity& N : W.Nodes)
	{
		FVector Loc(N.X * WorldToUnrealScale, N.Y * WorldToUnrealScale, NodeZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P5-T5: لا نرسم عقداً في ضباب
		if (Fog && !Fog->IsExplored(N.X * WorldToUnrealScale, N.Y * WorldToUnrealScale))
		{
			continue;
		}

		if (N.Kind == TEXT("barb"))
		{
			if (BarbarianNodeHISM) BarbarianNodeHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(0.8f, 0.8f, 0.8f)));
		}
		else
		{
			if (ResourceNodeHISM) ResourceNodeHISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(0.8f, 0.8f, 0.8f)));
		}
	}

	// P2-T7: مرتفعات KayKit عند زوايا مناطق Zone1 (علامات حدود بصرية)
	// P4-T7: مفتاح محتوى — لا تُعاد زراعة التلال كل تحديث (كان churn كل 3ث)؛
	// تُعاد فقط عند تغيّر مجموعة المدن فعلياً (عدد/مواقع).
	int64 CitiesKey = 0;
	for (const FRok2CityEntity& C : W.Cities)
	{
		CitiesKey = CitiesKey * 1315423911LL + (int64)FMath::RoundToInt(C.X) * 2654435761LL + (int64)FMath::RoundToInt(C.Y);
	}
	if (CitiesKey != ArtHillsKey)
	{
		// تغيّرت مجموعة المدن — أعد التلال الحالية للمسبح وأعد الزرع
		if (URok2Perf* Perf = URok2Perf::Get(this))
		{
			for (AActor* A : SpawnedHills) { if (AStaticMeshActor* SM = Cast<AStaticMeshActor>(A)) Perf->ReleaseMarkerActor(SM); }
		}
		else
		{
			for (AActor* A : SpawnedHills) { if (A) A->Destroy(); }
		}
		SpawnedHills.Empty();
		ArtHillsKey = CitiesKey;

		if (URok2ArtAssets* Art = URok2ArtAssets::Get())
		{
			if (UStaticMesh* Hills = Art->LoadMesh(TEXT("hills")))
			{
				for (const FRok2CityEntity& C : W.Cities)
				{
					// تلال خلف كل مدينة كنقطة ارتكاز بصرية للتضاريس
					FVector HillLoc(C.X * WorldToUnrealScale + 350.f, C.Y * WorldToUnrealScale + 350.f, CityZ);
					if (AActor* Hill = SpawnMarkerActor(Hills, HillLoc, TEXT("ArtHill"), FLinearColor::White))
					{
						Hill->SetActorScale3D(FVector(2.0f));
						SpawnedHills.Add(Hill);
					}
				}
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

			if (URok2AudioManager* Audio = URok2AudioManager::Get())
			{
				Audio->PlaySfx(ERok2AudioType::MarchStart);
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
	URok2Perf* PerfForCleanup = URok2Perf::Get(this);
	for (const auto& KV : SpawnedMarches)
	{
		if (!ActiveMarches.Contains(KV.Key))
		{
			// P4-T7: إعادة للمسبح بدل Destroy
			if (KV.Value)
			{
				if (PerfForCleanup) { if (AStaticMeshActor* SM = Cast<AStaticMeshActor>(KV.Value)) PerfForCleanup->ReleaseMarkerActor(SM); else KV.Value->Destroy(); }
				else KV.Value->Destroy();
			}
			ToRemove.Add(KV.Key);
		}
	}
	for (const FString& K : ToRemove) SpawnedMarches.Remove(K);
}
