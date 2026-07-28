// P3-T4: منطق المتجر — speedups + VIP + gems (sandbox، بدون مدفوعات حقيقية).
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
  free_speedup_sec_per_day: number;
};

export function shopConstants() {
  return getShop().constants;
}

export function shopCatalog(): SpeedupItem[] {
  return getShop().speedups;
}

export function getSpeedup(itemId: string): SpeedupItem | undefined {
  return getShop().speedups.find((s) => s.id === itemId);
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

/** عدد اليوم (UTC) لاستخدامه في المنح اليومية */
export function utcDay(nowMs: number): number {
  return Math.floor(nowMs / 86_400_000);
}
