import researchData from "../../data/research.json";

/**
 * شجرة البحث (P2-T3)
 * فرعان: economy + military. التكلفة/المدة تتدرج مع المستوى عبر المضاعفات في الملف.
 * الباف يُطبق: buff.per_level × المستوى المُكتمِل.
 * كل القيم تُقرأ من data/research.json — لا قيم ثابتة هنا.
 */

export type TechBranch = "economy" | "military";

export type TechPrereq = { id: string; level: number };

export type TechDef = {
  id: string;
  name: string;
  branch: TechBranch;
  max_level: number;
  base_cost: { food: number; wood: number; stone: number; gold: number };
  base_duration_sec: number;
  academy_base_req: number;
  prerequisites: TechPrereq[];
  buff: { stat: string; per_level: number };
  description: string;
};

export type ResearchLevels = Record<string, number>; // techId → level مُكتمِل

const DATA = researchData as any;
const TECHS: TechDef[] = DATA.technologies;
const COST_MULT: number = DATA.cost_mult;
const DURATION_MULT: number = DATA.duration_mult;

const BY_ID: Record<string, TechDef> = {};
for (const t of TECHS) BY_ID[t.id] = t;

/** قائمة كل التقنيات (للـ meta/API) */
export const TECHNOLOGIES: Record<string, TechDef> = BY_ID;

export function getTech(id: string): TechDef | undefined {
  return BY_ID[id];
}

export function isValidTech(id: string): boolean {
  return !!BY_ID[id];
}

export function getTechTree() {
  return DATA;
}

/** تكلفة البدء في مستوى معين (level 1-based) */
export function researchCost(techId: string, level: number) {
  const t = BY_ID[techId];
  if (!t) return { food: 0, wood: 0, stone: 0, gold: 0 };
  const m = Math.pow(COST_MULT, Math.max(0, level - 1));
  return {
    food: Math.floor(t.base_cost.food * m),
    wood: Math.floor(t.base_cost.wood * m),
    stone: Math.floor(t.base_cost.stone * m),
    gold: Math.floor(t.base_cost.gold * m),
  };
}

/** مدة البدء في مستوى معين بالثواني */
export function researchDurationSec(techId: string, level: number): number {
  const t = BY_ID[techId];
  if (!t) return 0;
  return Math.floor(t.base_duration_sec * Math.pow(DURATION_MULT, Math.max(0, level - 1)));
}

/** مستوى الأكاديمية المطلوب لبدء مستوى معين */
export function academyReq(techId: string, level: number): number {
  const t = BY_ID[techId];
  if (!t) return 1;
  return t.academy_base_req * Math.max(1, level);
}

/**
 * مجموع باف إحصائية معينة من أبحاث اللاعب المكتملة.
 * levels: techId → level مكتمل. يعيد رقماً عشرياً (مثلاً 0.06 = 6%).
 */
export function researchBuff(levels: ResearchLevels, stat: string): number {
  let total = 0;
  for (const [id, lvl] of Object.entries(levels || {})) {
    const t = BY_ID[id];
    if (!t || t.buff.stat !== stat) continue;
    total += t.buff.per_level * Math.max(0, Math.min(lvl, t.max_level));
  }
  return total;
}

/** تحقق من استيفاء الـ prerequisites لبدء مستوى جديد */
export function prereqsMet(techId: string, levels: ResearchLevels): { ok: boolean; missing: TechPrereq[] } {
  const t = BY_ID[techId];
  if (!t) return { ok: false, missing: [] };
  const missing: TechPrereq[] = [];
  for (const p of t.prerequisites) {
    if ((levels[p.id] || 0) < p.level) missing.push(p);
  }
  return { ok: missing.length === 0, missing };
}
