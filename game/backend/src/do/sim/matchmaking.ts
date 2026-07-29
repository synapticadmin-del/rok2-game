// P4-T6: matchmaking ممالك — تعيين اللاعبين الجدد على ممالك الإطلاق المفتوحة بشكل متوازن.
// كل القيم تُقرأ من data/matchmaking.json (+ ممالك الإطلاق من data/softlaunch.json) — لا ثوابت هنا.
import matchmakingData from "../../data/matchmaking.json";

const CFG = matchmakingData as any;

export type MatchmakingStrategy = "least_fill" | "round_robin";

export type KingdomCandidate = {
  id: string;
  open: boolean;
  max_players: number;
};

export type KingdomChoice = {
  kingdomId: string;
  strategy: MatchmakingStrategy;
  fillRatio: number;
  reason: string;
};

export function matchmakingConfig() {
  return CFG;
}

export function matchmakingStrategy(): MatchmakingStrategy {
  return (CFG.strategy as MatchmakingStrategy) || "least_fill";
}

export function guardrails(): { hard_cap_ratio: number; prefer_below_ratio: number; fallback_to_env_kingdom: boolean } {
  return CFG.guardrails;
}

/** نسبة امتلاء مملكة = عدد اللاعبين / السعة (1.0 = ممتلئة). */
export function fillRatio(playerCount: number, maxPlayers: number): number {
  if (maxPlayers <= 0) return 1;
  return playerCount / maxPlayers;
}

/**
 * اختيار مملكة للاعب جديد حسب الاستراتيجية.
 * counts: عدد اللاعبين الحاليين لكل مملكة مرشحة.
 * rrCounters: عدد تعيينات سابقة لكل مملكة (round_robin فقط).
 * يُعيد null إن كانت كل الممالك المفتوحة ممتلئة فوق hard_cap_ratio.
 */
export function chooseKingdom(
  candidates: KingdomCandidate[],
  counts: Record<string, number>,
  rrCounters: Record<string, number> = {},
): KingdomChoice | null {
  const open = candidates.filter((k) => k.open && k.max_players > 0);
  if (open.length === 0) return null;

  const g = guardrails();
  const viable = open.filter((k) => fillRatio(counts[k.id] || 0, k.max_players) < g.hard_cap_ratio);
  if (viable.length === 0) return null;

  const strategy = matchmakingStrategy();

  if (strategy === "round_robin") {
    // الأقل تعييناً سابقاً يفوز؛ كسر التعادل أبجدياً للحتمية
    const sorted = [...viable].sort((a, b) => {
      const da = rrCounters[a.id] || 0;
      const db = rrCounters[b.id] || 0;
      if (da !== db) return da - db;
      return a.id.localeCompare(b.id);
    });
    const pick = sorted[0];
    return {
      kingdomId: pick.id,
      strategy,
      fillRatio: fillRatio(counts[pick.id] || 0, pick.max_players),
      reason: `round_robin: least_assigned (${rrCounters[pick.id] || 0})`,
    };
  }

  // least_fill (الافتراضي): الأقل امتلاءً؛ كسر التعادل: الأقل عدداً ثم الأبجدي
  const sorted = [...viable].sort((a, b) => {
    const fa = fillRatio(counts[a.id] || 0, a.max_players);
    const fb = fillRatio(counts[b.id] || 0, b.max_players);
    if (fa !== fb) return fa - fb;
    const ca = counts[a.id] || 0;
    const cb = counts[b.id] || 0;
    if (ca !== cb) return ca - cb;
    return a.id.localeCompare(b.id);
  });
  const pick = sorted[0];
  const fr = fillRatio(counts[pick.id] || 0, pick.max_players);
  return {
    kingdomId: pick.id,
    strategy,
    fillRatio: fr,
    reason: `least_fill: fill=${(fr * 100).toFixed(1)}% of ${open.length} open kingdom(s)`,
  };
}
