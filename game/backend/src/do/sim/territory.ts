/**
 * P9-T2: أراضي التحالف ومراكز الموارد — منطق نقي بلا اعتماد على node:fs ولا الخادم.
 * كل الثوابت تُقرأ من data/alliance_territory.json.
 *
 * المفاهيم:
 * - كل قلعة (flag/outpost) تنشر نطاقًا إقليميًا حولها.
 * - جمع الموارد داخل النطاق يحصل على multiplier (باف +25%).
 * - المسيرات البربرية العابرة للنطاق تُخفَّف أضرارها (patrol reduction).
 * - مراكز الموارد (granary/wood_lot/stone_pit/mother_lode) غير قابلة للهجوم،
 *   تجمع آمن +25% وتُقفل على التحالف الذي جمَع منها لدقائق قليلة، ثم تعاد
 *   تعبئتها دوريًا طوال الموسم (respawn).
 */

import territoryData from "../../data/alliance_territory.json";

export type CastleEntity = {
  id: string;
  allianceId: string | null;
  x: number;
  y: number;
  radius?: number;
  kind: "flag" | "outpost";
};

export type CenterEntity = {
  id: string;
  kind: "granary" | "wood_lot" | "stone_pit" | "mother_lode";
  x: number;
  y: number;
  radius: number;
  lockedAllianceId: string | null;
  lockedUntilMs: number | null;
  reserve: number;
  spawnedSeasonDay: number;
};

type RawCenter = {
  id: string;
  kind: string;
  x: number;
  y: number;
  radius: number;
  locked_alliance_id: string | null;
  locked_until_ms: number | null;
  reserve: number;
  spawned_season_day: number;
};

const TD = territoryData as any;

export const TERRITORY_CFG = TD.territory as {
  flag_radius: number;
  outpost_radius: number;
  gather_multiplier: number;
  patrol_reduction: number;
  seed_radius_min: number;
  center: {
    gather_capacity: number;
    gather_per_tick: string;
    spawn_days: string;
    respawn_interval_days: number;
    lock_minutes: number;
    kinds: string[];
  };
  outpost: {
    max_outposts_per_alliance: number;
    min_player_count: number;
    min_total_power: number;
    min_hall_level: number;
  };
};

export const CENTER_SEEDS: [number, number][] = (() => {
  const out: [number, number][] = [];
  const seen = new Set<string>();
  const kinds = (TD.territory.center.kinds as string[]) || [];
  for (const kind of kinds) {
    const pts = (TD.seeds?.[kind] as number[][]) || [];
    for (const p of pts) {
      const k = `${p[0]},${p[1]}`;
      if (seen.has(k) || out.length >= 14) continue;
      seen.add(k);
      out.push([p[0], p[1]] as [number, number]);
    }
  }
  return out;
})();

const CENTER_RADIUS = 120;
const MAP_MIN = 0;
const MAP_MAX = 8000;

export function flagRadius(): number {
  return TERRITORY_CFG.flag_radius;
}

export function outpostRadius(): number {
  return TERRITORY_CFG.outpost_radius;
}

export function gatherBonus(): number {
  return TERRITORY_CFG.gather_multiplier - 1;
}

export function gatherMultiplier(): number {
  return TERRITORY_CFG.gather_multiplier;
}

export function patrolReduction(): number {
  return TERRITORY_CFG.patrol_reduction;
}

/** هل النقطة داخل نطاق أي قلعة للتحالف؟ */
export function insideTerritory(
  x: number,
  y: number,
  castles: CastleEntity[],
  allianceId: string | null,
): boolean {
  if (!allianceId) return false;
  for (const c of castles) {
    if (c.allianceId !== allianceId) continue;
    const r = c.kind === "outpost" ? outpostRadius() : flagRadius();
    const dx = c.x - x;
    const dy = c.y - y;
    if (dx * dx + dy * dy <= r * r) return true;
  }
  return false;
}

/** تخفيض أضرار البرابرة لممر يعبر أرض التحالف. */
export function patrolMod(crosses: boolean): number {
  return crosses ? 1 - patrolReduction() : 1;
}

/** هل الممر يمر داخل نطاق قلعة للتحالف؟ */
export function marchCrossesTerritory(
  fromX: number,
  fromY: number,
  toX: number,
  toY: number,
  castles: CastleEntity[],
  allianceId: string | null,
): boolean {
  if (!allianceId) return false;
  // أخذ عينات على طول خط الممر (8 نقاط) — تقريب مقبول لنطاقات كبيرة.
  for (let i = 1; i <= 8; i++) {
    const t = i / 9;
    const x = fromX + (toX - fromX) * t;
    const y = fromY + (toY - fromY) * t;
    if (insideTerritory(x, y, castles, allianceId)) return true;
  }
  return false;
}

