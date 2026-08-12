#include "Rok2ViewManager.h"
#include "Rok2WorldRenderer.h"
#include "Rok2CityBuilder.h"
#include "Rok2IsometricCamera.h"

ARok2ViewManager::ARok2ViewManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsCityView = false;
	LastMapZoomDistance = DefaultMapZoomDistance;
}

void ARok2ViewManager::BeginPlay()
{
	Super::BeginPlay();

	// تبدأ التجربة على خريطة العالم، والمدينة لا تستمر في تحديث عناصرها وهي مخفية.
	SetWorldVisibility(true);
	SetCityVisibility(false);
	if (IsoCamera)
	{
		IsoCamera->FocusOn(LastMapLocation);
		IsoCamera->SetTargetZoomDistance(LastMapZoomDistance);
	}
}

void ARok2ViewManager::SetWorldVisibility(bool bVisible)
{
	if (WorldRenderer)
	{
		WorldRenderer->SetActorHiddenInGame(!bVisible);
		WorldRenderer->SetActorTickEnabled(bVisible);
	}
}

void ARok2ViewManager::SetCityVisibility(bool bVisible)
{
	if (CityBuilder)
	{
		CityBuilder->SetActorHiddenInGame(!bVisible);
		CityBuilder->SetActorTickEnabled(bVisible);
	}
}

void ARok2ViewManager::BeginTransition(ERok2ViewTransition Direction)
{
	if (!IsoCamera || ActiveTransition != ERok2ViewTransition::None)
	{
		return;
	}

	ActiveTransition = Direction;
	TransitionElapsed = 0.f;

	if (Direction == ERok2ViewTransition::ToMap)
	{
		// تظهر الخريطة من البداية كي يبدو الخروج تكبيراً في نفس العالم، ثم تخفي المدينة لاحقاً.
		SetWorldVisibility(true);
		IsoCamera->FocusOn(LastMapLocation);
		IsoCamera->SetTargetZoomDistance(LastMapZoomDistance);
	}
	else
	{
		// المدينة تظهر أولاً، فيما تبقى الخريطة مرئية خلال الجزء الأول من حركة الاقتراب.
		SetCityVisibility(true);
		IsoCamera->FocusOn(CityViewLocation);
		IsoCamera->SetTargetZoomDistance(CityViewZoomDistance);
	}
}

void ARok2ViewManager::SwitchToMapView()
{
	if (!bIsCityView || ActiveTransition != ERok2ViewTransition::None)
	{
		return;
	}

	if (LastMapZoomDistance <= KINDA_SMALL_NUMBER)
	{
		LastMapZoomDistance = DefaultMapZoomDistance;
	}
	BeginTransition(ERok2ViewTransition::ToMap);
}

void ARok2ViewManager::SwitchToCityView()
{
	if (bIsCityView || ActiveTransition != ERok2ViewTransition::None)
	{
		return;
	}

	// نحفظ هدف الإدخال، لا الموضع المتأخر بصرياً، لذلك يعود اللاعب بدقة حتى أثناء التنعيم.
	if (IsoCamera)
	{
		LastMapLocation = IsoCamera->GetTargetFocusLocation();
		LastMapZoomDistance = FMath::Max(IsoCamera->GetTargetZoomDistance(), DefaultMapZoomDistance * 0.25f);
	}
	BeginTransition(ERok2ViewTransition::ToCity);
}

void ARok2ViewManager::FinishTransition()
{
	if (ActiveTransition == ERok2ViewTransition::ToMap)
	{
		bIsCityView = false;
		SetWorldVisibility(true);
		SetCityVisibility(false);
	}
	else if (ActiveTransition == ERok2ViewTransition::ToCity)
	{
		bIsCityView = true;
		SetWorldVisibility(false);
		SetCityVisibility(true);
	}

	ActiveTransition = ERok2ViewTransition::None;
	TransitionElapsed = 0.f;
}

void ARok2ViewManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ActiveTransition == ERok2ViewTransition::None)
	{
		return;
	}

	const float Duration = ActiveTransition == ERok2ViewTransition::ToMap
		? FMath::Max(0.01f, CityToMapDuration)
		: FMath::Max(0.01f, MapToCityDuration);
	TransitionElapsed += DeltaSeconds;
	const float Progress = FMath::Clamp(TransitionElapsed / Duration, 0.f, 1.f);

	if (ActiveTransition == ERok2ViewTransition::ToMap && Progress >= 0.35f)
	{
		SetCityVisibility(false);
	}
	else if (ActiveTransition == ERok2ViewTransition::ToCity && Progress >= 0.60f)
	{
		SetWorldVisibility(false);
	}

	if (Progress >= 1.f)
	{
		FinishTransition();
	}
}

void ARok2ViewManager::ToggleView()
{
	if (ActiveTransition != ERok2ViewTransition::None)
	{
		return;
	}

	if (bIsCityView)
	{
		SwitchToMapView();
	}
	else
	{
		SwitchToCityView();
	}
}
