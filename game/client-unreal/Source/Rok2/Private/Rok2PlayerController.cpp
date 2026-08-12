// Copyright ROK2. Player controller impl.

#include "Rok2PlayerController.h"
#include "Rok2GameMode.h"
#include "Rok2IsometricCamera.h"
#include "Rok2Api.h"
#include "Rok2Types.h"
#include "EngineUtils.h"
#include "Rok2MarchPanel.h"
#include "Rok2WorldRenderer.h"
#include "Kismet/GameplayStatics.h"
#include "Rok2BlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

ARok2PlayerController::ARok2PlayerController()
{
	// بدون هذا لا تصل أحداث اللمس إلى InputComponent على أندرويد.
	bEnableTouchEvents = true;
	bEnableTouchOverEvents = false;

#if PLATFORM_ANDROID || PLATFORM_IOS
	// لا مؤشر على الهاتف — إظهاره يرسم سهماً عالقاً في زاوية الشاشة.
	bShowMouseCursor = false;
#else
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
#endif
}

void ARok2PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ARok2GameMode* GM = Cast<ARok2GameMode>(GetWorld()->GetAuthGameMode()))
	{
		Api = GM->Api;
	}

	// Spawn iso camera if not in level
	if (!IsoCamera)
	{
		FVector Loc(0, 0, 1500);
		FRotator Rot(-50.f, 0.f, 0.f);
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		UClass* SpawnClass = CameraClass ? *CameraClass : ARok2IsometricCamera::StaticClass();
		IsoCamera = GetWorld()->SpawnActor<ARok2IsometricCamera>(SpawnClass, Loc, Rot, P);
		if (IsoCamera)
		{
			SetViewTargetWithBlend(IsoCamera, 0.f);
		}
	}
}

void ARok2PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis(TEXT("MoveRight"), this, &ARok2PlayerController::OnPanX);
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ARok2PlayerController::OnPanY);
	InputComponent->BindAxis(TEXT("Zoom"), this, &ARok2PlayerController::OnZoom);
	InputComponent->BindAction(TEXT("Tap"), IE_Pressed, this, &ARok2PlayerController::OnTap);
	InputComponent->BindAction(TEXT("Escape"), IE_Pressed, this, &ARok2PlayerController::OnEscape);

	// اللمس — المسار الوحيد الفعّال على أندرويد.
	InputComponent->BindTouch(IE_Pressed, this, &ARok2PlayerController::OnTouchBegin);
	InputComponent->BindTouch(IE_Repeat, this, &ARok2PlayerController::OnTouchMoved);
	InputComponent->BindTouch(IE_Released, this, &ARok2PlayerController::OnTouchEnd);
}

void ARok2PlayerController::OnTouchBegin(ETouchIndex::Type FingerIndex, FVector Location)
{
	const FVector2D Pos(Location.X, Location.Y);

	if (FingerIndex == ETouchIndex::Touch1)
	{
		Touch0Pos = Pos;
		bTouch0Active = true;
		bTouchMovedTooFarForTap = false;
		bPinching = false;
		TouchStartSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		Touch1Pos = Pos;
		bTouch1Active = true;
		bPinching = true;
		// قياس مرجعي أول، وإلا قفزت قيمة التكبير في الإطار الأول.
		LastPinchDistance = FVector2D::Distance(Touch0Pos, Touch1Pos);
		// إصبع ثانٍ يعني أن الإيماءة ليست نقرة.
		bTouchMovedTooFarForTap = true;
	}
}

void ARok2PlayerController::OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
	const FVector2D Pos(Location.X, Location.Y);

	if (FingerIndex == ETouchIndex::Touch1)
	{
		const FVector2D Delta = Pos - Touch0Pos;
		Touch0Pos = Pos;

		if (Delta.Size() > TapMoveThresholdPx)
		{
			bTouchMovedTooFarForTap = true;
		}

		// أثناء التكبير بإصبعين لا نحرّك الكاميرا — وإلا تصارعت الإيماءتان.
		if (!bPinching && IsoCamera)
		{
			IsoCamera->PanByScreenDelta(Delta);
		}
	}
	else if (FingerIndex == ETouchIndex::Touch2)
	{
		Touch1Pos = Pos;
	}

	if (bPinching && bTouch0Active && bTouch1Active && IsoCamera)
	{
		const float Distance = FVector2D::Distance(Touch0Pos, Touch1Pos);
		IsoCamera->ZoomByPinch(Distance - LastPinchDistance);
		LastPinchDistance = Distance;
	}
}

