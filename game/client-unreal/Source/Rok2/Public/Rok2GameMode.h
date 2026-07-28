// Copyright ROK2. Game mode for the kingdom.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Rok2GameMode.generated.h"

class URok2Api;
class URok2BootWidget;
class URok2CityWidget;

UCLASS(minimalapi)
class ARok2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARok2GameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The Cloudflare API base URL. Editable in editor for dev vs prod. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString ApiBaseUrl;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString KingdomId;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString AdminKey;

	/** Shared HTTP + WS client for all actors. */
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	URok2BootWidget* BootWidget;

	UPROPERTY(Transient)
	URok2CityWidget* CityWidget;

protected:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	float TickIntervalSeconds;

	UFUNCTION()
	void OnPlayerLoadedHandler(const FRok2Player& Player);
};
