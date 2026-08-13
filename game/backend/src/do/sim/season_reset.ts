/**
 * P12-T6: جاهزية الموسم الثاني — نهاية الموسم وتقريره وإعادة الضبط الموسمي (منطق نقي).
 *
 * - `computeSeasonReport`: يحوّل حالة العالم في الشارد إلى تقرير نهاية موسم نهائي
 *   (بطل الموسم، التحالفات المصنفة، أرقام قياسية). لا يقرأ من أي مصدر خارج ملفات التصميم.
 * - `legacyPointsFor`: يحوّل نقاط موسم اللاعب/التحالف إلى نقاط Legacy دائمة (من data/shop.json constants).
 * - `resetWorldForSeason`: وصف تعديلات الحالة اللازمة للموسم التالي (بدون مسح بيانات دائمة).
 *
 * لا يحتوي هذا الملف على أي حالة — كل الدوال pure.
 */

export type SeasonReportInput = {
  seasonId: string;
  shop: any;
  championAllianceId: string | null;
  championScore: number;
  throneScores: Array<{ allianceId: string; score: number }>;
  allianceScores: Array<{ allianceId: string; score: number }>;
  playerScores: Array<{ playerId: string; score: number }>;
  passesConquered: number;
  zonesUnlocked: number;
  citiesCount: number;
  lkCitadelsDestroyed: number;
  lkMigrants: number;
  storyEvents: number;
};

export type SeasonReport = {
  seasonId: string;
  generatedAt: number;
  championAllianceId: string | null;
  championScore: number;
  topAlliances: Array<{ allianceId: string; score: number; rank: number }>;
  topPlayers: Array<{ playerId: string; score: number; rank: number }>;
  stats: {
    passesConquered: number;
    zonesUnlocked: number;
    citiesCount: number;
    lkCitadelsDestroyed: number;
    lkMigrants: number;
    storyEvents: number;
  };
  legacy: {
    alliances: Array<{ allianceId: string; legacyPoints: number }>;
    players: Array<{ playerId: string; legacyPoints: number }>;
  };
};

/** أعلى عدد تحالفات/لاعبين يُدخلون في تقرير نهاية الموسم وترتيبهم. */
const TOP_N = 50;

/** صيغة نقاط Legacy من نقاط الموسم — تُقرأ من data/shop.json (constants.season.legacy_per_1000). */
export function legacyConfig(shop: any): { pointsPerScoreUnit: number } {
  const s = shop?.constants?.season || {};
  return { pointsPerScoreUnit: Number(s.legacy_per_1000 ?? 10) / 1000 };
}

/** ينتج تقرير نهاية موسم نهائيًا من حالة العالم. */
export function computeSeasonReport(input: SeasonReportInput, nowMs: number): SeasonReport {
  const topAlliances = [...input.throneScores]
    .sort((a, b) => b.score - a.score || a.allianceId.localeCompare(b.allianceId))
    .slice(0, TOP_N)
    .map((t, i) => ({ allianceId: t.allianceId, score: t.score, rank: i + 1 }));
  const topPlayers = [...input.playerScores]
    .sort((a, b) => b.score - a.score || a.playerId.localeCompare(b.playerId))
    .slice(0, TOP_N)
    .map((t, i) => ({ playerId: t.playerId, score: t.score, rank: i + 1 }));
  const lp = legacyConfig(input.shop).pointsPerScoreUnit;
  return {
    seasonId: input.seasonId,
    generatedAt: nowMs,
    championAllianceId: input.championAllianceId,
    championScore: input.championScore,
    topAlliances,
    topPlayers,
    stats: {
      passesConquered: input.passesConquered,
      zonesUnlocked: input.zonesUnlocked,
      citiesCount: input.citiesCount,
      lkCitadelsDestroyed: input.lkCitadelsDestroyed,
      lkMigrants: input.lkMigrants,
      storyEvents: input.storyEvents,
    },
    legacy: {
      alliances: input.throneScores.map((t) => ({ allianceId: t.allianceId, legacyPoints: Math.floor(t.score * lp) })),
      players: input.playerScores.map((t) => ({ playerId: t.playerId, legacyPoints: Math.floor(t.score * lp) })),
    },
  };
}

/** نوع تعديل حالة العالم في إعادة الضبط الموسمي. */
export type SeasonResetOp =
  | { kind: "throne"; ownerAllianceId: null; captureProgress: 0; unlockDay: number }
  | { kind: "holySite"; ownerId: string; progress: 0 }
  | { kind: "coreObjective"; ownerId: string; progress: 0; firstCapturedBy: null }
  | { kind: "passOwner"; passId: string; ownerAllianceId: null; captureProgress: 0 }
  | { kind: "scores"; allianceId?: string; playerId?: string; reset: "alliance" | "player" }
  | { kind: "king"; crownedAtMs: null; expiresAtMs: null }
  | { kind: "flags"; allianceId: string };

/** وصف إعادة الضبط الموسمي المطلوب — تنفيذ التعديلات في الشارد حسب kind. */
export function resetWorldForSeason(input: {
  throneUnlockDay: number;
  passIds: string[];
  holySiteIds: string[];
  coreObjectiveIds: string[];
  allianceIds: string[];
  playerIds: string[];
}): SeasonResetOp[] {
  const ops: SeasonResetOp[] = [];
  ops.push({ kind: "throne", ownerAllianceId: null, captureProgress: 0, unlockDay: input.throneUnlockDay });
  for (const id of input.passIds) ops.push({ kind: "passOwner", passId: id, ownerAllianceId: null, captureProgress: 0 });
  for (const id of input.holySiteIds) ops.push({ kind: "holySite", ownerId: id, progress: 0 });
  for (const id of input.coreObjectiveIds) ops.push({ kind: "coreObjective", ownerId: id, progress: 0, firstCapturedBy: null });
  for (const id of input.allianceIds) ops.push({ kind: "scores", allianceId: id, reset: "alliance" });
  for (const id of input.playerIds) ops.push({ kind: "scores", playerId: id, reset: "player" });
  ops.push({ kind: "king", crownedAtMs: null, expiresAtMs: null });
  return ops;
}

/** Legacy points من تقرير الموسم للاعب (من legacy.alliances/players). */
export function legacyPointsFromReport(report: SeasonReport, allianceId: string | null): number {
  if (allianceId) return report.legacy.alliances.find((a) => a.allianceId === allianceId)?.legacyPoints ?? 0;
  return 0;
}
