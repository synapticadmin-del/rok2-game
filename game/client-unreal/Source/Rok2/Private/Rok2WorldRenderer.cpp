// Copyright Rok2. World renderer impl.

#include "Rok2WorldRenderer.h"
#include "Rok2BuildingActor.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2ProceduralAssets.h"
#include "Rok2ArtAssets.h"
#include "Rok2CivThemes.h"
#include "Rok2WorldIconography.h"
#include "Rok2VisualTheme.h"
#include "Rok2FogOfWar.h"
#include "Rok2AudioManager.h"
#include "Rok2Perf.h"
#include "Rok2IsometricCamera.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

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

bool ARok2WorldRenderer::UpdateZoomLayer(float TargetZoomDistance)
{
	const ERok2WorldZoomLayer NextLayer = TargetZoomDistance < TacticalZoomMaxDistance
		? ERok2WorldZoomLayer::Tactical
		: (TargetZoomDistance < RegionalZoomMaxDistance ? ERok2WorldZoomLayer::Regional : ERok2WorldZoomLayer::Kingdom);
	if (NextLayer == CurrentZoomLayer)
	{
		return false;
	}

	CurrentZoomLayer = NextLayer;
	ApplyZoomLayerVisibility();
	// تعاد قراءة اللقطة المخزنة في API فقط عند تغير الطبقة؛ لا طلب شبكة إضافي ولا churn لكل إطار.
	if (Api)
	{
		RefreshFromApi();
	}
	return true;
}

bool ARok2WorldRenderer::CanIssueMarchCommand() const
{
	return Api && Api->HasPlayer() && CurrentZoomLayer == ERok2WorldZoomLayer::Tactical
		&& GetActiveMarchCount() < GetMarchCapacity();
}

bool ARok2WorldRenderer::CanInteractWithWorldTarget(const FString& TargetType, bool bRequiresMarchOrder) const
{
	const FString Normalized = TargetType.ToLower();
	const bool bTacticalTarget = Normalized == TEXT("resource") || Normalized == TEXT("node")
		|| Normalized == TEXT("barb") || Normalized == TEXT("barbarian") || Normalized == TEXT("city")
		|| Normalized == TEXT("pass") || Normalized == TEXT("throne")
		|| Normalized == TEXT("core_objective") || Normalized == TEXT("point");
	const bool bRegionalTarget = Normalized == TEXT("city") || Normalized == TEXT("pass")
		|| Normalized == TEXT("march") || Normalized == TEXT("alliance_structure");
	const bool bKingdomTarget = Normalized == TEXT("city") || Normalized == TEXT("alliance_structure");

	if (bRequiresMarchOrder)
	{
		return bTacticalTarget && CanIssueMarchCommand();
	}
	if (CurrentZoomLayer == ERok2WorldZoomLayer::Tactical) return bTacticalTarget || bRegionalTarget;
	if (CurrentZoomLayer == ERok2WorldZoomLayer::Regional) return bRegionalTarget;
	return bKingdomTarget;
}

int32 ARok2WorldRenderer::GetActiveMarchCount() const
{
	if (!Api || !Api->HasPlayer()) return 0;

	const FString& PlayerId = Api->GetPlayer().Id;
	int32 Count = 0;
	for (const FRok2MarchEntity& March : CurrentMarches)
	{
		if (March.OwnerPlayerId == PlayerId && (March.State == TEXT("moving") || March.State == TEXT("gathering")))
		{
			++Count;
		}
	}
	return Count;
}

int32 ARok2WorldRenderer::GetMarchCapacity() const
{
	const int32 HallLevel = Api ? FMath::Max(1, Api->GetCity().HallLevel) : 1;
	return FMath::Min(5, 1 + (HallLevel - 1) / 5);
}

