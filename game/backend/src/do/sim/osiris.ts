// P10-T4: Ark of Osiris (تحالف ضد تحالف) — منطق نقي. الثوابت كلها من data/osiris.json.
import { MS_PER_HOUR } from "../../lib/timeConstants";

export interface OsirisFacility { id: string; name: string; capturePoints: number; pointsPerHoldHour: number }
export interface OsirisRoute { id: string; name: string; checkpoints: number }
export interface OsirisLimits { maxLeaguesPerSeason: number; minAllianceMembersToRegister: number; cooldownDaysAfterLeague: number }
export interface OsirisSpec { season: { durationDays: number; phases: { id: string; name: string; days: number[] }[]; playersPerSide: number; registrationWindowHours: number; matchmakingMinPlayers: number };
  structures: { facilities: OsirisFacility[]; captureRules: { attackPowerMultiplier: number; holdCapPerStructurePct: number; contestedThresholdPct: number } };
  ark: { routes: OsirisRoute[]; moveIntervalHours: number; pointsPerCheckpoint: number };
  scoring: { pointsPerMember: number; victoryThresholdPct: number; tiebreaker: string };
  rewards: { winnerAlliance: { gems: number; title: string; seasonStoryEntry: string }; loserAlliance: { gems: number; title: string }; mvpPlayer: { gems: number; sculptureShards: number } };
  limits: OsirisLimits }

export interface OsirisSide { allianceId: string; registered: string[]; points: number; facilityHours: Record<string, number>;
  arkRouteId: string | null; arkCheckpoint: number }

/** فحص شروط تسجيل التحالف في الدوري. */
export function canRegister(spec: OsirisSpec, side: OsirisSide, memberCount: number, activeLeagues: number):
  { ok: boolean; reason?: string } {
  if (side.registered.length >= spec.season.playersPerSide) return { ok: false, reason: "side_full" };
  if (memberCount < spec.limits.minAllianceMembersToRegister) return { ok: false, reason: "insufficient_members" };
  if (activeLeagues >= spec.limits.maxLeaguesPerSeason) return { ok: false, reason: "league_already_active" };
  return { ok: true };
}

/** احتلال منشأة: نقاط الالتقاط تتراكم حسب قوة الهجوم. */
export function attackFacility(spec: OsirisSpec, side: OsirisSide, facilityId: string, attackPower: number):
  { error?: string; captured: boolean; progressPct: number; newState: OsirisSide } {
  const facility = spec.structures.facilities.find(f => f.id === facilityId);
  if (!facility) return { error: "unknown_facility", captured: false, progressPct: 0, newState: side };
  const delta = attackPower * spec.structures.captureRules.attackPowerMultiplier;
  const capped = Math.min(delta, facility.capturePoints - (side.points % facility.capturePoints));
  return {
    captured: capped >= facility.capturePoints - (side.points % facility.capturePoints),
    progressPct: Math.round(((side.points % facility.capturePoints) + capped) / facility.capturePoints * 100),
    newState: { ...side, points: side.points + capped }
  };
}

/** نقل الفلك عبر مسار: checkpoins كل moveIntervalHours. */
export function moveArk(spec: OsirisSpec, side: OsirisSide, nowMs: number, lastMoveAtMs: number):
  { error?: string; moved: boolean; checkpoint: number; pointsEarned: number; newState: OsirisSide } {
  const route = side.arkRouteId ? spec.ark.routes.find(r => r.id === side.arkRouteId) : null;
  if (!route) return { error: "no_route_assigned", moved: false, checkpoint: side.arkCheckpoint, pointsEarned: 0, newState: side };
  const interval = spec.ark.moveIntervalHours * MS_PER_HOUR;
  if (nowMs - lastMoveAtMs < interval) return { error: "ark_on_cooldown", moved: false, checkpoint: side.arkCheckpoint, pointsEarned: 0, newState: side };
  const next = Math.min(side.arkCheckpoint + 1, route.checkpoints);
  const pointsEarned = next > side.arkCheckpoint ? spec.ark.pointsPerCheckpoint : 0;
  return {
    moved: next > side.arkCheckpoint,
    checkpoint: next,
    pointsEarned,
    newState: { ...side, arkCheckpoint: next, points: side.points + pointsEarned }
  };
}

/** نتيجة الدوري: الفائز حسب عتبة النصر. */
export function leagueResult(spec: OsirisSpec, a: OsirisSide, b: OsirisSide):
  { winner: OsirisSide; loser: OsirisSide; reason: string; tiebreakApplied: boolean } {
  const aPts = a.points + Object.values(a.facilityHours).reduce((s, h) => s + h * spec.scoring.pointsPerMember, 0);
  const bPts = b.points + Object.values(b.facilityHours).reduce((s, h) => s + h * spec.scoring.pointsPerMember, 0);
  const total = aPts + bPts || 1;
  if (Math.abs(aPts - bPts) / total < (1 - spec.scoring.victoryThresholdPct / 100)) {
    // تعادل أو نتيجة قريبة: tiebreaker على ساعات المنشآت
    const aH = Object.values(a.facilityHours).reduce((s, h) => s + h, 0);
    const bH = Object.values(b.facilityHours).reduce((s, h) => s + h, 0);
    return { winner: aH >= bH ? a : b, loser: aH >= bH ? b : a, reason: "tiebreaker_structure_hours", tiebreakApplied: true };
  }
  return { winner: aPts > bPts ? a : b, loser: aPts > bPts ? b : a, reason: "victory_threshold", tiebreakApplied: false };
}

/** مكافآت الدوري + مدخل في حكاية الموسم. */
export function leagueRewards(spec: OsirisSpec, winner: OsirisSide, loser: OsirisSide):
  { seasonStoryEntry: { entry: string; championAllianceId: string; title: string }; gems: Record<string, number>; titles: Record<string, string> } {
  return {
    seasonStoryEntry: { entry: spec.rewards.winnerAlliance.seasonStoryEntry, championAllianceId: winner.allianceId, title: spec.rewards.winnerAlliance.title },
    gems: { [winner.allianceId]: spec.rewards.winnerAlliance.gems, [loser.allianceId]: spec.rewards.loserAlliance.gems },
    titles: { [winner.allianceId]: spec.rewards.winnerAlliance.title, [loser.allianceId]: spec.rewards.loserAlliance.title }
  };
}
