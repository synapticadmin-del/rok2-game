// Copyright ROK2. Single city building actor (P5-T1 / P5-T2) — implementation.

#include "Rok2BuildingActor.h"
#include "Rok2CivThemes.h"
#include "Rok2ProceduralAssets.h"
#include "Rok2Perf.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

ARok2BuildingActor::ARok2BuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);

	RoofMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofMesh"));
	RoofMesh->SetupAttachment(Root);
	RoofMesh->SetMobility(EComponentMobility::Movable);

	TrimMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrimMesh"));
	TrimMesh->SetupAttachment(Root);
	TrimMesh->SetMobility(EComponentMobility::Movable);

	AccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AccentMesh"));
	AccentMesh->SetupAttachment(Root);
	AccentMesh->SetMobility(EComponentMobility::Movable);

	StatusIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StatusIndicator"));
	StatusIndicator->SetupAttachment(Root);
	StatusIndicator->SetMobility(EComponentMobility::Movable);
	StatusIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Mesh->SetStaticMesh(CubeFinder.Object);
		StatusIndicator->SetStaticMesh(CubeFinder.Object);
		TrimMesh->SetStaticMesh(CubeFinder.Object);
		AccentMesh->SetStaticMesh(CubeFinder.Object);
	}

	// سقف placeholder: نستخدم Cylinder لمعظم الأنماط، Cone للقباب/الأسقف المدببة
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderFinder.Succeeded()) RoofMesh->SetStaticMesh(CylinderFinder.Object);
	if (ConeFinder.Succeeded()) { /* نحتفظ به للاستخدام في ApplyArchStyleToRoof */ }
	if (SphereFinder.Succeeded()) { /* للقباب */ }

	// إخفاء الأجزاء الإضافية افتراضياً (تظهر فقط في placeholder mode)
	RoofMesh->SetVisibility(false);
	TrimMesh->SetVisibility(false);
	AccentMesh->SetVisibility(false);
}

void ARok2BuildingActor::BeginPlay()
{
	Super::BeginPlay();
	OnClicked.AddDynamic(this, &ARok2BuildingActor::HandleActorClicked);
	Mesh->SetCastShadow(false);
	UpdateStatusIndicator();
}

void ARok2BuildingActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAnimation(DeltaSeconds);
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
	// الاحتفاظ بالحضارة الافتراضية (روما) إذا لم تُحدد
	SetupWithCiv(InId, InLevel, InCell, HexSize, TEXT("rome"));
}

void ARok2BuildingActor::SetupWithCiv(const FString& InId, int32 InLevel, const FRok2HexCell& InCell, float HexSize, const FString& InCivId)
{
	BuildingId = InId;
	Level = FMath::Max(1, InLevel);
	AnchorCell = InCell;
	CachedHexSize = HexSize;
	CivId = InCivId;

	// بصمة افتراضية حسب النوع (تُستبدل بقراءة data/city_layout.json لاحقاً)
	static const TSet<FString> Medium = { TEXT("barracks"), TEXT("stable"), TEXT("archery_range"), TEXT("siege_workshop"), TEXT("hospital"), TEXT("academy"), TEXT("tavern"), TEXT("trading_post"), TEXT("alliance_center"), TEXT("builders_hut") };
	static const TSet<FString> Large = { TEXT("castle"), TEXT("city_hall") };
	if (Large.Contains(InId)) Footprint = ERok2Footprint::Large;
	else if (Medium.Contains(InId)) Footprint = ERok2Footprint::Medium;
	else Footprint = ERok2Footprint::Small;

	// القلعة المركزية ثابتة دائماً؛ إعادة الضبط ضرورية عند إعادة استخدام ممثل مبنى.
	bIsStatic = InId == TEXT("city_hall");

	// الموضع العالمي من الخلية
	const FVector Loc = URok2HexGrid::HexToWorld(AnchorCell, HexSize);
	SetActorLocation(Loc + FVector(0.f, 0.f, 40.f));

	// تطبيق ثيم الحضارة (لون + نمط)
	ApplyCivTheme();

	// مقياس المبنى placeholder (يتدرج قليلاً مع المستوى لو لا أصل فني)
	const float S = FootprintWorldScale();
	if (!bUsingArtAsset)
	{
		const bool bCityCore = InId == TEXT("city_hall");
		Mesh->SetWorldScale3D(bCityCore
			? FVector(S * 1.16f, S * 1.16f, 1.18f + Level * 0.14f)
			: FVector(S, S, 0.8f + Level * 0.1f));
		if (bCityCore)
		{
			Tags.AddUnique(FName(TEXT("city_core")));
		}
	}
}

