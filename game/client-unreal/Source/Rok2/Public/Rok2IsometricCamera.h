// Copyright ROK2. Isometric / strategy camera actor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Rok2IsometricCamera.generated.h"

UCLASS()
class ARok2IsometricCamera : public AActor
{
	GENERATED_BODY()

public:
	ARok2IsometricCamera();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Rok2")
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float PanSpeed = 15000.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float ZoomSpeed = 20000.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float MinZoom = 800.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float MaxZoom = 80000.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float Pitch = -50.f;

	UPROPERTY(EditAnywhere, Category = "Rok2")
	float Yaw = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AddPan(const FVector2D& Dir);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AddZoom(float V);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FocusOn(const FVector& WorldLocation);

	UPROPERTY(Transient)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float TargetDistance = 2200.f;

	UPROPERTY(Transient)
	FVector CurrentVelocity = FVector::ZeroVector;

protected:
	void UpdateCameraTransform(float DeltaSeconds);
};
