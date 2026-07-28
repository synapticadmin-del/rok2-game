// Copyright ROK2. Player controller impl.

#include "Rok2PlayerController.h"
#include "Rok2GameMode.h"
#include "Rok2IsometricCamera.h"
#include "Rok2Api.h"
#include "Rok2Types.h"
#include "EngineUtils.h"
#include "Rok2MarchPanel.h"
#include "Rok2BlueprintLibrary.h"
#include "Blueprint/UserWidget.h"

ARok2PlayerController::ARok2PlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
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
	FHitResult HitResult;
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult))
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
