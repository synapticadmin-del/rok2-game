// Copyright ROK2. View Manager implementation.

#include "Rok2ViewManager.h"
#include "Rok2WorldRenderer.h"
#include "Rok2CityBuilder.h"
#include "Rok2IsometricCamera.h"
#include "Kismet/GameplayStatics.h"

ARok2ViewManager::ARok2ViewManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsCityView = false;
}

void ARok2ViewManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Default to map view initially, or city view if requested.
	// For now we assume map view is the default.
}

void ARok2ViewManager::SwitchToMapView()
{
	if (!bIsCityView) return;
	bIsCityView = false;

	if (WorldRenderer)
	{
		WorldRenderer->SetActorHiddenInGame(false);
		WorldRenderer->SetActorTickEnabled(true);
	}

	if (CityBuilder)
	{
		CityBuilder->SetActorHiddenInGame(true);
		CityBuilder->SetActorTickEnabled(false);
	}

	if (IsoCamera)
	{
		// SnapTo وليس SetActorLocation: الأخيرة تنقل الـ actor بينما يظل
		// Tick يُقحم نحو TargetLocation القديم، فترتد الكاميرا فوراً.
		IsoCamera->SnapTo(LastMapLocation);
	}
}

void ARok2ViewManager::SwitchToCityView()
{
	if (bIsCityView) return;
	bIsCityView = true;

	if (WorldRenderer)
	{
		WorldRenderer->SetActorHiddenInGame(true);
		WorldRenderer->SetActorTickEnabled(false);
	}

	if (CityBuilder)
	{
		CityBuilder->SetActorHiddenInGame(false);
		CityBuilder->SetActorTickEnabled(true);
	}

	if (IsoCamera)
	{
		// نحفظ موضع الخريطة قبل المغادرة حتى يعود اللاعب حيث كان،
		// بدل إعادته دائماً إلى مركز العالم.
		LastMapLocation = IsoCamera->GetActorLocation();
		IsoCamera->SnapTo(CityViewLocation);
	}
}

void ARok2ViewManager::ToggleView()
{
	if (bIsCityView)
	{
		SwitchToMapView();
	}
	else
	{
		SwitchToCityView();
	}
}
