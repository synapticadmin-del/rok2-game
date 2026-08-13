// P8-T6: المهام اليومية والأسبوعية — منطق نقّي (pure logic) يُقرأ كل شيء من daily_quests.json
// 5 مهام يومية تُجدَّد يوميًا + 3 مهام أسبوعية. تقدم من مصادر أحداث (train/battle_win/barb_kill/gather/help/research_start/speedup/build_upgrade).
// اكتمال 100 نقطة يومية → مفتاح ذهبي. اكتمال 300 أسبوعية → صندوق أسبوعي.
import questData from "../../data/daily_quests.json";

export const QUESTS = questData as unknown as {
  version: number;
  constants: {
    daily_quest_count: number;
    daily_points_limit: number;
    weekly_quest_count: number;
    weekly_points_limit: number;
    refresh_at_hour_utc: number;
  };
  rewards: {
    golden_key_cost_points: number;
    golden_key_item_id: string;
    golden_key_gems: number;
    weekly_chest_cost_points: number;
    weekly_chest_gems: number;
    weekly_chest_speedups: number;
    suggested_chest_gems: number;
    weekly_chest_speedup_id: string;
  };
  types: Record<
    string,
    {
      id: string;
      name: string;
      unit: string;
      progress_sources: string[];
      description: string;
    }
  >;
  daily_pools: Record<string, { goal_range: [number, number]; point_options: number[] }>;
  weekly_pools: Record<string, { goal_range: [number, number]; point_options: number[] }>;
  daily_order: string[];
  weekly_order: string[];
};

export type QuestDef = {
  typeId: string;
  goal: number;
  points: number;
};

export type QuestProgress = {
  id: string;
  typeId: string;
  goal: number;
  points: number;
  progress: number;
  claimed: boolean;
};

/** يوم UTC (يُستخدم لمفاتيح التوزيع والاستبدال) — نفس نمط shop/utcDay. */
export function questDay(nowMs: number): number {
  return Math.floor(nowMs / 86_400_000);
}

/** أسبوع ISO (يُستخدم لتوزيع/استبدال المهام الأسبوعية). */
export function questWeek(nowMs: number): number {
  return Math.floor(nowMs / (7 * 86_400_000));
}

/** توليد رقم شبه عشوائي حتمي من seed (LCG بسيط — كافٍ للتوزيع المتساوي). */
export function seededRandom(seed: number): () => number {
  let s = seed >>> 0 || 1;
  return () => {
    s = (s * 1664525 + 1013904223) >>> 0;
    return s / 0x100000000;
  };
}

function choiceInRange(rng: () => number, lo: number, hi: number): number {
  return lo + Math.floor(rng() * (hi - lo + 1));
}

function pickPoints(rng: () => number, options: number[]): number {
  return options[Math.floor(rng() * options.length)] ?? options[0];
}

/** توزيع مهام يومية حتمي: نفس اللاعب في نفس اليوم يحصل على نفس المجموعة (بـ playerId + day seed). */
export function buildDailyQuests(playerId: string, day: number, rng: () => number = seededRandom(hashPlayerDay(playerId, day))): QuestDef[] {
  const order = QUESTS.daily_order;
  const count = QUESTS.constants.daily_quest_count;
  const picked: string[] = [];
  const available = order.slice();
  for (let i = 0; i < count; i++) {
    const idx = Math.floor(rng() * available.length);
    picked.push(available.splice(idx, 1)[0]);
  }
  let pointsSum = 0;
  const quests: QuestDef[] = picked.map((typeId) => {
    const pool = QUESTS.daily_pools[typeId];
    if (!pool) throw new Error(`Unknown daily quest type: ${typeId}`);
    const points = pickPoints(rng, pool.point_options);
    pointsSum += points;
    return { typeId, goal: choiceInRange(rng, pool.goal_range[0], pool.goal_range[1]), points };
  });
  // P8-T6: سقف النقاط اليومية — إذا تجاوزت المهام الخمس السقف نخفض أكبر مهمة
  const limit = QUESTS.constants.daily_points_limit;
  while (pointsSum > limit && quests.length > 0) {
    const biggest = quests.reduce((a, b) => (b.points > a.points ? b : a), quests[0]);
    const reduction = Math.min(biggest.points, pointsSum - limit);
    biggest.points -= reduction;
    pointsSum -= reduction;
  }
  return quests.map((q, i) => ({ ...q, id: `daily_${day}_${i}` }));
}

