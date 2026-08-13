import mapSpec from "../data/map_spec_coordinates.json";
import civilizations from "../data/civilizations.json";
import buildings from "../data/buildings.json";
import troopTiers from "../data/troop_tiers.json";
import commanders from "../data/commanders.json";
import techTree from "../data/research.json";
import zonesSpec from "../data/zones.json";
import shopSpec from "../data/shop.json";
import softLaunchSpec from "../data/softlaunch.json";
import battlePassSpec from "../data/battlepass.json";
import chatSpec from "../data/chat.json";
import allianceStructuresSpec from "../data/alliance_structures.json";

export const MAP_SCALE = 0.5; // 2400 -> 1200 prototype world

export type MapPass = {
  id: string;
  from: string;
  to: string;
  center: [number, number];
  level: number;
  unlock_day?: number;
  type?: string;
  zone_link?: number[];
};

export type MapRegion = {
  id: string;
  zone_id: number;
  name: string;
  aabb: [number, number, number, number];
  spawn_anchor?: [number, number];
  altar?: { id: string; pos: [number, number]; buff: string };
};

function scalePoint(p: [number, number]): [number, number] {
  return [p[0] * MAP_SCALE, p[1] * MAP_SCALE];
}

function scaleAabb(a: [number, number, number, number]): [number, number, number, number] {
  return [a[0] * MAP_SCALE, a[1] * MAP_SCALE, a[2] * MAP_SCALE, a[3] * MAP_SCALE];
}

export function getMap() {
  const regions = (mapSpec.regions as MapRegion[]).map((r) => ({
    ...r,
    aabb: scaleAabb(r.aabb as [number, number, number, number]),
    spawn_anchor: r.spawn_anchor ? scalePoint(r.spawn_anchor as [number, number]) : undefined,
    altar: r.altar
      ? { ...r.altar, pos: scalePoint(r.altar.pos as [number, number]) }
      : undefined,
  }));

  const passes = (mapSpec.passes as MapPass[]).map((p) => ({
    ...p,
    center: scalePoint(p.center as [number, number]),
  }));

  return {
    mapId: mapSpec.map_id,
    width: (mapSpec.units as any).world_width * MAP_SCALE,
    height: (mapSpec.units as any).world_height * MAP_SCALE,
    constants: {
      mountainBelt: (mapSpec.constants as any).mountain_belt_thickness * MAP_SCALE,
      passWidth: (mapSpec.constants as any).pass_opening_width * MAP_SCALE,
      teleportBlock: (mapSpec.constants as any).pass_teleport_block_radius * MAP_SCALE,
    },
    zones: mapSpec.zones,
    regions,
    passes,
    zone3: {
      throne: {
        ...(mapSpec.zone3_objectives as any).throne,
        pos: scalePoint((mapSpec.zone3_objectives as any).throne.pos),
      },
      outer_forts: ((mapSpec.zone3_objectives as any).outer_forts as any[]).map((f) => ({
        ...f,
        pos: scalePoint(f.pos),
      })),
      side_altars: ((mapSpec.zone3_objectives as any).side_altars as any[]).map((f) => ({
        ...f,
        pos: scalePoint(f.pos),
      })),
    },
    season_unlock_schedule: mapSpec.season_unlock_schedule,
  };
}

export function getCivilizations() {
  return civilizations;
}

export function getBuildings() {
  return buildings;
}

export function getTroops() {
  return troopTiers;
}

/** منشآت التحالف التفاعلية: كتالوج موحّد للبناء والنطاقات المرئية وقواعد الحماية. */
export function getAllianceStructures() {
  return allianceStructuresSpec;
}

/** P2-T4: مواصفة المناطق الموحدة (فتح زمني + نطاقات موارد) — تُقرأ من data/zones.json */
export function getZones() {
  return zonesSpec;
}

/** P3-T4: كتالوج المتجر (speedups + مستويات VIP + ثوابت gems) — يُقرأ من data/shop.json */
export function getShop() {
  return shopSpec;
}

/** P3-T5: إعدادات Soft launch (ممالك محدودة + عتبات retention) — تُقرأ من data/softlaunch.json */
export function getSoftLaunch() {
  return softLaunchSpec;
}

export function starterBuildings(): string[] {
  return [
    "city_hall",
    "farm",
    "lumber_mill",
    "quarry",
    "goldmine",
    "barracks",
    "stable",
    "archery_range",
    "hospital",
    "wall",
    "storehouse",
  ];
}

/** Production per hour at building level */
export function productionPerHour(buildingId: string, level: number): number {
  const base: Record<string, number> = {
    farm: 100,
    lumber_mill: 100,
    quarry: 70,
    goldmine: 40,
  };
  if (!base[buildingId]) return 0;
  return base[buildingId] * Math.pow(1.2, Math.max(0, level - 1));
}

