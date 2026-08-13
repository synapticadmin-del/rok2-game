// P10-T3: Sunset Canyon (ساحة 5×5) — منطق نقي. الثوابت كلها من data/canyon.json.
import { MS_PER_DAY, MS_PER_HOUR } from "../../lib/timeConstants";

export interface CanyonLimits { maxBuffsActive: number; seasonsOverlap: number; scoreDecayPctPerDay: number }
export interface CanyonStarRewards { canyonTokens: number; victoryPoints: number }
export interface CanyonSpec { arenaSize: number; buffs: Record<string, number>;
  buffsSources: { id: string; description: string; atkDef?: number; hp?: number; durationHours: number }[];
  challenges: { perDay: number; forces: { minTier: number; maxTier: number; description: string }; starRewards: Record<string, CanyonStarRewards> };
  season: { durationDays: number; leaderboardSize: number; seasonRewards: { rankCeil: number; tokens: number; title: string }[] };
  tokenShop: { items: { id: string; name: string; cost: number; reward: Record<string, Record<string, number> | number> }[] };
  limits: CanyonLimits }

export interface CanyonChallenge { id: string; seasonId: string; daySlot: number; stars: number; score: number }
export interface CanyonState {
  challenges: CanyonChallenge[];
  activeBuffs: { buffId: string; expiresAtMs: number }[];
  tokens: number;
  victoryPoints: number;
  currentSeasonId: string;
  seasonDay: number;
}

/** مفتاح الموسم حسب اليوم الموسمي (7 أيام). */
export function seasonIdForSeasonDay(spec: CanyonSpec, seasonStartMs: number, nowMs: number): string {
  const elapsed = Math.floor((nowMs - seasonStartMs) / MS_PER_DAY);
  const seasonNumber = Math.floor(elapsed / spec.season.durationDays) + 1;
  return `canyon_${seasonNumber}`;
}

/** إنشاء تحدي جديد: 5 تحديات/يوم بقوات T3 متطابقة — النتيجة تعتمد على أداء القادة. */
export function createChallenge(spec: CanyonSpec, state: CanyonState, nowMs: number, rand: () => number):
  { challenge?: CanyonChallenge; error?: string; newState: CanyonState } {
  const todayChallenges = state.challenges.filter(c => c.seasonId === state.currentSeasonId && c.daySlot === state.seasonDay);
  if (todayChallenges.length >= spec.challenges.perDay) return { error: "daily_challenges_exhausted", newState: state };
  const challenge: CanyonChallenge = {
    id: `cz_${state.currentSeasonId}_${state.seasonDay}_${todayChallenges.length}`,
    seasonId: state.currentSeasonId,
    daySlot: state.seasonDay,
    stars: 0,
    score: 0
  };
  return { challenge, newState: { ...state, challenges: [...state.challenges, challenge] } };
}

/** نتائج التحدي بنجوم 1-3 مع بافات canyon. */
export function completeChallenge(spec: CanyonSpec, state: CanyonState, challengeId: string, stars: number, nowMs: number):
  { error?: string; reward?: CanyonStarRewards; score: number; newState: CanyonState } {
  const idx = state.challenges.findIndex(c => c.id === challengeId);
  if (idx < 0) return { error: "unknown_challenge", score: 0, newState: state };
  const ch = state.challenges[idx];
  if (ch.seasonId !== state.currentSeasonId) return { error: "wrong_season", score: 0, newState: state };
  const starKey = `${stars}stars` as keyof typeof spec.challenges.starRewards;
  const reward = spec.challenges.starRewards[starKey];
  if (!reward) return { error: "invalid_stars", score: 0, newState: state };
  if (ch.stars >= stars) return { error: "already_completed", score: ch.score, newState: state };
  // بافات canyon: كل باف نشط يرفع النتيجة 5%
  const activeBuffCount = state.activeBuffs.filter(b => b.expiresAtMs > nowMs).length;
  const buffMultiplier = 1 + Math.min(activeBuffCount, spec.limits.maxBuffsActive) * 0.05;
  const score = Math.round(reward.victoryPoints * buffMultiplier);
  const updated = [...state.challenges];
  updated[idx] = { ...ch, stars, score };
  return {
    reward,
    score,
    newState: { ...state, challenges: updated, tokens: state.tokens + reward.canyonTokens, victoryPoints: state.victoryPoints + score }
  };
}

/** تفعيل باف canyon (بحد maxBuffsActive). */
export function activateBuff(spec: CanyonSpec, state: CanyonState, buffId: string, nowMs: number):
  { error?: string; newState: CanyonState } {
  const buff = spec.buffsSources.find(b => b.id === buffId);
  if (!buff) return { error: "unknown_buff", newState: state };
  const active = state.activeBuffs.filter(b => b.expiresAtMs > nowMs);
  if (active.length >= spec.limits.maxBuffsActive) return { error: "buff_slots_full", newState: state };
  return { newState: { ...state, activeBuffs: [...active, { buffId, expiresAtMs: nowMs + buff.durationHours * MS_PER_HOUR }] } };
}

/** مكافآت الموسم حسب الترتيب. */
export function seasonReward(spec: CanyonSpec, rank: number): { tokens: number; title: string; rank: number } | null {
  const entry = spec.season.seasonRewards.find(r => rank <= r.rankCeil);
  return entry ? { tokens: entry.tokens, title: entry.title, rank } : null;
}

/** شراء عنصر من متجر canyon tokens. */
export function buyTokenItem(spec: CanyonSpec, state: CanyonState, itemId: string):
  { error?: string; item?: CanyonSpec["tokenShop"]["items"][0]; newState: CanyonState } {
  const item = spec.tokenShop.items.find(i => i.id === itemId);
  if (!item) return { error: "unknown_item", newState: state };
  if (state.tokens < item.cost) return { error: "insufficient_tokens", newState: state };
  return { item, newState: { ...state, tokens: state.tokens - item.cost } };
}
