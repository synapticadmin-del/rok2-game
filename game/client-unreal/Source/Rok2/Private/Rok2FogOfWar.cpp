// Copyright ROK2. Fog of War + Scouts system (P5-T5) — implementation.

#include "Rok2FogOfWar.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Fog, Log, All);

URok2FogOfWar* URok2FogOfWar::Get()
{
	static URok2FogOfWar* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2FogOfWar>();
		Instance->AddToRoot();
	}
	return Instance;
}

void URok2FogOfWar::Init(int32 MapWidth, int32 MapHeight, int32 InCellSize)
{
	CellSize = InCellSize;
	GridWidth = FMath::CeilToInt((float)MapWidth / (float)CellSize);
	GridHeight = FMath::CeilToInt((float)MapHeight / (float)CellSize);

	FogGrid.Init(0, GridWidth * GridHeight);
	bInitialized = true;

	UE_LOG(LogRok2Fog, Log, TEXT("FogOfWar initialized: %dx%d cells (cell size %d)"), GridWidth, GridHeight, CellSize);
}

int32 URok2FogOfWar::WorldToGridIndex(double WorldX, double WorldY) const
{
	if (!bInitialized || GridWidth <= 0) return -1;

	const int32 CellX = FMath::FloorToInt((float)(WorldX / (double)CellSize));
	const int32 CellY = FMath::FloorToInt((float)(WorldY / (double)CellSize));

	if (!IsValidCell(CellX, CellY)) return -1;
	return CellY * GridWidth + CellX;
}

bool URok2FogOfWar::IsValidCell(int32 CellX, int32 CellY) const
{
	return CellX >= 0 && CellX < GridWidth && CellY >= 0 && CellY < GridHeight;
}

void URok2FogOfWar::RevealCell(int32 CellX, int32 CellY, bool bFullReveal)
{
	if (!IsValidCell(CellX, CellY)) return;

	const int32 Idx = CellY * GridWidth + CellX;
	if (bFullReveal)
	{
		FogGrid[Idx] = (uint8)ERok2FogState::Explored;
	}
	else if (FogGrid[Idx] == (uint8)ERok2FogState::Unexplored)
	{
		FogGrid[Idx] = (uint8)ERok2FogState::Partially;
	}
}

void URok2FogOfWar::RevealArea(double CenterX, double CenterY, int32 RadiusCells, bool bFullReveal)
{
	if (!bInitialized) return;

	const int32 CenterCellX = FMath::FloorToInt((float)(CenterX / (double)CellSize));
	const int32 CenterCellY = FMath::FloorToInt((float)(CenterY / (double)CellSize));

	for (int32 dy = -RadiusCells; dy <= RadiusCells; ++dy)
	{
		for (int32 dx = -RadiusCells; dx <= RadiusCells; ++dx)
		{
			// دائرة (وليس مربع)
			if (dx * dx + dy * dy <= RadiusCells * RadiusCells)
			{
				RevealCell(CenterCellX + dx, CenterCellY + dy, bFullReveal);
			}
		}
	}

	OnFogUpdated.Broadcast(FogGrid);
}

ERok2FogState URok2FogOfWar::GetFogStateAt(double WorldX, double WorldY) const
{
	const int32 Idx = WorldToGridIndex(WorldX, WorldY);
	if (Idx < 0 || Idx >= FogGrid.Num()) return ERok2FogState::Unexplored;
	return (ERok2FogState)FogGrid[Idx];
}

bool URok2FogOfWar::IsExplored(double WorldX, double WorldY) const
{
	return GetFogStateAt(WorldX, WorldY) != ERok2FogState::Unexplored;
}

void URok2FogOfWar::AddScout(const FRok2Scout& Scout)
{
	// لا تكرار
	for (const FRok2Scout& S : Scouts)
	{
		if (S.Id == Scout.Id) return;
	}
	Scouts.Add(Scout);
	UE_LOG(LogRok2Fog, Log, TEXT("Scout %s added: (%.0f,%.0f) -> (%.0f,%.0f) ETA %lld"),
		*Scout.Id, Scout.FromX, Scout.FromY, Scout.ToX, Scout.ToY, Scout.EtaMs);
}

void URok2FogOfWar::UpdateScouts(int64 NowMs)
{
	TArray<FString> ToRemove;

	for (FRok2Scout& S : Scouts)
	{
		if (!S.bArrived && S.EtaMs > 0 && NowMs >= S.EtaMs)
		{
			S.bArrived = true;

			// كشف المنطقة حول نقطة الوصول
			RevealArea(S.ToX, S.ToY, ScoutRevealRadius, true);

			UE_LOG(LogRok2Fog, Log, TEXT("Scout %s arrived at (%.0f,%.0f) — area revealed"), *S.Id, S.ToX, S.ToY);

			OnScoutArrived.Broadcast(S);
			ToRemove.Add(S.Id);
		}
	}

	for (const FString& Id : ToRemove)
	{
		RemoveScout(Id);
	}
}

void URok2FogOfWar::RemoveScout(const FString& ScoutId)
{
	Scouts.RemoveAll([&](const FRok2Scout& S) { return S.Id == ScoutId; });
}