/** توزيع مهام أسبوعية حتمي بنفس الآلية. */
export function buildWeeklyQuests(playerId: string, week: number, rng: () => number = seededRandom(hashPlayerWeek(playerId, week))): QuestDef[] {
  const order = QUESTS.weekly_order;
  const count = QUESTS.constants.weekly_quest_count;
  const picked: string[] = [];
  const available = order.slice();
  for (let i = 0; i < count; i++) {
    const idx = Math.floor(rng() * available.length);
    picked.push(available.splice(idx, 1)[0]);
  }
  return picked.map((typeId, i) => {
    const pool = QUESTS.weekly_pools[typeId];
    if (!pool) throw new Error(`Unknown weekly quest type: ${typeId}`);
    return { id: `weekly_${week}_${i}`, typeId, goal: choiceInRange(rng, pool.goal_range[0], pool.goal_range[1]), points: pickPoints(rng, pool.point_options) };
  });
}

/** ترقية التقدم لمهمة: تعيد التقدم الجديد (capped) مع حساب نقاط مكتسبة جديدة فقط. */
export function applyProgress(quest: QuestProgress, source: string, amount: number): { progress: number; pointsEarned: number } {
  const def = QUESTS.types[quest.typeId];
  if (!def) return { progress: quest.progress, pointsEarned: 0 };
  if (!def.progress_sources.includes(source)) return { progress: quest.progress, pointsEarned: 0 };
  if (quest.claimed) return { progress: quest.progress, pointsEarned: 0 };
  const before = quest.progress;
  const after = Math.min(quest.goal, before + amount);
  return { progress: after, pointsEarned: after >= quest.goal && before < quest.goal ? quest.points : 0 };
}

/** هل يمكن استرداد المكافأة الآن؟ (اكتمال غير مسترد + الحد اليومي/الأسبوعي) */
export function canClaim(pointsTotal: number, claimed: boolean, limit: number, claimedBefore: boolean): { ok: boolean; reason?: string } {
  if (claimedBefore) return { ok: false, reason: "already_claimed" };
  if (!claimed) return { ok: false, reason: "quest_not_complete" };
  if (pointsTotal > limit) return { ok: false, reason: "daily_points_limit" };
  return { ok: true };
}

/** هل يستحق اللاعب مفتاحًا ذهبيًا؟ (مجموع نقاط اليوم >= حد المكافأة) */
export function goldenKeyEligible(pointsTotal: number): boolean {
  return pointsTotal >= QUESTS.rewards.golden_key_cost_points;
}

export function weeklyChestEligible(pointsTotal: number): boolean {
  return pointsTotal >= QUESTS.rewards.weekly_chest_cost_points;
}

/** وصف نصي من القالب (description: "درّب {goal} جنديًا"). */
export function questDescription(typeId: string, goal: number): string {
  const def = QUESTS.types[typeId];
  if (!def) return typeId;
  return def.description.replace("{goal}", String(goal));
}

// ---------- hashing helpers ----------
function hashPlayerDay(playerId: string, day: number): number {
  return hash(`${playerId}|daily|${day}`);
}
function hashPlayerWeek(playerId: string, week: number): number {
  return hash(`${playerId}|weekly|${week}`);
}
function hash(s: string): number {
  let h = 5381;
  for (let i = 0; i < s.length; i++) {
    h = ((h << 5) + h + s.charCodeAt(i)) >>> 0;
  }
  return h;
}
