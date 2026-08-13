// P3-T4 / P9-T4: منطق المتجر — speedups + VIP كامل + gems (sandbox، بدون مدفوعات حقيقية).
// كل القيم تُقرأ من data/shop.json عبر getShop() — لا ثوابت هنا.
import { getShop } from "../../lib/gameData";

export type SpeedupItem = {
  id: string;
  name: string;
  seconds: number;
  cost_gems: number;
  vip_points: number;
};

export type VipTier = {
  level: number;
  points_required: number;
  production_mult: number;
  build_speed_mult: number;
  train_speed_mult: number;
  research_speed_mult: number;
  heal_speed_mult: number;
  gather_mult: number;
  free_speedup_sec_per_day: number;
  extra_build_queue: boolean;
};

export type VipDailyConfig = {
  base_per_day: number;
  connected_bonus: number;
  daily_cap: number;
};

export type VipStoreDiscount = {
  min_level: number;
  discount: number;
};

export function shopConstants() {
  return getShop().constants;
}

export function vipDailyConfig(): VipDailyConfig {
  return getShop().constants.vip_daily;
}

export function vipDailyPoints(): number {
  const c = vipDailyConfig();
  return Number(c.base_per_day);
}

export function vipConnectedBonus(): number {
  const c = vipDailyConfig();
  return Number(c.connected_bonus);
}

export function vipDailyCap(): number {
  const c = vipDailyConfig();
  return Number(c.daily_cap);
}

/** نقاط VIP اليومية الكاملة لمن يدخل اليوم (40 أساس + 20 اتصال يومي) */
export function vipDailyFullGrant(): number {
  return Math.min(vipDailyPoints() + vipConnectedBonus(), vipDailyCap());
}

export function shopCatalog(): SpeedupItem[] {
  return getShop().speedups;
}

export function getSpeedup(itemId: string): SpeedupItem | undefined {
  return getShop().speedups.find((s) => s.id === itemId);
}

/** تكلفة إنهاء طابور بالجواهر، مشتقة من أفضل قيمة زمن/جوهرة في كتالوج التسريعات. */
export function gemFinishCost(remainingSeconds: number): number {
  const seconds = Math.max(0, Math.ceil(Number(remainingSeconds) || 0));
  if (seconds <= 0) return 0;
  const rates = shopCatalog()
    .filter((item) => item.seconds > 0 && item.cost_gems > 0)
    .map((item) => item.cost_gems / item.seconds);
  const gemsPerSecond = rates.length > 0 ? Math.min(...rates) : 1 / 60;
  return Math.max(1, Math.ceil(seconds * gemsPerSecond));
}

export function vipTiers(): VipTier[] {
  return getShop().vip_tiers;
}

/** أعلى مستوى VIP تكفيه النقاط الحالية */
export function vipTierForPoints(points: number): VipTier {
  const tiers = getShop().vip_tiers;
  let cur = tiers[0];
  for (const t of tiers) {
    if (points >= t.points_required) cur = t;
  }
  return cur;
}

/** نقاط VIP المكتسبة من شراء بقيمة gems معينة */
export function vipPointsForPurchase(gemsSpent: number): number {
  return gemsSpent * getShop().constants.vip_points_per_gem;
}

/**
 * P9-T4: تطبيق نقاط VIP اليومية على الحالة — 40/يوم + 20 لمن يدخل اليوم،
 * مع سقف يومي (200). يعيد الحالة المحدثة ونقاط المنحة الفعلية.
 * lastDailyPointsDay = آخر يوم (UTC) مُنحت فيه النقاط؛ lastLoginDay = آخر يوم نشاط.
 */
export function applyVipDailyPoints(state: {
  points: number;
  lastDailyPointsDay: number;
  lastLoginDay: number;
}, nowMs: number): { state: typeof state; granted: number; connectedToday: boolean } {
  const day = utcDay(nowMs);
  const connectedToday = state.lastLoginDay >= day;
  // المنحة اليومية تُحسب مرة واحدة في اليوم وتُمنح لمن كان نشطًا اليوم
  // (lastDailyPointsDay < day تعني أن اليوم جديد ولم تُمنح نقاطه بعد).
  if (state.lastDailyPointsDay >= day) {
    return { state, granted: 0, connectedToday };
  }
  const grant = vipDailyFullGrant();
  const newPoints = state.points + grant;
  return {
    state: {
      ...state,
      points: newPoints,
      lastDailyPointsDay: day,
      lastLoginDay: day,
    },
    granted: grant,
    connectedToday,
  };
}

/** تحديث يوم النشاط فقط (تسجيل اتصال بدون منح نقاط إضافية). */
export function markVipConnected(state: { lastLoginDay: number }, nowMs: number): typeof state {
  const day = utcDay(nowMs);
  if (state.lastLoginDay >= day) return state;
  return { ...state, lastLoginDay: day };
}

/**
 * P9-T4: متجر VIP — أسعار مخفّضة حسب المستوى.
 * يعيد سعر العنصر بعد الخصم (يُقرّب للأعلى لجوهرة كاملة) أو 0 إذا لم يُفتح بعد (CH5).
 */
export function vipStorePrice(baseGems: number, playerLevel: number, hallLevel: number): number {
  if (hallLevel < vipStoreHallRequired()) return 0;
  const discounts = getShop().constants.vip_store.discount_by_tier as VipStoreDiscount[];
  let best = 0;
  for (const d of discounts) {
    if (playerLevel >= d.min_level && d.discount > best) best = d.discount;
  }
  if (best <= 0) return baseGems;
  return Math.max(1, Math.ceil(Number(baseGems) * (1 - best)));
}

export function vipStoreHallRequired(): number {
  return Number(getShop().constants.vip_store.hall_level_required);
}

export function vipStoreDiscountForLevel(level: number): number {
  const discounts = getShop().constants.vip_store.discount_by_tier as VipStoreDiscount[];
  let best = 0;
  for (const d of discounts) {
    if (level >= d.min_level && d.discount > best) best = d.discount;
  }
  return best;
}

/** عدد اليوم (UTC) لاستخدامه في المنح اليومية */
export function utcDay(nowMs: number): number {
  return Math.floor(nowMs / 86_400_000);
}
