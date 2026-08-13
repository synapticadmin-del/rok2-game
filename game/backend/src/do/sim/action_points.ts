// P8-T5: نقاط العمل (AP) والدروع (Peace Shield) والتهجير (Relocation)
// AP رصيد عالمي 1000 يتجدد 1 نقطة كل 45 ثانية؛ كل مسيرة هجومية تخصم رصيدًا
// (برابرة/موقع مقدس/مدينة). الدرع يُفعّل بالجواهر ويحمي من الاستهداف،
// وحماية ساعة (war frenzy) بعد كل هجوم متلقَّى. التهجير عشوائي أو موجه.
import apData from "../../data/action_points.json";

export const ACTION_POINTS = apData as unknown as {
  version: number;
  constants: {
    ap_cap: number;
    ap_regen_interval_ms: number;
    ap_regen_amount: number;
    sandbox_starting_ap: number;
  };
  costs: Record<string, number>;
  shields: Array<{ duration_minutes: number; cost_gems: number }>;
  war_frenzy: { duration_ms: number };
  relocation: {
    cooldown_ms: number;
    random_cost_gems: number;
    targeted_cost_gems: number;
    random_cost_ap: number;
    targeted_cost_ap: number;
  };
};

/** تجديد AP بناءً على آخر تحديث (idempotent — لا يتجاوز السقف). */
export function regenAp(current: number, lastRegenMs: number, nowMs: number): { ap: number; lastRegenMs: number } {
  if (current >= apCap()) return { ap: apCap(), lastRegenMs: nowMs };
  const elapsed = nowMs - lastRegenMs;
  if (elapsed < AP_INTERVAL_MS) return { ap: current, lastRegenMs: lastRegenMs };
  const gained = Math.floor(elapsed / AP_INTERVAL_MS) * AP_REGEN_AMOUNT;
  const ap = Math.min(apCap(), current + gained);
  return { ap, lastRegenMs: lastRegenMs + Math.floor(elapsed / AP_INTERVAL_MS) * AP_INTERVAL_MS };
}

export function apCap(): number { return ACTION_POINTS.constants.ap_cap; }
export const AP_INTERVAL_MS = ACTION_POINTS.constants.ap_regen_interval_ms;
export const AP_REGEN_AMOUNT = ACTION_POINTS.constants.ap_regen_amount;
export function startingAp(): number { return ACTION_POINTS.constants.sandbox_starting_ap; }
export function apCost(action: string): number { return ACTION_POINTS.costs[action] ?? 0; }
export function shieldOptions(): Array<{ duration_minutes: number; cost_gems: number }> { return ACTION_POINTS.shields; }
export function warFrenzyDurationMs(): number { return ACTION_POINTS.war_frenzy.duration_ms; }
export function relocationCooldownMs(): number { return ACTION_POINTS.relocation.cooldown_ms; }
export function relocationCosts(mode: "random" | "targeted"): { gems: number; ap: number } {
  return mode === "targeted"
    ? { gems: ACTION_POINTS.relocation.targeted_cost_gems, ap: ACTION_POINTS.relocation.targeted_cost_ap }
    : { gems: ACTION_POINTS.relocation.random_cost_gems, ap: ACTION_POINTS.relocation.random_cost_ap };
}
/** هل يجوز تفعيل الدرع الآن؟ (لا cooldown ولا war_frenzy نشط) */
export function canActivateShield(warFrenzyUntilMs: number | null, shieldUntilMs: number | null, nowMs: number): { ok: boolean; reason?: string } {
  if (shieldUntilMs && shieldUntilMs > nowMs) return { ok: false, reason: "shield_active" };
  if (warFrenzyUntilMs && warFrenzyUntilMs > nowMs) return { ok: false, reason: "war_frenzy" };
  return { ok: true };
}
