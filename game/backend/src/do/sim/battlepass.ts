// P4-T1: منطق Battle Pass — نقاط خبرة من أفعال اللعب + مستويات بمكافآت مجانية/مدفوعة.
// كل القيم تُقرأ من data/battlepass.json عبر getBattlePass() — لا ثوابت هنا.
import { getBattlePass } from "../../lib/gameData";

export type BpReward = {
  type: string;                 // "food" | "wood" | "stone" | "gold" | "gems" | "speedup"
  amount?: number;              // للموارد و gems
  item_id?: string;             // لـ speedup
  count?: number;               // عدد عناصر speedup
};

export type BpLevel = {
  level: number;
  free: BpReward;
  premium: BpReward;
};

export function bpConstants() {
  return getBattlePass().constants;
}

export function bpSeasonId(): string {
  return getBattlePass().season_id;
}

export function bpLevels(): BpLevel[] {
  return getBattlePass().levels;
}

export function bpXpFor(action: string): number {
  return (getBattlePass().xp_sources as Record<string, number>)[action] || 0;
}

/** مستوى اللاعب من مجموع XP (خطي: xp_per_level لكل مستوى، بسقف max_level) */
export function bpLevelForXp(xp: number): number {
  const c = getBattlePass().constants;
  const lvl = Math.floor(xp / c.xp_per_level);
  return Math.min(lvl, c.max_level);
}

/** XP المطلوب للوصول لمستوى معين */
export function bpXpRequiredFor(level: number): number {
  return level * getBattlePass().constants.xp_per_level;
}

/** تقدم اللاعب نحو المستوى التالي: (xp الحالي في المستوى, المطلوب للمستوى التالي) */
export function bpProgressInLevel(xp: number): { inLevel: number; perLevel: number; atMax: boolean } {
  const c = getBattlePass().constants;
  const level = bpLevelForXp(xp);
  if (level >= c.max_level) return { inLevel: c.xp_per_level, perLevel: c.xp_per_level, atMax: true };
  return { inLevel: xp - bpXpRequiredFor(level), perLevel: c.xp_per_level, atMax: false };
}

/** مكافآت مستوى معين (null إن لم يوجد) */
export function bpRewardFor(level: number, track: "free" | "premium"): BpReward | null {
  const lvl = getBattlePass().levels.find((l) => l.level === level);
  return lvl ? lvl[track] : null;
}

/** كل المستويات القابلة للمطالبة عند مستوى لاعب معين (1..playerLevel) */
export function bpClaimableLevels(playerLevel: number): number[] {
  const out: number[] = [];
  for (let l = 1; l <= Math.min(playerLevel, getBattlePass().constants.max_level); l++) out.push(l);
  return out;
}