/** شروط بناء outpost للتحالف (playerCount/power/hallLevel) من JSON. */
export function canBuildOutpost(
  allianceOutposts: number,
  minPlayerCount: number,
  minTotalPower: number,
  minHallLevel: number,
): boolean {
  return (
    allianceOutposts < TERRITORY_CFG.outpost.max_outposts_per_alliance &&
    minPlayerCount >= TERRITORY_CFG.outpost.min_player_count &&
    minTotalPower >= TERRITORY_CFG.outpost.min_total_power &&
    minHallLevel >= TERRITORY_CFG.outpost.min_hall_level
  );
}

/** هل الموضع صالح لمركز مورد جديد (بعيد عن القلاع والمراكز الأخرى)؟ */
export function validPosition(
  x: number,
  y: number,
  castles: CastleEntity[],
  centers: CenterEntity[],
): boolean {
  if (x < MAP_MIN || x > MAP_MAX || y < MAP_MIN || y > MAP_MAX) return false;
  const minR = TERRITORY_CFG.seed_radius_min;
  for (const c of castles) {
    const r = (c.kind === "outpost" ? outpostRadius() : flagRadius()) + minR / 2;
    const dx = c.x - x;
    const dy = c.y - y;
    if (dx * dx + dy * dy <= r * r) return false;
  }
  for (const c of centers) {
    const dx = c.x - x;
    const dy = c.y - y;
    if (dx * dx + dy * dy <= (CENTER_RADIUS + minR) * (CENTER_RADIUS + minR)) return false;
  }
  return true;
}

/** بذر حتمي لمراكز الموارد — نفس البذور لكل موسم. */
export function seedCenters(seasonDay: number): CenterEntity[] {
  const kinds = TD.territory.center.kinds as string[];
  return CENTER_SEEDS.slice(0, 14).map(([x, y], i) => {
    const kind = kinds[i % kinds.length] as CenterEntity["kind"];
    return {
      id: `cnt_${kind}_${i}`,
      kind,
      x,
      y,
      radius: CENTER_RADIUS,
      lockedAllianceId: null,
      lockedUntilMs: null,
      reserve: TD.territory.center.gather_capacity,
      spawnedSeasonDay: Math.min(Math.max(seasonDay, 1), 28),
    };
  });
}

/** إعادة تعبئة المراكز المستنفدة بعد انتهاء فترة respawn. */
export function respawnDueCenters(
  centers: CenterEntity[],
  seasonDay: number,
  nowMs: number,
): CenterEntity[] {
  const out: CenterEntity[] = [];
  for (const c of centers) {
    if (c.reserve <= 0 && c.lockedUntilMs !== null && c.lockedUntilMs <= nowMs) {
      out.push({
        ...c,
        reserve: TD.territory.center.gather_capacity,
        lockedAllianceId: null,
        lockedUntilMs: null,
      });
    }
  }
  return out;
}

/** قفل مركز على التحالف الذي جمَع منه. */
export function lockCenter(c: CenterEntity, allianceId: string, nowMs: number): CenterEntity {
  const lockMs = TD.territory.center.lock_minutes * 60 * 1000;
  return {
    ...c,
    lockedAllianceId: allianceId,
    lockedUntilMs: nowMs + lockMs,
  };
}

/** حساب كمية الجمع من المركز حاليًا. */
export function centerGatherAmount(
  c: CenterEntity,
  troopsCount: number,
): { amount: number; depleted: boolean } {
  const perTick = Math.max(1, Math.floor(troopsCount / 100));
  const amount = Math.min(Math.max(1, perTick), c.reserve);
  return { amount, depleted: amount >= c.reserve };
}

/** نوع المورد الذي يجمعه المركز. */
export function centerResource(kind: CenterEntity["kind"]): "food" | "wood" | "stone" | "gold" {
  switch (kind) {
    case "granary":
      return "food";
    case "wood_lot":
      return "wood";
    case "stone_pit":
      return "stone";
    case "mother_lode":
      return "gold";
  }
}

/** أنواع المراكز من JSON. */
export function centerKinds(): string[] {
  return (TD.territory.center.kinds as string[]) || [];
}

export default {
  flagRadius,
  outpostRadius,
  gatherBonus,
  gatherMultiplier,
  patrolReduction,
  insideTerritory,
  marchCrossesTerritory,
  canBuildOutpost,
  validPosition,
  seedCenters,
  respawnDueCenters,
  lockCenter,
  centerGatherAmount,
  centerResource,
  centerKinds,
};
