#include "Rok2CityEditorMode.h"
#include "Rok2BuildingActor.h"
#include "Components/StaticMeshComponent.h"

ARok2CityEditorMode::ARok2CityEditorMode()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ARok2CityEditorMode::BeginPlay()
{
	Super::BeginPlay();

	// ثلاث خانات افتراضية
	SavedLayouts.SetNum(3);
	SavedLayouts[0].Name = TEXT("عسكري");
	SavedLayouts[1].Name = TEXT("إنتاج");
	SavedLayouts[2].Name = TEXT("حر");
}

void ARok2CityEditorMode::SetLayout(ARok2CityLayoutActor* InLayout)
{
	Layout = InLayout;
}

void ARok2CityEditorMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ARok2CityEditorMode::EnterEditMode()
{
	if (!Layout) return;
	bActive = true;
	Layout->SetEditMode(true);
}

void ARok2CityEditorMode::ExitEditMode(bool bSaveChanges)
{
	if (!Layout) return;
	EndDrag();
	bActive = false;
	Layout->SetEditMode(false);
	if (bSaveChanges)
	{
		Layout->SaveLayoutToServer();
	}
}

void ARok2CityEditorMode::BeginDrag(const FString& BuildingId)
{
	if (!bActive || !Layout) return;
	ARok2BuildingActor** Found = Layout->Buildings.Find(BuildingId);
	if (!Found || !*Found) return;
	ARok2BuildingActor* B = *Found;
	if (B->bIsStatic) return; // city_hall لا يُسحب

	DraggedBuilding = B;
	DragOriginCell = B->AnchorCell;
	SpawnGhost();
}

void ARok2CityEditorMode::SpawnGhost()
{
	DestroyGhost();
	if (!DraggedBuilding || !Layout) return;

	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GhostBuilding = GetWorld()->SpawnActor<ARok2BuildingActor>(DraggedBuilding->GetActorLocation(), DraggedBuilding->GetActorRotation(), P);
	if (GhostBuilding)
	{
		GhostBuilding->Setup(DraggedBuilding->BuildingId, DraggedBuilding->Level, DraggedBuilding->AnchorCell, Layout->HexSize);
		GhostBuilding->Footprint = DraggedBuilding->Footprint;
		GhostBuilding->SetActorEnableCollision(false);
		if (GhostBuilding->Mesh)
		{
			GhostBuilding->Mesh->SetStaticMesh(DraggedBuilding->Mesh->GetStaticMesh());
			GhostBuilding->Mesh->SetWorldScale3D(DraggedBuilding->Mesh->GetComponentScale());
		}
		// شبح: شفافية عبر إخفاء المبنى الأصلي مؤقتاً وإظهار الشبح باهت
		DraggedBuilding->SetActorHiddenInGame(true);
	}
	UpdateGhostTint();
}

void ARok2CityEditorMode::DestroyGhost()
{
	if (GhostBuilding)
	{
		GhostBuilding->Destroy();
		GhostBuilding = nullptr;
	}
	if (DraggedBuilding)
	{
		DraggedBuilding->SetActorHiddenInGame(false);
	}
}

void ARok2CityEditorMode::UpdateDrag(const FVector& WorldLocation)
{
	if (!bActive || !DraggedBuilding || !Layout || !GhostBuilding) return;

	// اعرض الشبح عند أقرب خلية
	HoverCell = URok2HexGrid::WorldToHex(WorldLocation, Layout->HexSize);
	const FVector Snapped = URok2HexGrid::HexToWorld(HoverCell, Layout->HexSize);
	GhostBuilding->SetActorLocation(Snapped);

	// صلاحية الموضع: نفترض الصلاحية والتحقق النهائي في EndDrag عبر TryMoveBuilding
	bHoverValid = true;
	UpdateGhostTint();
}

void ARok2CityEditorMode::UpdateGhostTint()
{
	// التلوين الأخضر/الأحمر يُدار من مادة الأصل عبر Tag للمستوى ليضبط Parameter.
	if (GhostBuilding)
	{
		GhostBuilding->Tags.Empty();
		GhostBuilding->Tags.Add(bHoverValid ? FName(TEXT("ghost_valid")) : FName(TEXT("ghost_invalid")));
	}
}

void ARok2CityEditorMode::EndDrag()
{
	if (!DraggedBuilding || !Layout)
	{
		DestroyGhost();
		DraggedBuilding = nullptr;
		return;
	}

	// حاول النقل للخلية الحالية — TryMoveBuilding يتحقق من الصلاحية ويعيد false عند الفشل
	const FString Id = DraggedBuilding->BuildingId;
	const bool bMoved = Layout->TryMoveBuilding(Id, HoverCell);
	if (!bMoved)
	{
		// رجوع للأصل
		Layout->TryMoveBuilding(Id, DragOriginCell);
	}

	DestroyGhost();
	DraggedBuilding = nullptr;
}

void ARok2CityEditorMode::RotateDragged()
{
	if (DraggedBuilding && Layout)
	{
		Layout->RotateBuilding(DraggedBuilding->BuildingId);
		if (GhostBuilding)
		{
			GhostBuilding->SetActorRotation(DraggedBuilding->GetActorRotation());
		}
	}
}

void ARok2CityEditorMode::SaveCurrentLayout(int32 Slot, const FString& Name)
{
	if (!Layout || !SavedLayouts.IsValidIndex(Slot)) return;
	FRok2SavedLayout& L = SavedLayouts[Slot];
	L.Name = Name;
	L.Placements = Layout->GetLayoutPlacements();
}

void ARok2CityEditorMode::LoadLayout(int32 Slot)
{
	if (!Layout || !SavedLayouts.IsValidIndex(Slot)) return;
	const FRok2SavedLayout& L = SavedLayouts[Slot];
	for (const FRok2BuildingPlacement& P : L.Placements)
	{
		if (Layout->Buildings.Contains(P.BuildingId))
		{
			Layout->TryMoveBuilding(P.BuildingId, FRok2HexCell(P.Q, P.R));
			// ضبط الدوران
			ARok2BuildingActor* B = Layout->Buildings[P.BuildingId];
			if (B)
			{
				B->RotationSteps = FMath::Abs(P.RotationSteps) % 6;
				B->SetActorRotation(FRotator(0.f, B->RotationSteps * 60.f, 0.f));
				if (!B->bIsStatic)
				{
					B->SetFacade(P.Facade);
				}
			}
		}
	}
	Layout->SaveLayoutToServer();
}
