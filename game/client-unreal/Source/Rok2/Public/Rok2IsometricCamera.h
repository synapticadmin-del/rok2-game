// Copyright ROK2. Isometric / strategy camera actor.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "Rok2IsometricCamera.generated.h"


class USceneComponent;
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

	/** سحب بالإصبع: يحرّك الهدف بمقدار متناسب مع بُعد الكاميرا، فيبقى
	 *  إحساس "الأرض تتبع الإصبع" ثابتاً عند كل مستويات التكبير. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Touch")
	void PanByScreenDelta(const FVector2D& ScreenDelta);

	/** تكبير بإصبعين: الفرق بالبكسل بين المسافتين. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Touch")
	void ZoomByPinch(float PinchDeltaPixels);

	/** ينقل الكاميرا فوراً بلا إقحام — للتبديل بين عرض المدينة والخريطة.
	 *  الاستدعاء المباشر لـ SetActorLocation لا يعمل لأن Tick يُقحم نحو
	 *  TargetLocation فيُلغي النقل في الإطار التالي. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SnapTo(const FVector& WorldLocation);

	UPROPERTY(EditAnywhere, Category = "Rok2|Touch")
	float TouchPanScale = 0.0016f;

	UPROPERTY(EditAnywhere, Category = "Rok2|Touch")
	float TouchZoomScale = 0.004f;

	UPROPERTY(Transient)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	float TargetDistance = 2200.f;

	UPROPERTY(Transient)
	FVector CurrentVelocity = FVector::ZeroVector;

protected:
	void UpdateCameraTransform(float DeltaSeconds);
};
