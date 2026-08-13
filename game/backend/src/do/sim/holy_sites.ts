import holySitesData from "../../data/holy_sites.json";
import { tierData, troopTierStats, unitName } from "./troops";
// P8-T4: المواقع المقدسة ودورة Lost Temple Cycle
// Sanctum (حراس T1 × 10k) / Altar (T2 × 15k) / Shrine (T3 × 30k) بحماية
// احتفاظ 4 ساعات، والمنازعة تُفتح دوريًا كل 3 أيام. Temple في قلب الخريطة
// بحراس من كل الدرجات حتى T5؛ من يحتفظه 8 ساعات كاملة يصبح ملك المملكة.
// قواعد الباف: النوع الواحد لا يتراكب — أعلى باف مملوك فقط يسري، ومالك
// المعبد يتخطى كل أنواع المواقع الأخرى (temple buff أعلى دائمًا).
export const HOLY_SITES = holySitesData as unknown as {
  version: number;
  constants: {
    hold_duration_ms: number;
    contest_cycle_days: number;
    capture_gain_base: number;
    capture_gain_power_div: number;
  };
  buffs: Record<string, Record<string, number>>;
  kinds: Record<string, { guard_tier: number; guard_count: number; hold_duration_ms: number; buff: string }>;
  temple: {
    id: string;
    pos: [number, number];
    guard_tiers: { tier: number; share: number }[];
    guard_total: number;
    hold_for_king_ms: number;
    wounded_dead_share: number;
    buff: string;
    unlock_day: number;
  };
  sites: { id: string; kind: string; pos: [number, number] }[];
};
export function holdDurationMs(): number {
  return HOLY_SITES.constants.hold_duration_ms;
}
/** حامية الموقع المقدس — قوات من نوع T1-T3 حسب النوع (فرع مختار دوريًا للقراءة). */
export function siteGuardTroops(siteKind: string): { troops: Record<string, number>; total: number } {
  const kind = HOLY_SITES.kinds[siteKind];
  if (!kind) return { troops: {}, total: 0 };
  const unitId = unitIdForTier(kind.guard_tier, "infantry");
  if (!unitId) return { troops: {}, total: 0 };
  return { troops: { [unitId]: kind.guard_count }, total: kind.guard_count };
}
/** حامية المعبد — مزيج من كل الدرجات حتى T5. */
export function templeGuardTroops(): Record<string, number> {
  const troops: Record<string, number> = {};
  const temple = HOLY_SITES.temple;
  for (const gt of temple.guard_tiers) {
    for (const branch of ["infantry", "archer", "cavalry", "siege"]) {
      const unitId = unitIdForTier(gt.tier, branch);
      if (!unitId) continue;
      const share = temple.guard_total * gt.share / 4;
      troops[unitId] = (troops[unitId] || 0) + Math.round(share);
    }
  }
  return troops;
}
function unitIdForTier(tier: number, branch: string): string | null {
  const name = unitName(tier, branch);
  if (!name) return null;
  return `${branch}_t${tier}`;
}
/** إجمالي حامية المعبد */
export function templeGuardTotal(): number {
  return HOLY_SITES.temple.guard_total;
}
/** باف الموقع لكل نوع — أعلى باف مملوك يسري وحده (لا تراكم). */
export function bestHeldSiteBuff(heldKinds: Set<string>): Record<string, number> {
  const hierarchy = ["temple", "shrine", "altar", "sanctum"];
  const buffs = HOLY_SITES.buffs;
  for (const kind of hierarchy) {
    if (heldKinds.has(kind) && buffs[kind]) return buffs[kind];
  }
  return {};
}
/** هل المعبد مفتوحًا في هذا اليوم من الموسم؟ */
export function templeUnlocked(seasonDay: number): boolean {
  return seasonDay >= (HOLY_SITES.temple.unlock_day ?? 40);
}
/** مكسب تقدم الاحتلال من قوة القوات المتبقية — من JSON. */
export function siteCaptureGain(remainingPower: number): number {
  const c = HOLY_SITES.constants;
  return Math.min(100, c.capture_gain_base + Math.floor(remainingPower / c.capture_gain_power_div));
}
/** حامية النوع كـ total بسيط (حراس T3 للموقع) لاستخدامه مع troopPower. */
export function siteGuardPower(siteKind: string, civId?: string): number {
  const troops = siteGuardTroops(siteKind);
  if (!troops.total) return 0;
  const stats = troopTierStats(HOLY_SITES.kinds[siteKind].guard_tier, "infantry");
  return stats ? stats.attack * troops.total : 0;
}
/** نسبة الجرحى التي تموت في المعبد (50%) فوق القاعدة — لقتال المعبد فقط. */
export function templeWoundedDeadShare(): number {
  return HOLY_SITES.temple.wounded_dead_share;
}
/** طول فترة التتويج: 8 ساعات احتفاظ مستمر بالمعبد. */
export function holdForKingMs(): number {
  return HOLY_SITES.temple.hold_for_king_ms;
}