void ARok2BuildingActor::MarkUsingArtAsset()
{
	bUsingArtAsset = true;

	// تجاوزات المواد لا تُمحى مع SetStaticMesh: المبنى مرّ أولاً بمسار placeholder
	// فحمل نسخة لون مسطّح على القناة 0، وبقيت فوق الأصل الفني الجديد فمحت نسيجه.
	// نمحوها هنا لتعود مادة الأصل المستوردة، ثم نصبغ داخلها في ApplyCivTheme.
	if (Mesh)
	{
		Mesh->EmptyOverrideMaterials();
	}

	ApplyCivTheme();
}

void ARok2BuildingActor::SetFacade(ERok2BuildingFacade NewFacade)
{
	Facade = NewFacade;
	ApplyCivTheme();
}

void ARok2BuildingActor::ApplyCivTheme()
{
	URok2CivThemes* Themes = URok2CivThemes::Get();
	if (!Themes) return;

	URok2ProceduralAssets* Mats = URok2ProceduralAssets::Get();
	if (!Mats) return;

	const FRok2CivTheme& Theme = Themes->GetTheme(CivId);

	// لو نستخدم أصل فني حقيقي (GLB)، نصبغ داخل مادته المستوردة فقط — استبدالها
	// بمادة لون مسطّح كان سيمحو نسيج KayKit كله.
	if (bUsingArtAsset)
	{
		Mats->TintExistingMaterialOn(Mesh, 0, Theme.Primary);
		ApplyFacadeStyle();
		return;
	}

	// --- Placeholder composite mode ---
	RoofMesh->SetVisibility(true);
	TrimMesh->SetVisibility(true);
	AccentMesh->SetVisibility(true);

	// الجسم الرئيسي بلون الحضارة الأساسي، والزخارف بالثانوي، والتمييز بالـAccent.
	Mats->MakeTintedMaterialOn(Mesh, 0, Theme.Primary);
	Mats->MakeTintedMaterialOn(TrimMesh, 0, Theme.Secondary);
	Mats->MakeTintedMaterialOn(AccentMesh, 0, Theme.Accent);

	// ضبط أشكال الأجزاء حسب نمط العمارة ثم طبقة الواجهة التجميلية.
	ApplyArchStyleToRoof();
	ApplyFacadeStyle();
}

void ARok2BuildingActor::ApplyFacadeStyle()
{
	if (!Mesh || !TrimMesh || !AccentMesh)
	{
		return;
	}

	Tags.Remove(FName(TEXT("facade_standard")));
	Tags.Remove(FName(TEXT("facade_ceremonial")));
	Tags.Remove(FName(TEXT("facade_fortified")));

	const float S = FootprintWorldScale();
	switch (Facade)
	{
	case ERok2BuildingFacade::Ceremonial:
		// شريط احتفالي وراية/شعلة أعلى المبنى؛ لا يغيّر من خصائص اللعب.
		Tags.Add(FName(TEXT("facade_ceremonial")));
		TrimMesh->SetVisibility(true);
		AccentMesh->SetVisibility(true);
		TrimMesh->SetWorldScale3D(FVector(S * 1.05f, S * 1.05f, 0.22f));
		AccentMesh->SetWorldScale3D(FVector(0.42f, 0.42f, 0.68f));
		AccentMesh->SetRelativeLocation(FVector(0.f, 0.f, (0.8f + Level * 0.1f) * 100.f + 70.f));
		break;

	case ERok2BuildingFacade::Fortified:
		// قاعدة أعرض وعلامة حراسة أعلى المبنى لتأكيد نطاق عسكري/دفاعي.
		Tags.Add(FName(TEXT("facade_fortified")));
		TrimMesh->SetVisibility(true);
		AccentMesh->SetVisibility(true);
		TrimMesh->SetWorldScale3D(FVector(S * 1.2f, S * 1.2f, 0.30f));
		TrimMesh->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
		AccentMesh->SetWorldScale3D(FVector(0.32f, 0.32f, 0.9f));
		AccentMesh->SetRelativeLocation(FVector(0.f, 0.f, (0.8f + Level * 0.1f) * 100.f + 55.f));
		break;

	case ERok2BuildingFacade::Standard:
	default:
		Tags.Add(FName(TEXT("facade_standard")));
		// الأصل الفني يبقى نظيفاً في الوضع القياسي، أما placeholder فيحتفظ بزخارف الحضارة.
		if (bUsingArtAsset)
		{
			TrimMesh->SetVisibility(false);
			AccentMesh->SetVisibility(false);
		}
		break;
	}
}