void ARok2PlayerController::OnTouchEnd(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (FingerIndex == ETouchIndex::Touch2)
	{
		bTouch1Active = false;
		bPinching = false;
		return;
	}

	if (FingerIndex != ETouchIndex::Touch1)
	{
		return;
	}

	bTouch0Active = false;

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const bool bWasQuick = (Now - TouchStartSeconds) <= TapMaxDurationSeconds;

	if (!bTouchMovedTooFarForTap && bWasQuick && !bTouch1Active)
	{
		HandleTapAtScreenPos(FVector2D(Location.X, Location.Y));
	}

	bPinching = false;
}

void ARok2PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsoCamera)
	{
		IsoCamera->AddPan(PanInput * DeltaSeconds);
		IsoCamera->AddZoom(ZoomInput * DeltaSeconds);
	}
}

void ARok2PlayerController::FocusOnPlayerCity()
{
	if (!IsoCamera || !Api) return;
	const FRok2Player& P = Api->GetPlayer();
	if (P.Id.IsEmpty()) return;
	// map world coords are in our own tile units; convert to UE cm.
	FVector Target(P.X * 100.f, P.Y * 100.f, 0.f);
	IsoCamera->FocusOn(Target);
}

void ARok2PlayerController::OnTap()
{
	// مسار الفأرة (حاسوب) — يحوّل إلى نفس منطق موضع الشاشة الذي يستخدمه اللمس.
	float MouseX = 0.f;
	float MouseY = 0.f;
	if (GetMousePosition(MouseX, MouseY))
	{
		HandleTapAtScreenPos(FVector2D(MouseX, MouseY));
	}
}

void ARok2PlayerController::HandleTapAtScreenPos(const FVector2D& ScreenPos)
{
	FHitResult HitResult;
	if (GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, true, HitResult))
	{
		if (!Api) return;

		FVector HitLoc = HitResult.ImpactPoint;
		float WorldX = HitLoc.X / 100.f;
		float WorldY = HitLoc.Y / 100.f;

		FString FoundType;
		FString FoundId;
		FString FoundName;
		float FoundX = 0;
		float FoundY = 0;
		float MinDistSq = 999999.f;

		const FRok2WorldSnapshot& W = Api->GetWorldSnapshot();

		for (const FRok2CityEntity& C : W.Cities)
		{
			if (C.PlayerId == Api->GetPlayer().Id) continue; 
			float DistSq = FVector2D::DistSquared(FVector2D(C.X, C.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = TEXT("city");
				FoundId = C.PlayerId;
				FoundName = C.Name;
				FoundX = C.X;
				FoundY = C.Y;
			}
		}

		for (const FRok2PassEntity& P : W.Passes)
		{
			float DistSq = FVector2D::DistSquared(FVector2D(P.X, P.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = TEXT("pass");
				FoundId = P.Id;
				FoundName = FString::Printf(TEXT("Pass Level %d"), P.Level);
				FoundX = P.X;
				FoundY = P.Y;
			}
		}

		for (const FRok2NodeEntity& N : W.Nodes)
		{
			float DistSq = FVector2D::DistSquared(FVector2D(N.X, N.Y), FVector2D(WorldX, WorldY));
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				FoundType = N.Kind == TEXT("barb") ? TEXT("barbarian") : TEXT("node");
				FoundId = N.Id;
				FoundName = FString::Printf(TEXT("%s Level %d"), *FoundType, N.Level);
				FoundX = N.X;
				FoundY = N.Y;
			}
		}

		if (MinDistSq < 100.f)
		{
			ARok2WorldRenderer* WorldRenderer = Cast<ARok2WorldRenderer>(UGameplayStatics::GetActorOfClass(GetWorld(), ARok2WorldRenderer::StaticClass()));
			if (!WorldRenderer || !WorldRenderer->CanInteractWithWorldTarget(FoundType, false))
			{
				return;
			}
			if (URok2MarchPanel* Panel = Cast<URok2MarchPanel>(URok2BlueprintLibrary::CreateRok2Widget(this, URok2MarchPanel::StaticClass())))
			{
				Panel->Api = Api;
				Panel->TargetType = FoundType;
				Panel->TargetId = FoundId;
				Panel->TargetName = FoundName;
				Panel->ToX = FoundX;
				Panel->ToY = FoundY;
				Panel->AddToViewport();
			}
		}
	}
}

void ARok2PlayerController::OnEscape()
{
	// could close UI panels
}
