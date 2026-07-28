// Copyright ROK2. Player controller driving isometric camera + input.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Rok2PlayerController.generated.h"

class URok2Api;

UCLASS()
class ARok2PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARok2PlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	URok2Api* GetApi() const { return Api; }

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FocusOnPlayerCity();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	class ARok2IsometricCamera* IsoCamera;

	UPROPERTY(EditDefaultsOnly, Category = "Rok2")
	TSubclassOf<class ARok2IsometricCamera> CameraClass;

	FVector2D PanInput = FVector2D::ZeroVector;
	float ZoomInput = 0.f;

	void OnPanX(float V) { PanInput.X = V; }
	void OnPanY(float V) { PanInput.Y = V; }
	void OnZoom(float V) { ZoomInput = V; }
	void OnTap();
	void OnEscape();
};
