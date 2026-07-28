import zonesData from "../../data/zones.json";

/**
 * نظام التحالف الكامل (P2-T5): رتب + helps + rally.
 * كل القواعد تُقرأ من data/zones.json → alliance — لا قيم ثابتة هنا.
 */

export type AllianceRank = string; // "R1".."R5"

const CFG = (zonesData as any).alliance;

export const ALLIANCE_CONSTANTS = {
  maxMembers: CFG.max_members as number,
  ranks: CFG.ranks as AllianceRank[],
  help: CFG.help as {
    speedup_per_help_sec: number;
    max_helps_per_queue: number;
    max_reduction_ratio: number;
  },
  rally: CFG.rally as {
    min_rank: string;
    max_participants: number;
    prep_seconds: number;
    allowed_targets: string[];
  },
};

const PERMS: Record<string, string[]> = CFG.rank_permissions;

/** هل الرتبة تملك صلاحية معينة؟ الرتب غير المعروفة = لا صلاحيات. */
export function rankHas(rank: string, perm: string): boolean {
  return (PERMS[rank] || []).includes(perm);
}

/** رقم الرتبة (R1=1 … R5=5) للمقارنات؛ 0 لغير معروفة. */
export function rankLevel(rank: string): number {
  const i = ALLIANCE_CONSTANTS.ranks.indexOf(rank);
  return i < 0 ? 0 : i + 1;
}

export function isValidRank(rank: string): boolean {
  return ALLIANCE_CONSTANTS.ranks.includes(rank);
}

/** هل يجوز للرتبة مهاجمة/ترقية/تنزيل رتبة أخرى؟ فقط رتبة أعلى تُغيّر أدنى منها. */
export function canModerate(actorRank: string, targetRank: string): boolean {
  return rankLevel(actorRank) > rankLevel(targetRank) && rankLevel(actorRank) >= 4;
}

/**
 * حساب تقليل مدة الطابور من المساعدات:
 * كل مساعدة تخصم speedup_per_help_sec، بحد أقصى max_helps_per_queue مساعدة،
 * ولا يتجاوز التخفيض الكلي max_reduction_ratio من المدة الأصلية.
 */
export function helpSpeedupSec(remainingMs: number, helpsCount: number): number {
  const h = ALLIANCE_CONSTANTS.help;
  const capped = Math.min(helpsCount, h.max_helps_per_queue);
  const rawSec = capped * h.speedup_per_help_sec;
  const maxSec = (remainingMs / 1000) * h.max_reduction_ratio;
  return Math.min(rawSec, maxSec);
}

/** هل عدد المساعدات على طابور ما بلغ السقف؟ */
export function helpsCapped(helpsCount: number): boolean {
  return helpsCount >= ALLIANCE_CONSTANTS.help.max_helps_per_queue;
}

/** شروط إطلاق rally: رتبة كافية + هدف مسموح. */
export function canLaunchRally(rank: string, targetType: string): boolean {
  const r = ALLIANCE_CONSTANTS.rally;
  return rankLevel(rank) >= rankLevel(r.min_rank) && r.allowed_targets.includes(targetType);
}

/** هل اكتمل عدد المشاركين؟ */
export function rallyFull(participants: number): boolean {
  return participants >= ALLIANCE_CONSTANTS.rally.max_participants;
}

/** مدة التجمع قبل انطلاق rally بالمللي ثانية. */
export function rallyPrepMs(): number {
  return ALLIANCE_CONSTANTS.rally.prep_seconds * 1000;
}
