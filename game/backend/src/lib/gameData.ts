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

export function trainCost(unitId: string, count: number) {
  const unitCosts: Record<string, { food: number; wood: number; stone: number; gold: number }> = {
    infantry_t1: { food: 50, wood: 20, stone: 0, gold: 0 },
    cavalry_t1: { food: 60, wood: 40, stone: 0, gold: 10 },
    archer_t1: { food: 40, wood: 50, stone: 0, gold: 5 },
    infantry_t2: { food: 100, wood: 40, stone: 0, gold: 0 },
    cavalry_t2: { food: 120, wood: 80, stone: 0, gold: 20 },
    archer_t2: { food: 80, wood: 100, stone: 0, gold: 10 },
    infantry_t3: { food: 200, wood: 80, stone: 0, gold: 0 },
    cavalry_t3: { food: 240, wood: 160, stone: 0, gold: 40 },
    archer_t3: { food: 160, wood: 200, stone: 0, gold: 20 },
    infantry_t4: { food: 400, wood: 160, stone: 0, gold: 0 },
    cavalry_t4: { food: 480, wood: 320, stone: 0, gold: 80 },
    archer_t4: { food: 320, wood: 400, stone: 0, gold: 40 },
  };
  const c = unitCosts[unitId] || unitCosts.infantry_t1;
  return {
    food: c.food * count,
    wood: c.wood * count,
    stone: c.stone * count,
    gold: c.gold * count,
  };
}

export function unitPower(unitId: string): number {
  let base = 10;
  if (unitId.includes("cavalry")) base = 12;
  else if (unitId.includes("archer")) base = 11;
  
  if (unitId.includes("_t2")) return base * 1.5;
  if (unitId.includes("_t3")) return base * 2.2;
  if (unitId.includes("_t4")) return base * 3.5;
  return base * 1.0;
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
