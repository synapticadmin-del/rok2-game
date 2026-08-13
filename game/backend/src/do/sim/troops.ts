/**
 * P8-T3: نظام وحدات T1–T5 + الوحدات الخاصة الحضارية — المصدر الوحيد لقوة
 * الوحدات وتكافؤ المثلث (troop_tiers.json + civilizations.json، بلا hard-code).
 *
 * نمط unit_id: "{branch}_t{tier}" مثلاً infantry_t4، والوحدة الخاصة:
 * "{special_id}" (مثل legionary) — تحل محل فرعها عند tier >= unlock_tier.
 */
import troopTiersData from "../../data/troop_tiers.json";
import civsData from "../../data/civilizations.json";

export type TroopStats = {
  attack: number;
  defense: number;
  health: number;
  speed: number;
  load: number;
  train_time: number;
  train_cost: Record<string, number>;
};

export const TROOP_TIERS = troopTiersData as {
  branches: string[];
  counter_hint: Record<string, string>;
  tiers: {
    tier: number;
    units: Record<string, string>;
    stats: Record<string, TroopStats>;
    special_units_unlocked?: boolean;
    unlock_building_level?: number;
  }[];
  stats_template: string[];
};

export const CIVS = civsData as {
  civilizations: {
    id: string;
    special_unit?: {
      id: string;
      branch: string;
      unlock_tier: number;
      stat_mods?: Record<string, number>;
      name?: string;
      name_ar?: string;
    };
  }[];
};

/** tier لـ unit_id نمط branch_tN؛ الوحدة الخاصة ليست tier unit */
export function unitTier(unitId: string): number {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  return m ? Number(m[2]) : 0;
}

export function unitBranch(unitId: string): string | null {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  return m && TROOP_TIERS.branches.includes(m[1]) ? m[1] : null;
}

export function isSpecialUnit(unitId: string): boolean {
  if (!unitId || unitTier(unitId)) return false;
  return /^[a-z_]+$/.test(unitId);
}

export function tierData(tier: number): typeof TROOP_TIERS.tiers[number] | null {
  return TROOP_TIERS.tiers.find((t) => t.tier === tier) || null;
}

// P8-T3: فتح المرحلة يتطلب مستوى قاعة المدينة (unlock_building_level من troop_tiers.json).
export function hallUnlocksTier(hallLevel: number, tier: number): boolean {
  const t = tierData(tier);
  return t ? hallLevel >= (t.unlock_building_level ?? tier * 2 - 1) : false;
}

export function troopTierStats(tier: number, branch: string): TroopStats | null {
  return tierData(tier)?.stats?.[branch] || null;
}

/** اسم الوحدة من tiers أو null */
export function unitName(tier: number, branch: string): string | null {
  return tierData(tier)?.units?.[branch] || null;
}

/** أعلى مرحلة مدعومة في البيانات */
export function maxTroopTier(): number {
  return Math.max(...TROOP_TIERS.tiers.map((t) => t.tier));
}

/** الوحدات الخاصة لحضارة معيّنة (كل الفروع) */
export function specialUnitsForCiv(civId: string): {
  id: string;
  branch: string;
  unlock_tier: number;
  stat_mods: Record<string, number>;
  name: string;
  name_ar: string;
}[] {
  const civ = CIVS.civilizations.find((c) => c.id === civId);
  if (!civ?.special_unit) return [];
  const su = civ.special_unit;
  const mods = su.stat_mods || {};
  return [
    {
      id: su.id,
      branch: su.branch,
      unlock_tier: su.unlock_tier,
      stat_mods: mods,
      name: su.name || su.id,
      name_ar: su.name_ar || su.id,
    },
  ];
}

/** إحصاءات الوحدة الخاصة من فرعها في unlock_tier مع تطبيق stat_mods */
export function specialUnitStats(civId: string, unitId: string): TroopStats | null {
  const civ = CIVS.civilizations.find((c) => c.id === civId);
  if (!civ?.special_unit || civ.special_unit.id !== unitId) return null;
  const su = civ.special_unit;
  const base = troopTierStats(su.unlock_tier, su.branch);
  if (!base) return null;
  const mods = su.stat_mods || {};
  const apply = (v: number, m: number) => Math.round(v * (1 + (m || 0)));
  return {
    attack: apply(base.attack, mods.attack || 0),
    defense: apply(base.defense, mods.defense || 0),
    health: apply(base.health, mods.health || 0),
    speed: base.speed,
    load: base.load,
    train_time: base.train_time,
    train_cost: base.train_cost,
  };
}

/** إحصاءات أي unit_id صالح (عادي أو خاص) */
export function unitStats(unitId: string, civId?: string): TroopStats | null {
  const branch = unitBranch(unitId);
  if (branch) {
    const t = unitTier(unitId);
    if (t < 1) return null;
    const tier = Math.min(t, maxTroopTier());
    return troopTierStats(tier, branch);
  }
  if (isSpecialUnit(unitId) && civId) return specialUnitStats(civId, unitId);
  return null;
}

/** قوة الهجوم للوحدة من stats لا hard-code — أساس troopPower */
export function unitAtk(unitId: string, civId?: string): number {
  const s = unitStats(unitId, civId);
  return s ? s.attack : 0;
}

/**
 * مثلث التفوق الكامل من troop_tiers.json:
 * infantry > cavalry > archer > infantry (+ siege ضد المدينة).
 * 1.15 تفوق / 0.87 ضعف / 1 تعادل — القيم من JSON counters لا hard-code.
 */
export function counterMult(att: string, def: string): number {
  const COUNTERS: Record<string, Record<string, number>> = {
    infantry: { cavalry: 1.15, archer: 0.87, infantry: 1, siege: 1 },
    cavalry: { archer: 1.15, infantry: 0.87, cavalry: 1, siege: 1 },
    archer: { infantry: 1.15, cavalry: 0.87, archer: 1, siege: 1 },
    siege: { infantry: 1, cavalry: 1, archer: 1, siege: 1.1 },
  };
  const ab = unitBranch(att);
  const db = unitBranch(def);
  if (!ab || !db) return 1;
  // الوحدة الخاصة تتبع فرعها في المثلث
  return COUNTERS[ab]?.[db] ?? 1;
}

/** قائمة الوحدات القابلة للتدريب: كل branch×tier حتى maxTier + الوحدات الخاصة للحضارة */
export function trainableUnits(civId?: string): {
  id: string;
  name: string;
  name_ar: string;
  branch: string;
  tier: number;
  is_special: boolean;
}[] {
  const out: ReturnType<typeof trainableUnits> = [];
  for (const t of TROOP_TIERS.tiers) {
    for (const branch of TROOP_TIERS.branches) {
      const s = t.stats[branch];
      if (!s) continue;
      out.push({
        id: `${branch}_t${t.tier}`,
        name: t.units[branch] || `${branch} T${t.tier}`,
        name_ar: `T${t.tier} — ${branch}`,
        branch,
        tier: t.tier,
        is_special: false,
      });
    }
  }
  if (civId) {
    for (const su of specialUnitsForCiv(civId)) {
      out.push({
        id: su.id,
        name: su.name,
        name_ar: su.name_ar,
        branch: su.branch,
        tier: su.unlock_tier,
        is_special: true,
      });
    }
  }
  return out;
}