void ARok2BuildingActor::ApplyArchStyleToRoof()
{
	URok2CivThemes* Themes = URok2CivThemes::Get();
	if (!Themes) return;

	const FRok2CivTheme& Theme = Themes->GetTheme(CivId);
	const float S = FootprintWorldScale();
	const float BaseHeight = 0.8f + Level * 0.1f;

	// تحميل المش المناسب للسقف حسب النمط
	// P4-T7: من خبأ meshes المركزي (كان LoadObject متكرراً لكل مبنى × rebuild).
	URok2Perf* Perf = URok2Perf::Get(this);
	auto GetMesh = [&](const TCHAR* Shape) -> UStaticMesh* {
		if (Perf) return Perf->GetEngineMesh(FString(Shape));
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Shape, Shape));
	};
	UStaticMesh* RoofMeshAsset = nullptr;
	FVector RoofScale = FVector::OneVector;
	FVector RoofLoc = FVector::ZeroVector;

	switch (Theme.ArchStyle)
	{
	case ERok2ArchStyle::ArchesMarble:
		// روما: سقف قرميد أحمر مسطح قليلاً (مكعب منخفض) + أعمدة (Trim رفيع)
		RoofMeshAsset = GetMesh(TEXT("Cube"));
		RoofScale = FVector(S * 0.9f, S * 0.9f, 0.25f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + 15.f);
		// الزخارف: شريط رفيع أسفل السقف
		TrimMesh->SetWorldScale3D(FVector(S * 0.95f, S * 0.95f, 0.15f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 5.f));
		// التمييز: نسر ذهبي صغير (مكعب صغير)
		AccentMesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.5f));
		AccentMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 35.f));
		break;

	case ERok2ArchStyle::CurvedRoofs:
		// الصين: أسقف منحنية بطبقات (اسطوانة رفيعة واسعة) + ذهب إمبراطوري
		RoofMeshAsset = GetMesh(TEXT("Cylinder"));
		RoofScale = FVector(S * 1.1f, S * 1.1f, 0.35f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + 20.f);
		// طبقة ثانية أصغر فوقها
		TrimMesh->SetWorldScale3D(FVector(S * 0.7f, S * 0.7f, 0.25f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 40.f));
		// فانوس ذهبي
		AccentMesh->SetWorldScale3D(FVector(0.25f, 0.25f, 0.4f));
		AccentMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 55.f));
		break;

	case ERok2ArchStyle::DomesArches:
		// العرب: قبة فيروزية/ذهبية (كرة) + أقواس حدوة حصان
		RoofMeshAsset = GetMesh(TEXT("Sphere"));
		RoofScale = FVector(S * 0.7f, S * 0.7f, S * 0.7f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + S * 30.f);
		// قاعدة القبة
		TrimMesh->SetWorldScale3D(FVector(S * 0.8f, S * 0.8f, 0.2f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 10.f));
		// هلال/نجمة ذهبية صغيرة فوق القبة
		AccentMesh->SetWorldScale3D(FVector(0.2f, 0.2f, 0.35f));
		AccentMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + S * 30.f + 25.f));
		break;

	case ERok2ArchStyle::ObelisksColumns:
		// مصر: مسلة ذهبية (مخروط رفيع طويل) + أعمدة بردي
		RoofMeshAsset = GetMesh(TEXT("Cone"));
		RoofScale = FVector(S * 0.4f, S * 0.4f, 1.2f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + 60.f);
		// قاعدة المعبد
		TrimMesh->SetWorldScale3D(FVector(S * 0.9f, S * 0.9f, 0.3f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 10.f));
		// قرص شمس ذهبي
		AccentMesh->SetWorldScale3D(FVector(0.35f, 0.35f, 0.1f));
		AccentMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 90.f));
		break;

	case ERok2ArchStyle::CarvedWood:
		// الفايكنج: سقف خشبي طويل (مكعب مستطيل) بعوارض منحوتة
		RoofMeshAsset = GetMesh(TEXT("Cube"));
		RoofScale = FVector(S * 1.2f, S * 0.8f, 0.4f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + 20.f);
		// عوارض التنين
		TrimMesh->SetWorldScale3D(FVector(S * 1.3f, S * 0.9f, 0.15f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 35.f));
		// رأس تنين خشبي
		AccentMesh->SetWorldScale3D(FVector(0.3f, 0.3f, 0.6f));
		AccentMesh->SetRelativeLocation(FVector(S * 50.f, 0, BaseHeight * 100.f + 45.f));
		break;

	case ERok2ArchStyle::TempleWood:
		// اليابان: قلعة tenshu خشبية داكنة (طبقات متدرجة) بقوادم منحنية
		RoofMeshAsset = GetMesh(TEXT("Cube"));
		RoofScale = FVector(S * 1.0f, S * 1.0f, 0.3f);
		RoofLoc = FVector(0, 0, BaseHeight * 100.f + 18.f);
		// طبقة ثانية أصغر
		TrimMesh->SetWorldScale3D(FVector(S * 0.75f, S * 0.75f, 0.25f));
		TrimMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 38.f));
		// قوادم ذهبية (shachi)
		AccentMesh->SetWorldScale3D(FVector(0.2f, 0.2f, 0.3f));
		AccentMesh->SetRelativeLocation(FVector(0, 0, BaseHeight * 100.f + 52.f));
		break;
	}

	if (BuildingId == TEXT("city_hall"))
	{
		// القلعة = كتلة أعلى + سقف أوسع + شارة أعلى، مع الاحتفاظ بسمات الحضارة.
		RoofScale *= 1.18f;
		RoofLoc.Z += 38.f;
		TrimMesh->SetWorldScale3D(TrimMesh->GetComponentScale() * 1.18f);
		AccentMesh->SetWorldScale3D(AccentMesh->GetComponentScale() * 1.35f);
		AccentMesh->SetRelativeLocation(AccentMesh->GetRelativeLocation() + FVector(0.f, 0.f, 32.f));
	}

	if (RoofMeshAsset)
	{
		RoofMesh->SetStaticMesh(RoofMeshAsset);
	}
	RoofMesh->SetWorldScale3D(RoofScale);
	RoofMesh->SetRelativeLocation(RoofLoc);

	// تلوين السقف بلون الحضارة الثانوي (أو Accent للقباب)
	if (URok2ProceduralAssets* Mats = URok2ProceduralAssets::Get())
	{
		FLinearColor RoofColor = Theme.Secondary;
		if (Theme.ArchStyle == ERok2ArchStyle::DomesArches)
		{
			RoofColor = Theme.Accent; // قبة ذهبية/فيروزية
		}
		else if (Theme.ArchStyle == ERok2ArchStyle::CurvedRoofs)
		{
			RoofColor = Theme.Secondary; // ذهب إمبراطوري
		}
		else if (Theme.ArchStyle == ERok2ArchStyle::TempleWood)
		{
			RoofColor = Theme.Primary; // خشب داكن
		}
		Mats->MakeTintedMaterialOn(RoofMesh, 0, RoofColor);
	}
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
	const float TopZ = (0.8f + Level * 0.1f) * 100.f + 40.f;
	StatusIndicator->SetRelativeLocation(FVector(0.f, 0.f, TopZ));

	// الشكل وحده لا يكفي للدلالة على الحالة؛ اللون يرافقه دائماً.
	FLinearColor StateColor = FLinearColor::White;

	switch (VisualState)
	{
	case ERok2BuildingVisualState::Complete:
		StatusIndicator->SetVisibility(false);
		break;
	case ERok2BuildingVisualState::Constructing:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.5f, 0.5f, 0.1f));
		StateColor = FLinearColor(0.23f, 0.44f, 0.60f); // أزرق: عمل جارٍ
		break;
	case ERok2BuildingVisualState::ReadyToCollect:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.35f, 0.35f, 0.35f));
		StateColor = FLinearColor(0.83f, 0.66f, 0.18f); // ذهبي: جاهز للجمع
		break;
	case ERok2BuildingVisualState::Training:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.15f, 0.5f, 0.6f));
		StateColor = FLinearColor(0.24f, 0.49f, 0.31f); // أخضر: تدريب
		break;
	case ERok2BuildingVisualState::HasWounded:
		StatusIndicator->SetVisibility(true);
		StatusIndicator->SetWorldScale3D(FVector(0.4f, 0.15f, 0.15f));
		StateColor = FLinearColor(0.71f, 0.25f, 0.20f); // أحمر: جرحى
		break;
	}

	if (StatusIndicator->IsVisible())
	{
		if (URok2ProceduralAssets* Mats = URok2ProceduralAssets::Get())
		{
			Mats->MakeTintedMaterialOn(StatusIndicator, 0, StateColor);
		}
	}
}