FRok2WorldPerfSnapshot ARok2WorldRenderer::GetPerformanceSnapshot() const
{
	FRok2WorldPerfSnapshot Snapshot;
	Snapshot.CityInstances = CityHISM ? CityHISM->GetInstanceCount() : 0;
	Snapshot.PassInstances = PassHISM ? PassHISM->GetInstanceCount() : 0;
	Snapshot.ResourceNodeInstances = ResourceNodeHISM ? ResourceNodeHISM->GetInstanceCount() : 0;
	Snapshot.BarbarianNodeInstances = BarbarianNodeHISM ? BarbarianNodeHISM->GetInstanceCount() : 0;
	Snapshot.MarkerActors = SpawnedActors.Num();
	Snapshot.MarchActors = SpawnedMarches.Num();
	Snapshot.HillActors = SpawnedHills.Num();
	if (const URok2Perf* Perf = URok2Perf::Get(this))
	{
		Snapshot.PooledMarkerActors = Perf->PoolSize();
		Snapshot.WorldFrameSamples = Perf->GetWorldFrameSampleCount();
		Snapshot.WorldFrameAverageMs = Perf->GetWorldFrameAverageMs();
		Snapshot.WorldFramePeakMs = Perf->GetWorldFramePeakMs();
	}
	return Snapshot;
}

void ARok2WorldRenderer::ResetPerformanceSnapshot()
{
	if (URok2Perf* Perf = URok2Perf::Get(this))
	{
		Perf->ResetWorldFrameTelemetry();
	}
}

void ARok2WorldRenderer::ApplyZoomLayerVisibility()
{
	const bool bTactical = IsTacticalLayer();
	const bool bRegional = IsRegionalOrCloserLayer();
	if (PassHISM) PassHISM->SetVisibility(bRegional, true);
	if (ResourceNodeHISM) ResourceNodeHISM->SetVisibility(bTactical, true);
	if (BarbarianNodeHISM) BarbarianNodeHISM->SetVisibility(bTactical, true);

	for (const auto& Entry : SpawnedMarches)
	{
		if (Entry.Value) Entry.Value->SetActorHiddenInGame(!bRegional);
	}
}

void ARok2WorldRenderer::RequestAllianceStructureAtWorldPoint(const FString& StructureKind, FVector WorldPoint)
{
	if (CurrentZoomLayer != ERok2WorldZoomLayer::Tactical)
	{
		UE_LOG(LogTemp, Warning, TEXT("Alliance structure placement requires tactical map zoom."));
		return;
	}
	if (!Api)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot build alliance structure: API is unavailable."));
		return;
	}
	if (WorldToUnrealScale <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot build alliance structure: WorldToUnrealScale must be positive."));
		return;
	}

	Api->BuildAllianceStructure(
		StructureKind,
		WorldPoint.X / WorldToUnrealScale,
		WorldPoint.Y / WorldToUnrealScale
	);
}

void ARok2WorldRenderer::OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot)
{
	RefreshFromApi();
}

void ARok2WorldRenderer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (URok2Perf* Perf = URok2Perf::Get(this))
	{
		Perf->RecordWorldFrame(DeltaSeconds);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (ARok2IsometricCamera* Camera = Cast<ARok2IsometricCamera>(PC->GetViewTarget()))
		{
			UpdateZoomLayer(Camera->GetTargetZoomDistance());
		}
	}

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
		if (!IsRegionalOrCloserLayer())
		{
			if (ActorPtr && *ActorPtr) (*ActorPtr)->SetActorHiddenInGame(true);
			continue;
		}
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

