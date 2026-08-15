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

	ApplyProjectionSettings();

	TargetLocation = FVector(0, 0, 0);
	TargetDistance = 2200.f;
	CurrentDistance = TargetDistance;
}

void ARok2IsometricCamera::BeginPlay()
{
	Super::BeginPlay();
	// الخصائص قابلة للتعديل في المحرر، فنعيد تطبيقها عند التشغيل أيضاً.
	ApplyProjectionSettings();
}

void ARok2IsometricCamera::ApplyProjectionSettings()
{
	if (!Camera)
	{
		return;
	}

	// إطار واحد على كل جهاز.
	//
	// FieldOfView في UE أفقي دائماً، ويُفسَّر على النسبة المرجعية في
	// UCameraComponent::AspectRatio. مع قيد MaintainYFOV يحسب المحرك مجال
	// الرؤية الرأسي = atan(tan(FOV/2) / AspectRatio) ويثبّته، ثم يمدّ الأفقي
	// بمقدار عرض النافذة الفعلي (CameraStackTypes.cpp:281).
	//
	// أي أن ثبات الإطار يتحقق بشرطين فقط: القيد MaintainYFOV، وAspectRatio
	// مضبوطة على النسبة التي ضُبطت عليها الزاوية والمسافة (16:9 نافذة المحرر).
	// حينها يرى لاعب هاتف 19.5:9 نفس العمق الرأسي، والزيادة على الجانبين فقط.
	// لا نحسب FOV يدوياً من نسبة النافذة: المحرك يفعل ذلك، وأي تصحيح إضافي
	// يُضاعف الأثر فيتّسع المجال الرأسي على الشاشات العريضة — وهو بالضبط
	// اختلاف الزاوية الذي نحاول منعه.
	Camera->AspectRatio = FMath::Max(0.1f, ReferenceAspectRatio);
	Camera->SetFieldOfView(FMath::Clamp(FieldOfView, 5.f, 170.f));
	Camera->bConstrainAspectRatio = false; // لا أشرطة سوداء — نريد ملء الشاشة
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->AspectRatioAxisConstraint = EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV;
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

void ARok2IsometricCamera::SetTargetZoomDistance(float NewDistance)
{
	TargetDistance = FMath::Clamp(NewDistance, MinZoom, MaxZoom);
}

void ARok2IsometricCamera::SnapTo(const FVector& WorldLocation)
{
	// الهدف والموضع معاً — وإلا أعاد Tick إقحام الكاميرا نحو الهدف القديم.
	TargetLocation = WorldLocation;
	SetActorLocation(WorldLocation);
	CurrentDistance = TargetDistance;
}

void ARok2IsometricCamera::PanByScreenDelta(const FVector2D& ScreenDelta)
{
	if (ScreenDelta.IsNearlyZero())
	{
		return;
	}

	// المقياس متناسب مع البُعد: كلما ابتعدت الكاميرا غطّى الإصبع مسافة أكبر.
	const float Scale = TargetDistance * TouchPanScale;

	const FRotator YawRot(0.f, Yaw, 0.f);
	const FVector Forward = UKismetMathLibrary::GetForwardVector(YawRot);
	const FVector Right = UKismetMathLibrary::GetRightVector(YawRot);

	// إحداثي Y للشاشة يزداد للأسفل: السحب لأسفل يكشف ما فوق، أي تتقدم الكاميرا.
	TargetLocation += Forward * (ScreenDelta.Y * Scale) + Right * (-ScreenDelta.X * Scale);
}

void ARok2IsometricCamera::ZoomByPinch(float PinchDeltaPixels)
{
	if (FMath::IsNearlyZero(PinchDeltaPixels))
	{
		return;
	}

	// تكبير نسبي: خطوة ثابتة بالبكسل تعطي نفس الإحساس قريباً وبعيداً.
	const float Delta = PinchDeltaPixels * TargetDistance * TouchZoomScale;
	TargetDistance = FMath::Clamp(TargetDistance - Delta, MinZoom, MaxZoom);
}

void ARok2IsometricCamera::UpdateCameraTransform(float DeltaSeconds)
{
	// Smooth follow target
	FVector Cur = GetActorLocation();
	FVector New = FMath::VInterpTo(Cur, TargetLocation, DeltaSeconds, 8.f);
	SetActorLocation(New);

	// المسافة تستقر نحو هدفها مثل نقطة التركيز كي لا تتحول إيماءة التكبير إلى قفزة بصرية.
	CurrentDistance = FMath::FInterpTo(CurrentDistance, TargetDistance, DeltaSeconds, CameraTransitionSpeed);

	// Apply camera offset along pitch direction
	FRotator CamRot(Pitch, Yaw, 0.f);
	FVector Offset = UKismetMathLibrary::GetForwardVector(CamRot) * (-CurrentDistance);
	if (Camera)
	{
		Camera->SetRelativeLocation(Offset);
		Camera->SetRelativeRotation(CamRot);
	}
}
