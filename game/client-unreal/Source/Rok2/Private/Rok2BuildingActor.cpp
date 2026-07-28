#include "Rok2BuildingActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ARok2BuildingActor::ARok2BuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);

	StatusIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StatusIndicator"));
	StatusIndicator->SetupAttachment(Root);
	StatusIndicator->SetMobility(EComponentMobility::Movable);
	StatusIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
		StatusIndicator->SetStaticMesh(CubeFinder.Object);
	}
}

void ARok2BuildingActor::BeginPlay()
{
	Super::BeginPlay();
	OnClicked.AddDynamic(this, &ARok2BuildingActor::OnClicked);
	Mesh->SetCastShadow(false);
	UpdateStatusIndicator();
}

int32 ARok2BuildingActor::FootprintRadius() const
{
	switch (Footprint)
	{
	case ERok2Footprint::Small: return 0;
	case ERok2Footprint::Medium: return 1;
	case ERok2Footprint::Large: return 2;
	}
	return 0;
}

float ARok2BuildingActor::FootprintWorldScale() const
{
	// مقياس المكعب placeholder حسب حجم البصمة
	switch (Footprint)
	{
	case ERok2Footprint::Small: return 1.2f;
	case ERok2Footprint::Medium: return 2.6f;
	case ERok2Footprint::Large: return 3.6f;
	}
	return 1.2f;
}

void ARok2BuildingActor::Setup(const FString& InId, int32 InLevel, const FRok2HexCell& InCell, float HexSize)
{
	BuildingId = InId;
	Level = FMath::Max(1, InLevel);
	AnchorCell = InCell;
	CachedHexSize = HexSize;

	// بصمة افتراضية حسب النوع (تُستبدل بقراءة data/city_layout.json لاحقاً)
	static const TSet<FString> Medium = { TEXT("barracks"), TEXT("stable"), TEXT("archery_range"), TEXT("siege_workshop"), TEXT("hospital"), TEXT("academy"), TEXT("tavern"), TEXT("trading_post"), TEXT("alliance_center"), TEXT("builders_hut") };
	static const TSet<FString> Large = { TEXT("castle"), TEXT("city_hall") };
	if (Large.Contains(InId)) Footprint = ERok2Footprint::Large;
	else if (Medium.Contains(InId)) Footprint = ERok2Footprint::Medium;
	else Footprint = ERok2Footprint::Small;

	if (InId == TEXT("city_hall")) bIsStatic = true;

	// الموضع العالمي من الخلية
	const FVector Loc = URok2HexGrid::HexToWorld(AnchorCell, HexSize);
	SetActorLocation(Loc);

	// مقياس المبنى placeholder (يتدرج قليلاً مع المستوى لو لا أصل فني)
	const float S = FootprintWorldScale();
	Mesh->SetWorldScale3D(FVector(S, S, 0.8f + Level * 0.1f));
}

TArray<FRok2HexCell> ARok2BuildingActor::OccupiedCells() const
{
	const int32 R = FootprintRadius();
	if (R == 0) return { AnchorCell };
	if (R == 1) return URok2HexGrid::Flower(AnchorCell);
	// كبير: كل الخلايا داخل نصف قطر 2 حول المركز (تقريب 12+ خلية)
	TArray<FRok2HexCell> Out;
	for (const FRok2HexCell& C : URok2HexGrid::FilledHexagon(2))
	{
		Out.Add(FRok2HexCell(AnchorCell.Q + C.Q, AnchorCell.R + C.R));
	}
	return Out;
}

bool ARok2BuildingActor::Occupies(const FRok2HexCell& Cell) const
{
	for (const FRok2HexCell& C : OccupiedCells())
	{
		if (C == Cell) return true;
	}
	return false;
}

void ARok2BuildingActor::SetVisualState(ERok2BuildingVisualState NewState)
{
	VisualState = NewState;
	UpdateStatusIndicator();
}

void ARok2BuildingActor::UpdateStatusIndicator()
{
	// مؤشر عائم فوق المبنى حسب الحالة (placeholder بمكعب صغير ملوّن بالمقياس)
	const float S = FootprintWorldScale();
	const float TopZ = (0.8f + Level * 0.1f) * 100.f + 40.f;
	StatusIndicator->SetRelativeLocation(FVector(0.f, 0.f, TopZ));

	switch (VisualState)
	{
	case ERok2BuildingVisualState::Complete:
		StatusIndicator->SetVisibility(false);
		break;
	case ERok2BuildingVisualState::Constructing:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.5f, 0.5f, 0.1f));
		break;
	case ERok2BuildingVisualState::ReadyToCollect:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.35f, 0.35f, 0.35f));
		break;
	case ERok2BuildingVisualState::Training:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.15f, 0.5f, 0.6f));
		break;
	case ERok2BuildingVisualState::HasWounded:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.4f, 0.15f, 0.15f));
		break;
	}
}

void ARok2BuildingActor::OnClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	// يُعاد توجيهه إلى CityLayoutActor عبر حدث عام — ربط في LayoutActor عند الزرع.
}
