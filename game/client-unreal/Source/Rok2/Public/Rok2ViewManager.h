//
// يدير الانتقال السلس بين مدينة اللاعب وخريطة العالم، مع حفظ موضع الخريطة.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rok2ViewManager.generated.h"

class ARok2WorldRenderer;
class ARok2CityBuilder;
class ARok2IsometricCamera;

enum class ERok2ViewTransition : uint8
{
	None,
	ToMap,
	ToCity
};

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

	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsCityView() const { return bIsCityView; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsTransitioning() const { return ActiveTransition != ERok2ViewTransition::None; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2WorldRenderer* WorldRenderer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2CityBuilder* CityBuilder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	ARok2IsometricCamera* IsoCamera;

	/** مركز مدينة اللاعب؛ Rok2CityBuilder يبني القلعة حول هذه النقطة. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FVector CityViewLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Transition")
	float CityViewZoomDistance = 2200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Transition")
	float DefaultMapZoomDistance = 26000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Transition")
	float CityToMapDuration = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Transition")
	float MapToCityDuration = 0.55f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bIsCityView = false;
	ERok2ViewTransition ActiveTransition = ERok2ViewTransition::None;
	float TransitionElapsed = 0.f;

	/** آخر هدف للكاميرا على الخريطة، لا موضع الممثل المتأخر بالتنعيم. */
	FVector LastMapLocation = FVector::ZeroVector;
	float LastMapZoomDistance = 26000.f;

	void BeginTransition(ERok2ViewTransition Direction);
	void FinishTransition();
	void SetWorldVisibility(bool bVisible);
	void SetCityVisibility(bool bVisible);
};
