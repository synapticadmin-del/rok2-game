// Copyright ROK2. World marker iconography (P6-T9).

#include "Rok2WorldIconography.h"

namespace Rok2WorldIconography
{
	static FLinearColor LevelColor(int32 Tier)
	{
		switch (Tier)
		{
		case 3: return FLinearColor(1.00f, 0.75f, 0.20f, 1.f); // final / elite gold
		case 2: return FLinearColor(0.42f, 0.78f, 1.00f, 1.f); // contested azure
		default: return FLinearColor(0.74f, 0.79f, 0.84f, 1.f); // entry silver
		}
	}

	static FRok2WorldIconStyle MakeStyle(FName Id, const TCHAR* Glyph, FLinearColor Base, int32 Tier, float Scale)
	{
		FRok2WorldIconStyle Style;
		Style.IconId = Id;
		Style.Glyph = Glyph;
		Style.BaseColor = Base;
		Style.Tier = Tier;
		Style.TierColor = LevelColor(Tier);
		Style.WorldScale = Scale;
		return Style;
	}
}

int32 URok2WorldIconography::TierForLevel(int32 Level, int32 MaximumLevel)
{
	const int32 SafeMax = FMath::Max(1, MaximumLevel);
	const int32 SafeLevel = FMath::Clamp(Level, 1, SafeMax);
	const float Ratio = static_cast<float>(SafeLevel) / static_cast<float>(SafeMax);
	return Ratio > 0.66f ? 3 : (Ratio > 0.33f ? 2 : 1);
}

FName URok2WorldIconography::ResourceIconId(const FString& ResourceKind)
{
	const FString Kind = ResourceKind.ToLower();
	if (Kind == TEXT("food") || Kind == TEXT("wheat")) return TEXT("node_wheat");
	if (Kind == TEXT("wood") || Kind == TEXT("timber")) return TEXT("node_wood");
	if (Kind == TEXT("stone") || Kind == TEXT("ore")) return TEXT("node_stone");
	if (Kind == TEXT("gold")) return TEXT("node_gold");
	return TEXT("node_resource");
}

FRok2WorldIconStyle URok2WorldIconography::Resolve(const FString& TargetType, const FString& ResourceKind, int32 Level)
{
	const FString Type = TargetType.ToLower();
	const FString Kind = ResourceKind.ToLower();

	if (Type == TEXT("throne") || Type == TEXT("core_objective") || Kind == TEXT("throne"))
	{
		return Rok2WorldIconography::MakeStyle(TEXT("objective_throne_crown"), TEXT("♛"), FLinearColor(0.98f, 0.70f, 0.14f, 1.f), 3, 1.55f);
	}

	if (Type == TEXT("pass") || Type == TEXT("gate"))
	{
		const int32 Tier = TierForLevel(Level, 5);
		return Rok2WorldIconography::MakeStyle(TEXT("objective_pass_gate"), TEXT("⌑"), FLinearColor(0.42f, 0.50f, 0.60f, 1.f), Tier, 1.08f + 0.09f * Tier);
	}

	if (Type == TEXT("barb") || Type == TEXT("barbarian"))
	{
		const int32 Tier = TierForLevel(Level, 6);
		const FName Id = Tier == 3 ? TEXT("barbarian_elite") : (Tier == 2 ? TEXT("barbarian_warband") : TEXT("barbarian_scout"));
		const TCHAR* Glyph = Tier == 3 ? TEXT("☠") : (Tier == 2 ? TEXT("⚔") : TEXT("⚑"));
		return Rok2WorldIconography::MakeStyle(Id, Glyph, FLinearColor(0.73f, 0.20f, 0.16f, 1.f), Tier, 1.03f + 0.10f * Tier);
	}

	const int32 Tier = TierForLevel(Level, 6);
	const FName ResourceId = ResourceIconId(Kind.IsEmpty() ? Type : Kind);
	if (ResourceId == TEXT("node_wheat"))
	{
		return Rok2WorldIconography::MakeStyle(ResourceId, TEXT("♜"), FLinearColor(0.90f, 0.68f, 0.20f, 1.f), Tier, 0.96f + 0.08f * Tier);
	}
	if (ResourceId == TEXT("node_wood"))
	{
		return Rok2WorldIconography::MakeStyle(ResourceId, TEXT("♣"), FLinearColor(0.20f, 0.56f, 0.30f, 1.f), Tier, 0.96f + 0.08f * Tier);
	}
	if (ResourceId == TEXT("node_stone"))
	{
		return Rok2WorldIconography::MakeStyle(ResourceId, TEXT("◆"), FLinearColor(0.52f, 0.58f, 0.65f, 1.f), Tier, 0.96f + 0.08f * Tier);
	}
	if (ResourceId == TEXT("node_gold"))
	{
		return Rok2WorldIconography::MakeStyle(ResourceId, TEXT("●"), FLinearColor(0.96f, 0.62f, 0.08f, 1.f), Tier, 0.96f + 0.08f * Tier);
	}

	return Rok2WorldIconography::MakeStyle(TEXT("world_marker"), TEXT("•"), FLinearColor(0.70f, 0.76f, 0.80f, 1.f), Tier, 1.f);
}
