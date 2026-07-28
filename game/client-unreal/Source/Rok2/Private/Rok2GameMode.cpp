#include "Rok2GameMode.h"
#include "Rok2Api.h"
#include "Rok2WorldRenderer.h"
#include "Rok2CityBuilder.h"
#include "Rok2PlayerController.h"
#include "Rok2BootWidget.h"
#include "Rok2CityWidget.h"
#include "Rok2HudWidget.h"
#include "Rok2BlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Kismet/GameplayStatics.h"

ARok2GameMode::ARok2GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PlayerControllerClass = ARok2PlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	ApiBaseUrl = TEXT("https://rok2-api.lolelarap.workers.dev");
	KingdomId = TEXT("kingdom-1");
	AdminKey = TEXT("rok2-dev-admin");
	TickIntervalSeconds = 0.1f;
}

void ARok2GameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (World)
	{
		// 1. Ensure Directional Light exists
		if (!UGameplayStatics::GetActorOfClass(World, ADirectionalLight::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector(0.f, 0.f, 1000.f), FRotator(-45.f, -45.f, 0.f), P);
			if (Sun && Sun->GetLightComponent())
			{
				Sun->GetLightComponent()->SetIntensity(3.14f);
				Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f, 0.95f, 0.85f));
			}
		}

		// 2. Ensure Sky Light exists
		if (!UGameplayStatics::GetActorOfClass(World, ASkyLight::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASkyLight* Sky = World->SpawnActor<ASkyLight>(FVector(0.f, 0.f, 1100.f), FRotator::ZeroRotator, P);
			if (Sky && Sky->GetLightComponent())
			{
				Sky->GetLightComponent()->SetIntensity(1.5f);
				Sky->GetLightComponent()->SetLightColor(FLinearColor(0.8f, 0.9f, 1.f));
			}
		}

		// 3. Ensure Rok2WorldRenderer exists
		if (!UGameplayStatics::GetActorOfClass(World, ARok2WorldRenderer::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<ARok2WorldRenderer>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		}

		// 4. Ensure Rok2CityBuilder exists
		if (!UGameplayStatics::GetActorOfClass(World, ARok2CityBuilder::StaticClass()))
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<ARok2CityBuilder>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		}
	}

	if (!Api)
	{
		Api = NewObject<URok2Api>(this);
	}
	// Populate civilizations from BlueprintLibrary before login
	Api->SetCivilizations(URok2BlueprintLibrary::GetDefaultCivilizations());
	Api->Init(ApiBaseUrl, KingdomId, AdminKey);
	Api->OnPlayerLoaded.AddDynamic(this, &ARok2GameMode::OnPlayerLoadedHandler);

	// Spawn Boot Widget UI
	if (!BootWidget && World)
	{
		BootWidget = Cast<URok2BootWidget>(URok2BlueprintLibrary::CreateRok2Widget(World, URok2BootWidget::StaticClass()));
		if (BootWidget)
		{
			BootWidget->Setup(Api);
			BootWidget->AddToViewport(100);
		}
	}

	Api->LoginAsGuest();
}

void ARok2GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Api)
	{
		Api->PumpEvents(DeltaSeconds);
	}
}

void ARok2GameMode::OnPlayerLoadedHandler(const FRok2Player& Player)
{
	if (BootWidget)
	{
		BootWidget->RemoveFromParent();
		BootWidget = nullptr;
	}

	if (!CityWidget && GetWorld())
	{
		CityWidget = Cast<URok2CityWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2CityWidget::StaticClass()));
		if (CityWidget)
		{
			CityWidget->Setup(Api);
			CityWidget->AddToViewport(10);
		}
	}

	// P2-T6: HUD موحد فوق بقية الواجهات — موارد حية + طوابير + إشعارات + شريط تنقل
	if (!HudWidget && GetWorld())
	{
		HudWidget = Cast<URok2HudWidget>(URok2BlueprintLibrary::CreateRok2Widget(GetWorld(), URok2HudWidget::StaticClass()));
		if (HudWidget)
		{
			HudWidget->Setup(Api);
			HudWidget->AddToViewport(20);
		}
	}
}
