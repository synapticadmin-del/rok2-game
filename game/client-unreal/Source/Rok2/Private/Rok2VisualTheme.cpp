#include "Rok2VisualTheme.h"

namespace
{
	// ── أسطح ────────────────────────────────────────────────────────────────
	const FLinearColor GInk(0.04f, 0.07f, 0.14f, 1.0f);             // #0A1224
	const FLinearColor GPanel(0.10f, 0.07f, 0.04f, 0.97f);          // warm dark panel
	const FLinearColor GCard(0.16f, 0.12f, 0.07f, 1.0f);
	const FLinearColor GBar(0.08f, 0.05f, 0.03f, 0.96f);
	const FLinearColor GEdge(0.79f, 0.64f, 0.15f, 0.55f);           // ذهب شبه شفاف للحواف
	const FLinearColor GScrim(0.0f, 0.0f, 0.0f, 0.55f);

	// ── حشو ──────────────────────────────────────────────────────────────────
	const FLinearColor GGold(0.79f, 0.64f, 0.15f, 1.0f);            // #C9A227
	const FLinearColor GIvory(0.96f, 0.91f, 0.81f, 1.0f);           // #F5E9D0
	const FLinearColor GMuted(0.72f, 0.68f, 0.60f, 0.90f);
	const FLinearColor GTabInactive(0.55f, 0.50f, 0.42f, 1.0f);
	const FLinearColor GPrimaryAction(0.55f, 0.42f, 0.10f, 1.0f);
	const FLinearColor GSuccess(0.30f, 0.82f, 0.45f, 1.0f);
	const FLinearColor GDanger(0.92f, 0.30f, 0.26f, 1.0f);
	const FLinearColor GInformation(0.31f, 0.66f, 0.96f, 1.0f);

	// ── نص فوق داكن: نسخ مفتّحة. القيم مأخوذة من اللوحات المحلية التي كانت
	//    الودجات تعرّفها لهذا الغرض بالضبط (Rok2SeasonStory وأخواتها). ────────
	const FLinearColor GGoldText(1.00f, 0.84f, 0.20f, 1.0f);
	const FLinearColor GSuccessText(0.40f, 0.85f, 0.58f, 1.0f);
	const FLinearColor GDangerText(0.95f, 0.42f, 0.36f, 1.0f);
	const FLinearColor GInformationText(0.52f, 0.78f, 1.00f, 1.0f);

	// ── موارد ────────────────────────────────────────────────────────────────
	const FLinearColor GResFood(0.50f, 0.95f, 0.55f, 1.0f);
	const FLinearColor GResWood(0.85f, 0.65f, 0.40f, 1.0f);
	const FLinearColor GResStone(0.75f, 0.75f, 0.78f, 1.0f);
	const FLinearColor GResGold(0.79f, 0.64f, 0.15f, 1.0f);
	const FLinearColor GResGems(0.45f, 0.85f, 1.00f, 1.0f);
	const FLinearColor GResAp(0.75f, 0.55f, 1.00f, 1.0f);

	// ── نُدرة ─────────────────────────────────────────────────────────────────
	const FLinearColor GRarityAdvanced(0.20f, 0.70f, 0.30f, 1.0f);
	const FLinearColor GRarityElite(0.20f, 0.50f, 0.90f, 1.0f);
	const FLinearColor GRarityEpic(0.60f, 0.30f, 0.80f, 1.0f);
	const FLinearColor GRarityLegendary(0.95f, 0.60f, 0.10f, 1.0f);
}

const FLinearColor& Rok2Visual::Ink() { return GInk; }
const FLinearColor& Rok2Visual::Panel() { return GPanel; }
const FLinearColor& Rok2Visual::Card() { return GCard; }
const FLinearColor& Rok2Visual::Bar() { return GBar; }
const FLinearColor& Rok2Visual::Edge() { return GEdge; }
const FLinearColor& Rok2Visual::Scrim() { return GScrim; }

const FLinearColor& Rok2Visual::Gold() { return GGold; }
const FLinearColor& Rok2Visual::Ivory() { return GIvory; }
const FLinearColor& Rok2Visual::Muted() { return GMuted; }
const FLinearColor& Rok2Visual::TabInactive() { return GTabInactive; }
const FLinearColor& Rok2Visual::PrimaryAction() { return GPrimaryAction; }
const FLinearColor& Rok2Visual::Success() { return GSuccess; }
const FLinearColor& Rok2Visual::Danger() { return GDanger; }
const FLinearColor& Rok2Visual::Information() { return GInformation; }

const FLinearColor& Rok2Visual::GoldText() { return GGoldText; }
const FLinearColor& Rok2Visual::SuccessText() { return GSuccessText; }
const FLinearColor& Rok2Visual::DangerText() { return GDangerText; }
const FLinearColor& Rok2Visual::InformationText() { return GInformationText; }

const FLinearColor& Rok2Visual::ResourceFood() { return GResFood; }
const FLinearColor& Rok2Visual::ResourceWood() { return GResWood; }
const FLinearColor& Rok2Visual::ResourceStone() { return GResStone; }
const FLinearColor& Rok2Visual::ResourceGold() { return GResGold; }
const FLinearColor& Rok2Visual::ResourceGems() { return GResGems; }
const FLinearColor& Rok2Visual::ResourceActionPoints() { return GResAp; }

FLinearColor Rok2Visual::RarityTier(int32 Tier)
{
	switch (Tier)
	{
	case 4:  return GRarityLegendary;
	case 3:  return GRarityEpic;
	case 2:  return GRarityElite;
	case 1:
	default: return GRarityAdvanced;
	}
}

FLinearColor Rok2Visual::CivilizationAccent(const FString& CivilizationId)
{
	if (CivilizationId == TEXT("rome")) return FLinearColor(0.545f, 0.118f, 0.118f, 1.0f);
	if (CivilizationId == TEXT("china")) return FLinearColor(0.710f, 0.071f, 0.106f, 1.0f);
	if (CivilizationId == TEXT("arabia")) return FLinearColor(0.788f, 0.635f, 0.153f, 1.0f);
	if (CivilizationId == TEXT("egypt")) return FLinearColor(0.055f, 0.486f, 0.482f, 1.0f);
	if (CivilizationId == TEXT("vikings")) return FLinearColor(0.180f, 0.251f, 0.341f, 1.0f);
	if (CivilizationId == TEXT("japan")) return FLinearColor(0.608f, 0.114f, 0.125f, 1.0f);
	return Gold();
}
