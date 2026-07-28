// P3-T5: منطق Soft launch + قياس retention — كل القيم من data/softlaunch.json.
// ممالك الإطلاق المحدود + حساب DAU ونسب رجوع cohorts (D1/D3/D7/D14/D30).
import { getSoftLaunch } from "../../lib/gameData";

export type LaunchKingdom = {
  id: string;
  name: string;
  open: boolean;
  max_players: number;
};

export type RetentionTargets = {
  d1_min_pct: number;
  d7_min_pct: number;
  d30_min_pct: number;
};

/** ممالك الإطلاق المفتوحة حالياً */
export function openKingdoms(): LaunchKingdom[] {
  return getSoftLaunch().kingdoms.filter((k) => k.open);
}

/** هل مملكة معينة جزء من إطلاق soft launch ومفتوحة للانضمام؟ */
export function isKingdomOpen(kingdomId: string): boolean {
  const k = getSoftLaunch().kingdoms.find((x) => x.id === kingdomId);
  return !!k && k.open;
}

/** سعة مملكة معينة (null = غير موجودة في الإطلاق) */
export function kingdomCapacity(kingdomId: string): number | null {
  const k = getSoftLaunch().kingdoms.find((x) => x.id === kingdomId);
  return k ? k.max_players : null;
}

/** عدد أيام retention المطلوب قياسها (D1, D3, D7...) */
export function retentionDayBuckets(): number[] {
  return getSoftLaunch().retention.day_buckets;
}

export function retentionTargets(): RetentionTargets {
  return getSoftLaunch().retention.targets;
}

/** عدد اليوم (UTC) — يطابق utcDay في sim/shop.ts */
export function utcDay(nowMs: number): number {
  return Math.floor(nowMs / 86_400_000);
}

/**
 * يوم cohort للاعب = يوم إنشاء حسابه (created_at بالـ ms → UTC day).
 * نسبة الرجوع Dn من cohort يوم c = (عدد لاعبين نشطوا يوم c+n) / (حجم cohort يوم c).
 */
export function cohortDayOf(createdAtMs: number): number {
  return utcDay(createdAtMs);
}

/** نسبة مئوية مدروسة بأمان (صفر عند مقام صفري) */
export function pct(numerator: number, denominator: number): number {
  if (denominator <= 0) return 0;
  return Math.round((1000 * numerator) / denominator) / 10;
}
