import buildingsData from "../../data/buildings.json";
import { trainCost } from "../../lib/gameData";

/**
 * نظام المستشفى (P2-T2)
 * الجرحى الخطيرون (severely wounded) من المعارك يُستقبلون حسب سعة المستشفى —
 * الفائض يموت. الشفاء زمني عبر طوابير الـ tick مقابل نصف تكلفة التدريب.
 * كل القيم تُقرأ من data/buildings.json — لا قيم ثابتة هنا.
 */

export type HospitalConfig = {
  base_capacity: number;
  capacity_per_level: number;
  heal_cost_factor: number;
  heal_seconds_per_troop: number;
};

export const HOSPITAL_CONFIG = (buildingsData as any).hospital as HospitalConfig;

/** سعة المستشفى حسب مستوى المبنى (مستوى 0 = لا مستشفى) */
export function hospitalCapacity(hospitalLevel: number): number {
  if (hospitalLevel <= 0) return 0;
  return HOSPITAL_CONFIG.base_capacity + HOSPITAL_CONFIG.capacity_per_level * (hospitalLevel - 1);
}

/** تكلفة شفاء مجموعة قوات = نصف تكلفة تدريبها (لكل وحدة على حدة) */
export function healCost(troops: Record<string, number>): { food: number; wood: number; stone: number; gold: number } {
  const total = { food: 0, wood: 0, stone: 0, gold: 0 };
  const f = HOSPITAL_CONFIG.heal_cost_factor;
  for (const [unitId, count] of Object.entries(troops)) {
    const c = trainCost(unitId, Math.max(0, Number(count) || 0));
    total.food += Math.floor(c.food * f);
    total.wood += Math.floor(c.wood * f);
    total.stone += Math.floor(c.stone * f);
    total.gold += Math.floor(c.gold * f);
  }
  return total;
}

/** مدة الشفاء بالثواني لعدد من القوات */
export function healDurationSec(troopCount: number): number {
  return HOSPITAL_CONFIG.heal_seconds_per_troop * Math.max(0, troopCount);
}

/**
 * قبول الجرحى حسب السعة المتاحة.
 * يعيد: admitted (مقبولون في المستشفى)، died (الفائض فوق السعة — يموتون).
 */
export function admitWounded(
  severely: Record<string, number>,
  alreadyWounded: Record<string, number>,
  hospitalLevel: number,
): { admitted: Record<string, number>; died: Record<string, number> } {
  const capacity = hospitalCapacity(hospitalLevel);
  const currentTotal = Object.values(alreadyWounded).reduce((s, n) => s + Math.max(0, n || 0), 0);
  let free = Math.max(0, capacity - currentTotal);

  const admitted: Record<string, number> = {};
  const died: Record<string, number> = {};
  for (const [unitId, count] of Object.entries(severely)) {
    let c = Math.max(0, Number(count) || 0);
    const take = Math.min(c, free);
    if (take > 0) admitted[unitId] = take;
    if (c - take > 0) died[unitId] = c - take;
    free -= take;
  }
  return { admitted, died };
}
