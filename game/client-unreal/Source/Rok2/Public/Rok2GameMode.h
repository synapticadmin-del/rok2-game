// Copyright ROK2. Game mode for the kingdom.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Rok2Types.h"
#include "Rok2GameMode.generated.h"

class URok2Api;
class URok2BootWidget;
class URok2CityWidget;
class URok2HudWidget;
class URok2BuildMenuWidget;
class URok2CommanderWidget;
class URok2AllianceRosterWidget;
class URok2BattleReportWidget;
class ARok2ViewManager;
class ARok2CityBuilder;

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

	/** HUD موحد بأسلوب RoK (P5-T3): موارد + أزرار دائرية + إشعارات */
	UPROPERTY(Transient)
	URok2HudWidget* HudWidget;

	/** مدير العرض مدينة/خريطة */
	UPROPERTY(Transient)
	ARok2ViewManager* ViewManager;

	/** واجهات تُفتح عند الطلب (تُنشأ مرة وتُخفى) */
	UPROPERTY(Transient)
	URok2BuildMenuWidget* BuildMenuWidget;

	UPROPERTY(Transient)
	URok2CommanderWidget* CommanderWidget;

	UPROPERTY(Transient)
	URok2AllianceRosterWidget* AllianceWidget;

	UPROPERTY(Transient)
	URok2BattleReportWidget* BattleReportWidget;

protected:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	float TickIntervalSeconds;

	UFUNCTION()
	void OnPlayerLoadedHandler(const FRok2Player& Player);

	// --- معالجات أحداث HUD (P5-T3) ---
	UFUNCTION() void HandleBuildAction();
	UFUNCTION() void HandleEditCityAction();
	UFUNCTION() void HandleCommandersAction();
	UFUNCTION() void HandleAllianceAction();
	UFUNCTION() void HandleItemsAction();
	UFUNCTION() void HandleEventsAction();
	UFUNCTION() void HandleMapAction();
	UFUNCTION() void HandleReportsAction();
	UFUNCTION() void HandleBuildMenuPick(const FString& BuildingId);

	/** ربط أحداث HUD بالمعالجات بعد إنشائها */
	void BindHudEvents();

	/** يجلب/ينشئ ViewManager ويربطه بالـ CityBuilder والكاميرا */
	void EnsureViewManager();
};
