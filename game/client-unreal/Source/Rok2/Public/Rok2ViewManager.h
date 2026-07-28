// Copyright ROK2. View Manager.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2ViewManager.generated.h"

class ARok2WorldRenderer;
class ARok2CityBuilder;
class ARok2IsometricCamera;

UCLASS()
class ROK2_API ARok2ViewManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ARok2ViewManager();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SwitchToMapView();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SwitchToCityView();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ToggleView();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2WorldRenderer* WorldRenderer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2CityBuilder* CityBuilder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2IsometricCamera* IsoCamera;

protected:
	virtual void BeginPlay() override;

private:
	bool bIsCityView;
};
