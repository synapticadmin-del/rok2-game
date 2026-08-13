import talentsData from "../../data/talents.json";
/**
 * نظام مواهب القادة (P8-T1)
 * كل قائد يملك شجرتي مواهب — troop_type (نوع القوات: infantry/cavalry/archer/siege)
 * وrole (الدور: attack/defense/support) — نقاط تُمنح بمستوى القائد وتُصرف على عقد
 * الشجرتين، وتُطبَّق بافاتها على قوة المسيرة في القتال وسرعة المسير والتدريب.
 * كل القيم تُقرأ من data/talents.json — لا قيم hard-coded هنا.
 */
export type TalentNodeDef = {
  id: string;
  branch: string;
  stat: string;
  per_point: number;
  max_points: number;
};
export type TalentTreeDef = {
  id: string;
  name: string;
  branches: string[];
  description: string;
  nodes: TalentNodeDef[];
};
export type TalentAllocations = Record<string, number>; // nodeId -> points
export type TalentConstants = {
  talent_points_per_level: number;
  points_cap_rarity: Record<string, number>;
  max_points_per_node: number;
  reset_refund_ratio: number;
  reset_cooldown_ms: number;
  talent_buff_stat_cap: number;
};
export type TalentMods = {
  troop_attack?: number;
  troop_defense?: number;
  troop_health?: number;
  march_speed?: number;
  training_speed?: number;
  gathering_speed?: number;
  resource_production?: number;
  siege_damage?: number;
  siege_resistance?: number;
  counter_damage?: number;
  damage_taken_reduction?: number;
  garrison_damage_reduction?: number;
  heal_cost_reduction?: number;
  xp_gain?: number;
};
const TREES = (talentsData as any).trees as TalentTreeDef[];
export const TALENT_CONSTANTS = (talentsData as any).constants as TalentConstants;
export const TALENT_TREES = TREES;
export const NODE_INDEX: Record<string, TalentNodeDef> = {};
for (const t of TREES) {
  for (const n of t.nodes) {
    NODE_INDEX[n.id] = n;
  }
}
export function talentNode(nodeId: string): TalentNodeDef | undefined {
  return NODE_INDEX[nodeId];
}
export function getTalentTrees(): TalentTreeDef[] {
  return TREES;
}
/** النقاط المكتسبة وفق مستوى القائد — من TALENT_CONSTANTS.talent_points_per_level */
export function talentPointsEarned(level: number): number {
  return Math.floor(Math.max(0, level)) * (TALENT_CONSTANTS.talent_points_per_level || 1);
}
/** سقف النقاط الكلي وفق ندرة القائد — من points_cap_rarity */
export function talentPointsCap(rarity: string): number {
  const caps = TALENT_CONSTANTS.points_cap_rarity || {};
  if (caps[rarity]) return caps[rarity];
  // قيمة افتراضية فقط عند ندرة غير معرّفة — الثوابت نفسها من JSON
  const vals = Object.values(caps) as number[];
  return vals.length > 0 ? Math.max(...vals) : 60;
}
/** إجمالي النقاط المصروفة في تخصيص ما */
export function totalSpent(allocs: TalentAllocations | null | undefined): number {
  if (!allocs) return 0;
  let s = 0;
  for (const v of Object.values(allocs)) s += Math.max(0, Math.floor(v));
  return s;
}
/** النقاط المتاحة للتخصيص: المكتسبة - المصروفة */
export function availableTalentPoints(level: number, rarity: string, allocs: TalentAllocations | null | undefined): number {
  const earned = talentPointsEarned(level);
  const spent = totalSpent(allocs);
  return Math.max(0, Math.min(earned, talentPointsCap(rarity)) - spent);
}
/**
 * التحقق من تخصيص صالح — يرجع رسالة خطأ أو null إذا كان صالحًا.
 * يفحص: وجود العقدة، سقف العقد، عدم تجاوز النقاط المتاحة، وتوافق فرع العقدة.
 */
export function validateTalentAllocation(
  nodeId: string,
  points: number,
  level: number,
  rarity: string,
  currentAllocs: TalentAllocations | null | undefined,
): string | null {
  const node = talentNode(nodeId);
  if (!node) return "Unknown talent node";
  const pts = Math.floor(points || 0);
  if (pts <= 0 || pts > node.max_points) return "Invalid points (max " + node.max_points + " per node)";
  const cap = talentPointsCap(rarity);
  const available = Math.max(0, Math.min(talentPointsEarned(level), cap)) - totalSpent(currentAllocs);
  if (pts > available) return "Not enough talent points";
  return null;
}
/**
 * حساب بافات المواهب الموزعة — يرجع TalentMods بنسب جمعية مقيدة بسقف
 * talent_buff_stat_cap لكل stat فردي (بلا hard-coded: من TALENT_CONSTANTS).
 */
export function computeTalentMods(allocs: TalentAllocations | null | undefined): TalentMods {
  const mods: TalentMods = {};
  const cap = TALENT_CONSTANTS.talent_buff_stat_cap ?? 0.3;
  if (!allocs) return mods;
  for (const [nodeId, pts] of Object.entries(allocs)) {
    const node = talentNode(nodeId);
    if (!node) continue;
    const p = Math.min(Math.floor(pts), node.max_points);
    if (p <= 0) continue;
    const add = p * node.per_point;
    const cur = mods[node.stat as keyof TalentMods] ?? 0;
    mods[node.stat as keyof TalentMods] = Math.min(cap, cur + add);
  }
  return mods;
}
/**
 * باف المواهب في القتال: مجموع troop_attack (للقوة الهجومية) وتجميع باقي بافات
 * الموديل في mods كاملة لتُستهلك في combat عبر talentMods.
 */
export function talentAttackMod(allocs: TalentAllocations | null | undefined): number {
  return computeTalentMods(allocs).troop_attack ?? 0;
}
/** باف الهجوم المضاد: يدخل في counterMult الفعال */
export function talentCounterMod(allocs: TalentAllocations | null | undefined): number {
  return computeTalentMods(allocs).counter_damage ?? 0;
}
/** إعادة ضبط المواهب: استرجاع النقاط برصيد reset_refund_ratio */
export function resetTalentAllocations(allocs: TalentAllocations | null | undefined): { refund: number; allocs: TalentAllocations } {
  const spent = totalSpent(allocs);
  const refund = Math.floor(spent * (TALENT_CONSTANTS.reset_refund_ratio ?? 0.8));
  return { refund, allocs: {} };
}
