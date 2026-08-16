#include "Rok2CityBuilder.h"
#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2BuildingDetailWidget.h"
#include "Rok2BlueprintLibrary.h"
#include "Rok2CityLayoutActor.h"
#include "Rok2CityEditorMode.h"
#include "Rok2BuildingActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Blueprint/UserWidget.h"

ARok2CityBuilder::ARok2CityBuilder()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ARok2CityBuilder::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	// زرع مدير التخطيط السداسي
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Layout = World->SpawnActor<ARok2CityLayoutActor>(FVector::ZeroVector, FRotator::ZeroRotator, P);

	// زرع وضع التحرير وربطه بالمدير
	Editor = World->SpawnActor<ARok2CityEditorMode>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Editor && Layout)
	{
		Editor->SetLayout(Layout);
	}

	// ربط حدث لمس المبنى ببطاقة التفاصيل
	if (Layout)
	{
		Layout->OnBuildingPicked.AddDynamic(this, &ARok2CityBuilder::OnBuildingPickedHandler);
	}

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(World->GetAuthGameMode()))
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
	if (Layout)
	{
		Layout->RebuildFromApi();
	}
}

void ARok2CityBuilder::ToggleEditMode()
{
	if (!Editor) return;
	if (Editor->bActive)
	{
		Editor->ExitEditMode(true);
	}
	else
	{
		Editor->EnterEditMode();
	}
}

bool ARok2CityBuilder::IsEditModeActive() const
{
	return Editor && Editor->bActive;
}

void ARok2CityBuilder::OnBuildingPickedHandler(const FString& BuildingId)
{
	// في وضع التحرير: اللمس يبدأ سحباً. وإلا: يفتح بطاقة المبنى.
	if (Editor && Editor->bActive)
	{
		Editor->BeginDrag(BuildingId);
		return;
	}

	if (Api)
	{
		const int32 Level = Api->GetBuildings().Contains(BuildingId) ? Api->GetBuildings()[BuildingId] : 1;
		if (URok2BuildingDetailWidget* DetailWidget = Cast<URok2BuildingDetailWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2BuildingDetailWidget::StaticClass())))
		{
			DetailWidget->SetupBuilding(Api, BuildingId, Level);
			// P18-T2: الزر الثانوي (بحث/تدريب/شفاء/صناديق) كان يُبث بلا مشترك —
			// المسار الوحيد الآن: GameMode يجيب الحدث ويفتح الشاشة الصحيحة.
			if (ARok2GameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ARok2GameMode>() : nullptr)
			{
				DetailWidget->OnBuildingAction.AddDynamic(GM, &ARok2GameMode::HandleBuildingAction);
			}
			DetailWidget->AddToViewport(200);
		}
	}
}