void ARok2BuildingActor::HandleActorClicked(AActor* TouchedActor, FKey ButtonPressed)
{
	// يُعاد توجيهه إلى CityLayoutActor عبر حدث عام — ربط في LayoutActor عند الزرع.
}

// ---------------------------------------------------------------------------
// P5-T6: حركات البناء والترقية والكشف
// ---------------------------------------------------------------------------

void ARok2BuildingActor::PlayBuildAnimation()
{
	bIsAnimating = true;
	ActiveAnimType = 1; // بناء
	AnimTimer = 0.f;
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void ARok2BuildingActor::PlayUpgradeAnimation()
{
	bIsAnimating = true;
	ActiveAnimType = 2; // ترقية
	AnimTimer = 0.f;
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void ARok2BuildingActor::PlayRevealAnimation()
{
	bIsAnimating = true;
	ActiveAnimType = 3; // كشف
	AnimTimer = 0.f;
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void ARok2BuildingActor::UpdateAnimation(float DeltaSeconds)
{
	if (!bIsAnimating) return;

	AnimTimer += DeltaSeconds;

	float Duration = 0.5f;
	if (ActiveAnimType == 1) Duration = BuildAnimDuration;
	else if (ActiveAnimType == 2) Duration = UpgradeAnimDuration;
	else if (ActiveAnimType == 3) Duration = RevealAnimDuration;

	const float Progress = FMath::Clamp(AnimTimer / Duration, 0.f, 1.f);
	const FVector NewScale = ComputeAnimatedScale();

	// تطبيق المقياس على الجسم الرئيسي
	if (Mesh)
	{
		Mesh->SetWorldScale3D(NewScale);
	}

	// إخفاء/إظهار حسب نوع الحركة
	if (ActiveAnimType == 3) // كشف (fade-in)
	{
		// نستخدم الشفافية عبر المقياس Z (تقريب — لا يوجد shader fade بسيط)
		// المقياس يكبر من 0.01 إلى الحجم الكامل
	}

	// انتهت الحركة؟
	if (Progress >= 1.f)
	{
		bIsAnimating = false;
		ActiveAnimType = 0;
		PrimaryActorTick.bCanEverTick = false;
		SetActorTickEnabled(false);
	}
}

FVector ARok2BuildingActor::ComputeAnimatedScale() const
{
	const float S = FootprintWorldScale();
	const float BaseZ = 0.8f + Level * 0.1f;

	if (!bIsAnimating)
	{
		return FVector(S, S, BaseZ);
	}

	float Duration = 0.5f;
	if (ActiveAnimType == 1) Duration = BuildAnimDuration;
	else if (ActiveAnimType == 2) Duration = UpgradeAnimDuration;
	else if (ActiveAnimType == 3) Duration = RevealAnimDuration;

	const float Progress = FMath::Clamp(AnimTimer / Duration, 0.f, 1.f);

	if (ActiveAnimType == 1) // بناء: scale-in من 0.1 إلى 1.0
	{
		const float Scale = FMath::Lerp(0.1f, 1.0f, Progress);
		return FVector(S * Scale, S * Scale, BaseZ * Scale);
	}
	else if (ActiveAnimType == 2) // ترقية: pulse ذهبي (يكبر ثم يصغر)
	{
		const float Pulse = FMath::Sin(Progress * PI) * 0.15f; // 0 → 0.15 → 0
		const float Scale = 1.0f + Pulse;
		return FVector(S * Scale, S * Scale, BaseZ * Scale);
	}
	else if (ActiveAnimType == 3) // كشف: fade-in من 0.01 إلى 1.0
	{
		const float Scale = FMath::Lerp(0.01f, 1.0f, Progress);
		return FVector(S * Scale, S * Scale, BaseZ * Scale);
	}

	return FVector(S, S, BaseZ);
}