export function upgradeCost(buildingId: string, nextLevel: number) {
  const mult = Math.pow(1.55, nextLevel - 1);
  return {
    food: Math.floor(200 * mult),
    wood: Math.floor(200 * mult),
    stone: Math.floor(120 * mult),
    gold: Math.floor(80 * mult),
  };
}

/**
 * مدة ترقية مبنى بالثواني. تعتمد على المستوى المستهدف وتُطبق سرعة البناء
 * السلطوية هنا حتى لا يرسل العميل مدة قابلة للتلاعب.
 */
export function buildingUpgradeDurationSec(nextLevel: number, buildSpeedMultiplier = 1): number {
  const normalizedMultiplier = Math.max(0.1, Number(buildSpeedMultiplier) || 1);
  const baseSeconds = 30 * Math.pow(1.35, Math.max(0, nextLevel - 1));
  return Math.max(5, Math.ceil(baseSeconds / normalizedMultiplier));
}

/** معدلات الموارد الأربع في الساعة قبل تحويلها إلى تسوية زمنية. */
export function resourceProductionRates(
  buildingsById: Record<string, number>,
  productionMultiplier = 1,
): { food: number; wood: number; stone: number; gold: number } {
  const normalizedMultiplier = Math.max(0, Number(productionMultiplier) || 0);
  return {
    food: productionPerHour("farm", buildingsById.farm || 0) * normalizedMultiplier,
    wood: productionPerHour("lumber_mill", buildingsById.lumber_mill || 0) * normalizedMultiplier,
    stone: productionPerHour("quarry", buildingsById.quarry || 0) * normalizedMultiplier,
    gold: productionPerHour("goldmine", buildingsById.goldmine || 0) * normalizedMultiplier,
  };
}

// P8-T3: تكلفة التدريب من troop_tiers.json بلا hard-code — الوحدة الخاصة تستخدم تكلفة فرعها.
export function trainCost(unitId: string, count: number) {
  const fallback: Record<string, { food: number; wood: number; stone: number; gold: number }> = {
    infantry_t1: { food: 50, wood: 20, stone: 0, gold: 0 },
    cavalry_t1: { food: 60, wood: 40, stone: 0, gold: 10 },
    archer_t1: { food: 40, wood: 50, stone: 0, gold: 5 },
  };
  const s = troopTierStatsRaw(unitId);
  const c = s ? normalizeCost(s.train_cost) : (fallback[unitId] || fallback.infantry_t1);
  return {
    food: c.food * count,
    wood: c.wood * count,
    stone: c.stone * count,
    gold: c.gold * count,
  };
}

// P8-T3: مدة التدريب بالثواني من troop_tiers.json (train_time × count، مقسوم على مضاعف السرعة).
export function trainDurationSec(unitId: string, count: number, speedMult = 1): number {
  const s = troopTierStatsRaw(unitId);
  const base = s ? Number(s.train_time) || 10 : 10;
  const norm = Math.max(0.1, Number(speedMult) || 1);
  return Math.max(1, Math.ceil((base * Math.max(1, count)) / 10 / norm));
}

function normalizeCost(raw: unknown): { food: number; wood: number; stone: number; gold: number } {
  const c = (raw || {}) as Record<string, number>;
  return {
    food: Number(c.food) || 0,
    wood: Number(c.wood) || 0,
    stone: Number(c.stone) || 0,
    gold: Number(c.gold) || 0,
  };
}

function troopTierStatsRaw(unitId: string): { train_cost: unknown; train_time: unknown } | null {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  if (!m) return null;
  const branch = m[1];
  const tier = Number(m[2]);
  const t = (troopTiers as any).tiers?.find((x: any) => x.tier === tier);
  return t?.stats?.[branch] || null;
}

// P8-T3: قوة الوحدة تُقرأ من troop_tiers.json (قوة الهجوم = troopPower الأساس) بلا hard-code.
export function unitPower(unitId: string): number {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  if (!m) return 10;
  const t = (troopTiers as any).tiers?.find((x: any) => x.tier === Number(m[2]));
  const s = t?.stats?.[m[1]];
  return s ? Number(s.attack) : 10;
}

export function getCommanders() {
  return commanders;
}

export function getTechTree() {
  return techTree;
}

/** P4-T1: مواصفة Battle Pass (مستويات + مكافآت + نقاط الأفعال) — تُقرأ من data/battlepass.json */
export function getBattlePass() {
  return battlePassSpec;
}

/** P6-T6: مواصفة الدردشة الحية (قنوات + حدود + ثوابت واجهة) — تُقرأ من data/chat.json */
export function getChatConfig() {
  return chatSpec;
}
