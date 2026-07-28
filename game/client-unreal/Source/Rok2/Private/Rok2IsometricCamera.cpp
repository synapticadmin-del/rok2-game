// Copyright Rok2. Isometric camera impl.

#include "Rok2IsometricCamera.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetMathLibrary.h"

ARok2IsometricCamera::ARok2IsometricCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	Camera->SetRelativeRotation(FRotator(Pitch, Yaw, 0.f));

	TargetLocation = FVector(0, 0, 0);
	TargetDistance = 2200.f;
}

void ARok2IsometricCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCameraTransform(DeltaSeconds);
}

void ARok2IsometricCamera::AddPan(const FVector2D& Dir)
{
	if (Dir.IsNearlyZero()) return;
	// Pan relative to camera yaw (screen-space)
	FRotator YawRot(0.f, Yaw, 0.f);
	FVector Forward = UKismetMathLibrary::GetForwardVector(YawRot);
	FVector Right = UKismetMathLibrary::GetRightVector(YawRot);
	// Use screen axis: up = forward, right = right
	FVector Delta = Forward * (-Dir.Y * PanSpeed) + Right * (Dir.X * PanSpeed);
	TargetLocation += Delta;
}

void ARok2IsometricCamera::AddZoom(float V)
{
	if (FMath::IsNearlyZero(V)) return;
	TargetDistance = FMath::Clamp(TargetDistance - V * ZoomSpeed, MinZoom, MaxZoom);
}

void ARok2IsometricCamera::FocusOn(const FVector& WorldLocation)
{
	TargetLocation = WorldLocation;
}

void ARok2IsometricCamera::UpdateCameraTransform(float DeltaSeconds)
{
	// Smooth follow target
	FVector Cur = GetActorLocation();
	FVector New = FMath::VInterpTo(Cur, TargetLocation, DeltaSeconds, 8.f);
	SetActorLocation(New);

	// Apply camera offset along pitch direction
	FRotator CamRot(Pitch, Yaw, 0.f);
	FVector Offset = UKismetMathLibrary::GetForwardVector(CamRot) * (-TargetDistance);
	if (Camera)
	{
		Camera->SetRelativeLocation(Offset);
		Camera->SetRelativeRotation(CamRot);
	}
}