AActor* ARok2WorldRenderer::SpawnSpriteActor(UTexture2D* Icon, const FVector& Loc, const FString& Label, float Scale)
{
	if (!Icon) return nullptr;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* SpriteActor = GetWorld()->SpawnActor<AActor>(Loc, FRotator::ZeroRotator, P);
	if (!SpriteActor) return nullptr;

	// P7-T10: billboard يواجه الكاميرا دائماً — الأيقونة المولدة مصممة باتجاه +X.
	UBillboardComponent* Sprite = NewObject<UBillboardComponent>(SpriteActor);
	Sprite->SetSprite(Icon);
	Sprite->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	Sprite->bIsScreenSizeScaled = false;
	Sprite->SetupAttachment(SpriteActor->GetRootComponent());
	Sprite->RegisterComponent();
	SpriteActor->SetActorLocation(Loc);
#if WITH_EDITOR
	SpriteActor->SetActorLabel(Label);
#endif
	return SpriteActor;
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
	const FString MyAlliance = Api->HasPlayer() ? Api->GetPlayer().AllianceId : TEXT("");
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
		if (!IsRegionalOrCloserLayer()) continue;
		FVector Loc(P.X * WorldToUnrealScale, P.Y * WorldToUnrealScale, PassZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P5-T5: لا نرسم ممرات في ضباب
		if (Fog && !Fog->IsExplored(P.X * WorldToUnrealScale, P.Y * WorldToUnrealScale))
		{
			continue;
		}

		// P7-T1: نحول هدف الممر إلى بوابة دلالية؛ والعرش يطلب تاجاً حين يظهر
		// بمعرّفه السلطوي. يحتفظ Label بالـ glyph كي يُرى التشخيص في محرر UE.
		const FString PassTargetType = P.Id.Contains(TEXT("throne"), ESearchCase::IgnoreCase)
			? TEXT("throne") : TEXT("pass");
		const FRok2WorldIconStyle Style = URok2WorldIconography::Resolve(PassTargetType, P.Id, P.Level);
		const FLinearColor IconColor = FMath::Lerp(Style.BaseColor, Style.TierColor, 0.35f);

		// P7-T10: أيقونة PNG مخصصة للعرش والممرات عند توفر الحزمة، مع fallback هندسي.
		if (UTexture2D* Icon = URok2ArtAssets::LoadWorldMapIcon(Style.IconId.ToString()))
		{
			if (AActor* Marker = SpawnSpriteActor(Icon, Loc,
				FString::Printf(TEXT("%s_%s_T%d"), *Style.Glyph, *P.Id, Style.Tier), Style.WorldScale))
			{
				SpawnedActors.Add(Marker);
			}
		}
		else if (PassMesh)
		{
			if (AActor* Marker = SpawnMarkerActor(PassMesh, Loc,
				FString::Printf(TEXT("%s_%s_T%d"), *Style.Glyph, *P.Id, Style.Tier), IconColor))
			{
				Marker->SetActorScale3D(FVector(Style.WorldScale));
				SpawnedActors.Add(Marker);
			}
		}
	}

	for (const FRok2NodeEntity& N : W.Nodes)
	{
		if (!IsTacticalLayer()) continue;
		FVector Loc(N.X * WorldToUnrealScale, N.Y * WorldToUnrealScale, NodeZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;

		// P5-T5: لا نرسم عقداً في ضباب
		if (Fog && !Fog->IsExplored(N.X * WorldToUnrealScale, N.Y * WorldToUnrealScale))
		{
			continue;
		}

		// P7-T1: تستخدم كل عقدة قاموس P6 لاختيار مورد/برابرة وتدرج المستوى.
		// P7-T10: عند توفر حزمة أيقونات خريطة العالم نرسم العُقد كـ sprites مخصصة
		// (أيقونة المورد/البرابرة)، وإلا نبقى على الشكل الهندسي المجمّع.
		const FRok2WorldIconStyle Style = URok2WorldIconography::Resolve(N.Kind, N.Kind, N.Level);
		const FLinearColor IconColor = FMath::Lerp(Style.BaseColor, Style.TierColor, 0.35f);
		FString IconId = Style.IconId.ToString();
		if (IconId.StartsWith(TEXT("barbarian"))) IconId = TEXT("node_barbarian");
		else if (IconId == TEXT("node_resource") || IconId == TEXT("world_marker")) IconId = TEXT("node_resource_generic");

		if (UTexture2D* Icon = URok2ArtAssets::LoadWorldMapIcon(IconId))
		{
			if (AActor* Marker = SpawnSpriteActor(Icon, Loc,
				FString::Printf(TEXT("%s_%s_T%d"), *Style.Glyph, *N.Id, Style.Tier), Style.WorldScale))
			{
				SpawnedActors.Add(Marker);
			}
		}
		else if (NodeMesh)
		{
			if (AActor* Marker = SpawnMarkerActor(NodeMesh, Loc,
				FString::Printf(TEXT("%s_%s_T%d"), *Style.Glyph, *N.Id, Style.Tier), IconColor))
			{
				Marker->SetActorScale3D(FVector(Style.WorldScale));
				SpawnedActors.Add(Marker);
			}
		}
	}

	// منشآت التحالف: نقطة مرئية ونطاق حماية مستمدان من لقطة الخادم.
	// اللون يميّز منشأة اللاعب/الحليف عن المنشأة المعادية؛ لا تُرسم في الضباب.
	for (const FRok2AllianceStructure& S : W.AllianceStructures)
	{
		const FVector Loc(S.X * WorldToUnrealScale, S.Y * WorldToUnrealScale, AllianceStructureZ);
		if (FVector::DistSquared(Loc, CamLoc) > RenderDistanceSq) continue;
		if (Fog && !Fog->IsExplored(S.X * WorldToUnrealScale, S.Y * WorldToUnrealScale)) continue;

		const bool bFriendly = !MyAlliance.IsEmpty() && S.AllianceId == MyAlliance;
		const FLinearColor StructureColor = bFriendly
			? Rok2Visual::Information()
			: Rok2Visual::Danger();

		// P7-T10: أيقونة مخصصة للبرج/المنجنيق عند توفر الحزمة، مع fallback هندسي.
		const FString StructureIconId = S.Kind == TEXT("catapult_emplacement")
			? TEXT("alliance_catapult") : TEXT("alliance_bastion");
		const float StructureScale = S.Kind == TEXT("bastion") ? 1.35f : (S.Kind == TEXT("catapult_emplacement") ? 1.15f : 0.95f);

		if (UTexture2D* Icon = URok2ArtAssets::LoadWorldMapIcon(StructureIconId))
		{
			if (AActor* Marker = SpawnSpriteActor(Icon, Loc, FString::Printf(TEXT("AllianceStructure_%s"), *S.Id), StructureScale))
			{
				SpawnedActors.Add(Marker);
			}
		}
		else
		{
			UStaticMesh* MarkerMesh = AllianceStructureMesh ? AllianceStructureMesh : NodeMesh;
			if (MarkerMesh)
			{
				if (AActor* Marker = SpawnMarkerActor(MarkerMesh, Loc, FString::Printf(TEXT("AllianceStructure_%s"), *S.Id), StructureColor))
				{
					Marker->SetActorScale3D(FVector(StructureScale));
					SpawnedActors.Add(Marker);
				}
			}
		}

		if (IsTacticalLayer() && ProtectionRadiusMesh && S.ProtectionRadius > 0.0)
		{
			const float DiameterScale = FMath::Max(0.01f, float((S.ProtectionRadius * WorldToUnrealScale) / 50.0));
			if (AActor* Range = SpawnMarkerActor(ProtectionRadiusMesh, FVector(Loc.X, Loc.Y, AllianceStructureZ - 4.f), FString::Printf(TEXT("ProtectionRange_%s"), *S.Id), StructureColor.CopyWithNewOpacity(0.18f)))
			{
				Range->SetActorScale3D(FVector(DiameterScale, DiameterScale, 1.f));
				SpawnedActors.Add(Range);
			}
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
	for (const FRok2MarchEntity& M : CurrentMarches)
	{
		// لا نرسم المسيرات المنتهية
		if (M.State == TEXT("returned") || M.State == TEXT("cancelled") || M.State == TEXT("arrived")) continue;

		ActiveMarches.Add(M.Id);
		if (!IsRegionalOrCloserLayer()) continue;
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

			// P7-T10: أيقونة الفرع الأبرز للمسيرات (infantry/cavalry/archer/siege)
			// عند توفر الحزمة، وإلا المخروط الهندسي الافتراضي بلون التحالف.
			AActor* NewMarch = nullptr;
			if (UTexture2D* MarchIcon = URok2ArtAssets::LoadWorldMapIcon(TEXT("march_") + M.Branch))
			{
				NewMarch = SpawnSpriteActor(MarchIcon,
					FVector(M.FromX * WorldToUnrealScale, M.FromY * WorldToUnrealScale, MarchZ),
					FString::Printf(TEXT("March_%s_%s"), *M.Branch, *M.Id), 0.75f);
			}
			if (!NewMarch)
			{
				NewMarch = SpawnMarkerActor(
					MarchMesh,
					FVector(M.FromX * WorldToUnrealScale, M.FromY * WorldToUnrealScale, MarchZ),
					FString::Printf(TEXT("March_%s"), *M.Id), Col);
				if (NewMarch)
				{
					NewMarch->SetActorScale3D(FVector(0.6f, 0.6f, 0.6f));
				}
			}
			if (NewMarch)
			{
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
