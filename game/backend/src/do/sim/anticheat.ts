import anticheatData from "../../data/anticheat.json";

/**
 * نظام anti-cheat الأساسي (P4-T5): حدود معدل للاعب (rate limits) + كشف شذوذ في الحمولات.
 * كل القيم تُقرأ من data/anticheat.json — لا ثوابت هنا.
 *
 * نموذج الحالة: RateLimiter في الذاكرة (لكل Durable Object / لكل isolate) —
 * عدّاد أفعال داخل نافذة زمنية منزلقة + آخر زمن فعل للـ cooldown.
 */

export type RateLimitRule = {
  window_ms: number;
  max_actions: number;
  cooldown_ms: number;
};

export type RateLimitAction =
  | "march"
  | "pass_attack"
  | "help"
  | "shop_buy"
  | "use_speedup"
  | "rally"
  | "shield_activate"
  | "city_relocate"
  | "alliance_tech_donate"
  | "alliance_tech_start";

const CFG = anticheatData as any;

export const ANTICHEAT_CONSTANTS = {
  enabled: CFG.constants.enabled as boolean,
  violationLogLimit: CFG.constants.violation_log_limit as number,
};

export const RATE_LIMITS: Record<RateLimitAction, RateLimitRule> = CFG.rate_limits;

export const ANOMALY_LIMITS = CFG.anomaly as {
  max_troops_per_march: number;
  max_single_unit_per_march: number;
  max_shop_buy_count: number;
  min_march_distance_tiles: number;
  max_active_marches_per_player: number;
};

export function isRateLimitedAction(a: string): a is RateLimitAction {
  return a in RATE_LIMITS;
}

/** نتيجة فحص المعدل: مسموح أو مرفوض مع سبب + مهلة إعادة المحاولة بالمللي ثانية. */
export type RateCheckResult =
  | { allowed: true }
  | { allowed: false; reason: "cooldown" | "window_exceeded"; retryAfterMs: number };

type Bucket = { windowStart: number; count: number; lastAction: number };

/**
 * Rate limiter في الذاكرة لكل لاعب × نوع فعل.
 * النافذة منزلقة ثابتة: تُصفَّر عند انقضاء window_ms من بدايتها.
 */
export class AntiCheatRateLimiter {
  private buckets = new Map<string, Bucket>();

  /** فحص + تسجيل فعل واحد. عند الرفض لا يُسجَّل الفعل (لا يحجب المحاولة التالية بعد المهلة). */
  check(playerId: string, action: RateLimitAction, now: number): RateCheckResult {
    if (!ANTICHEAT_CONSTANTS.enabled) return { allowed: true };
    const rule = RATE_LIMITS[action];
    const key = `${playerId}:${action}`;
    let b = this.buckets.get(key);

    if (!b) {
      this.buckets.set(key, { windowStart: now, count: 1, lastAction: now });
      return { allowed: true };
    }

    // انقضت النافذة — بداية جديدة
    if (now - b.windowStart >= rule.window_ms) {
      b.windowStart = now;
      b.count = 0;
    }

    // cooldown بين فعلين متتاليين
    if (rule.cooldown_ms > 0 && now - b.lastAction < rule.cooldown_ms) {
      return {
        allowed: false,
        reason: "cooldown",
        retryAfterMs: rule.cooldown_ms - (now - b.lastAction),
      };
    }

    // سقف النافذة
    if (b.count >= rule.max_actions) {
      return {
        allowed: false,
        reason: "window_exceeded",
        retryAfterMs: rule.window_ms - (now - b.windowStart),
      };
    }

    b.count += 1;
    b.lastAction = now;
    return { allowed: true };
  }

  /** إعادة ضبط لاعب (اختبارات/إدارة). */
  reset(playerId: string): void {
    for (const k of [...this.buckets.keys()]) {
      if (k.startsWith(playerId + ":")) this.buckets.delete(k);
    }
  }

  /** عدد السجلات الحية (للمراقبة). */
  size(): number {
    return this.buckets.size;
  }
}

/** فحص شذوذ حمولة مسيرة: أعداد ضخمة أو وحدة واحدة فوق السقف. يُعيد أول مخالفة أو null. */
export function checkMarchPayload(
  troops: Record<string, number>,
  activeMarches: number,
): string | null {
  if (!ANTICHEAT_CONSTANTS.enabled) return null;
  if (activeMarches >= ANOMALY_LIMITS.max_active_marches_per_player) {
    return "max_active_marches_exceeded";
  }
  let total = 0;
  for (const c of Object.values(troops)) {
    const n = Number(c);
    if (!Number.isInteger(n) || n <= 0) return "invalid_troop_count";
    if (n > ANOMALY_LIMITS.max_single_unit_per_march) return "single_unit_cap_exceeded";
    total += n;
  }
  if (total > ANOMALY_LIMITS.max_troops_per_march) return "total_troops_cap_exceeded";
  return null;
}

/** فحص شذوذ طلب شراء من المتجر. */
export function checkShopBuyPayload(count: number): string | null {
  if (!ANTICHEAT_CONSTANTS.enabled) return null;
  if (!Number.isInteger(count) || count <= 0) return "invalid_buy_count";
  if (count > ANOMALY_LIMITS.max_shop_buy_count) return "buy_count_cap_exceeded";
  return null;
}
