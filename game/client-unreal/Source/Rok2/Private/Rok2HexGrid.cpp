#include "Rok2HexGrid.h"
#include "Kismet/KismetMathLibrary.h"

const TArray<FRok2HexCell>& URok2HexGrid::Directions()
{
	static const TArray<FRok2HexCell> Dirs = {
		FRok2HexCell(1, 0), FRok2HexCell(1, -1), FRok2HexCell(0, -1),
		FRok2HexCell(-1, 0), FRok2HexCell(-1, 1), FRok2HexCell(0, 1)
	};
	return Dirs;
}

FVector URok2HexGrid::HexToWorld(const FRok2HexCell& Cell, float Size)
{
	// pointy-top axial → world (x أفقي، y عمودي في مستوى الأرض)
	const float X = Size * FMath::Sqrt(3.f) * (Cell.Q + Cell.R * 0.5f);
	const float Y = Size * 1.5f * Cell.R;
	return FVector(X, Y, 0.f);
}

FRok2HexCell URok2HexGrid::WorldToHex(const FVector& World, float Size)
{
	const float Qf = (FMath::Sqrt(3.f) / 3.f * World.X - 1.f / 3.f * World.Y) / Size;
	const float Rf = (2.f / 3.f * World.Y) / Size;
	return RoundHex(Qf, Rf);
}

bool URok2HexGrid::IsInsideRadius(const FRok2HexCell& Cell, int32 Radius)
{
	const int32 M = FMath::Max3(FMath::Abs(Cell.Q), FMath::Abs(Cell.R), FMath::Abs(Cell.S()));
	return M <= Radius;
}

TArray<FRok2HexCell> URok2HexGrid::Ring(int32 Radius)
{
	TArray<FRok2HexCell> Out;
	if (Radius == 0)
	{
		Out.Add(FRok2HexCell(0, 0));
		return Out;
	}
	// نبدأ من الاتجاه الرابع × Radius ونسير حول الحلقة بكل الاتجاهات
	FRok2HexCell Cell = FRok2HexCell(Directions()[4].Q * Radius, Directions()[4].R * Radius);
	for (int32 i = 0; i < 6; ++i)
	{
		for (int32 j = 0; j < Radius; ++j)
		{
			Out.Add(Cell);
			Cell = FRok2HexCell(Cell.Q + Directions()[i].Q, Cell.R + Directions()[i].R);
		}
	}
	return Out;
}

TArray<FRok2HexCell> URok2HexGrid::FilledHexagon(int32 Radius)
{
	TArray<FRok2HexCell> Out;
	for (int32 q = -Radius; q <= Radius; ++q)
	{
		const int32 r1 = FMath::Max(-Radius, -q - Radius);
		const int32 r2 = FMath::Min(Radius, -q + Radius);
		for (int32 r = r1; r <= r2; ++r)
		{
			Out.Add(FRok2HexCell(q, r));
		}
	}
	return Out;
}

TArray<FRok2HexCell> URok2HexGrid::Flower(const FRok2HexCell& Center)
{
	TArray<FRok2HexCell> Out;
	Out.Add(Center);
	for (const FRok2HexCell& D : Directions())
	{
		Out.Add(FRok2HexCell(Center.Q + D.Q, Center.R + D.R));
	}
	return Out;
}

TArray<FRok2HexCell> URok2HexGrid::Neighbors(const FRok2HexCell& Cell)
{
	TArray<FRok2HexCell> Out;
	for (const FRok2HexCell& D : Directions())
	{
		Out.Add(FRok2HexCell(Cell.Q + D.Q, Cell.R + D.R));
	}
	return Out;
}

FRok2HexCell URok2HexGrid::RoundHex(float Qf, float Rf)
{
	const float Sf = -Qf - Rf;
	int32 Q = FMath::RoundToInt(Qf);
	int32 R = FMath::RoundToInt(Rf);
	int32 S = FMath::RoundToInt(Sf);
	const float dQ = FMath::Abs(Q - Qf);
	const float dR = FMath::Abs(R - Rf);
	const float dS = FMath::Abs(S - Sf);
	if (dQ > dR && dQ > dS)
	{
		Q = -R - S;
	}
	else if (dR > dS)
	{
		R = -Q - S;
	}
	return FRok2HexCell(Q, R);
}
