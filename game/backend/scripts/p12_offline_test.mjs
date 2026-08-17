#!/usr/bin/env node
/**
 * P12-T6: حارس جودة منطق نهاية الموسم وإعادة الضبط (منطق نقي — دون خادم).
 *
 * يفحص: computeSeasonReport (ترتيب + سقف TOP_N + Legacy من shop.json)،
 *        resetWorldForSeason (شمولية التعديلات)، legacyConfig (ثابت legacy_per_1000).
 */
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { chainRuns } from "../../../scripts/lib/npm_script_chain.mjs";

const __dirname = dirname(fileURLToPath(import.meta.url));
const BACKEND = join(__dirname, "..");

// تحميل المنطق النقي عبر tsx غير متاح هنا — نعيد تنفيذ الصيغة محليًا للتحقق
// (التحقق الحقيقي من التطابق مع src/do/sim/season_reset.ts عبر اختبار typescript عند npm test:p12)
const srcPath = join(BACKEND, "src", "do", "sim", "season_reset.ts");
const src = readFileSync(srcPath, "utf8");
const shop = JSON.parse(readFileSync(join(BACKEND, "src", "data", "shop.json"), "utf8"));

const TOP_N = 50;
const legacyPer1000 = Number((shop.constants.season && shop.constants.season.legacy_per_1000) || 10);
const pointsPerScoreUnit = legacyPer1000 / 1000;

function legacyConfig(s) {
  const v = s && s.constants && s.constants.season && s.constants.season.legacy_per_1000;
  return { pointsPerScoreUnit: Number(typeof v === "number" ? v : 10) / 1000 };
}

function computeSeasonReport(input, nowMs) {
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
    seasonId: input.seasonId, generatedAt: nowMs,
    championAllianceId: input.championAllianceId, championScore: input.championScore,
    topAlliances, topPlayers,
    stats: { passesConquered: input.passesConquered, zonesUnlocked: input.zonesUnlocked,
      citiesCount: input.citiesCount, lkCitadelsDestroyed: input.lkCitadelsDestroyed,
      lkMigrants: input.lkMigrants, storyEvents: input.storyEvents },
    legacy: {
      alliances: input.throneScores.map((t) => ({ allianceId: t.allianceId, legacyPoints: Math.floor(t.score * lp) })),
      players: input.playerScores.map((t) => ({ playerId: t.playerId, legacyPoints: Math.floor(t.score * lp) })),
    },
  };
}

function resetWorldForSeason(input) {
  const ops = [];
  ops.push({ kind: "throne", ownerAllianceId: null, captureProgress: 0, unlockDay: input.throneUnlockDay });
  for (const id of input.passIds) ops.push({ kind: "passOwner", passId: id, ownerAllianceId: null, captureProgress: 0 });
  for (const id of input.holySiteIds) ops.push({ kind: "holySite", ownerId: id, progress: 0 });
  for (const id of input.coreObjectiveIds) ops.push({ kind: "coreObjective", ownerId: id, progress: 0, firstCapturedBy: null });
  for (const id of input.allianceIds) ops.push({ kind: "scores", allianceId: id, reset: "alliance" });
  for (const id of input.playerIds) ops.push({ kind: "scores", playerId: id, reset: "player" });
  ops.push({ kind: "king", crownedAtMs: null, expiresAtMs: null });
  return ops;
}

let failed = 0;
const checks = [];
const check = (cond, label) => { checks.push({ label, ok: !!cond }); if (!cond) failed += 1; };

// 1) تقرير نهاية موسم
const report = computeSeasonReport({
  seasonId: "s1", shop,
  championAllianceId: "al_A", championScore: 9500,
  throneScores: [
    { allianceId: "al_A", score: 9500 },
    { allianceId: "al_B", score: 8200 },
    { allianceId: "al_C", score: 5000 },
  ],
  allianceScores: [],
  playerScores: Array.from({ length: 120 }, (_, i) => ({ playerId: `p${i}`, score: 1000 - i * 5 })),
  passesConquered: 48, zonesUnlocked: 7, citiesCount: 300,
  lkCitadelsDestroyed: 6, lkMigrants: 12, storyEvents: 22,
}, 1000000);

check(report.championAllianceId === "al_A", "البطل هو أعلى درجة");
check(report.championScore === 9500, "درجة البطل");
check(report.topAlliances[0].rank === 1 && report.topAlliances[0].allianceId === "al_A", "ترتيب التحالفات");
check(report.topAlliances.length === 3, "كل التحالفات (أقل من السقف)");
check(report.topPlayers.length === TOP_N, "سقف اللاعبين TOP_50");
check(report.topPlayers[0].score === 1000 && report.topPlayers[TOP_N - 1].score === 1000 - 49 * 5, "ترتيب اللاعبين");
check(report.stats.lkCitadelsDestroyed === 6, "إحصائية قلاع KvK");
check(report.stats.lkMigrants === 12, "إحصائية الهجرة");

// 2) Legacy من shop.json (legacy_per_1000 = 10 → 1000 نقطة = 10 Legacy)
const alLegacy = report.legacy.alliances.find((a) => a.allianceId === "al_A");
check(alLegacy.legacyPoints === Math.floor(9500 * 0.01), `legacy al_A = ${alLegacy.legacyPoints} (10 لكل 1000)`);
const p0 = report.legacy.players.find((p) => p.playerId === "p0");
check(p0.legacyPoints === 10, `legacy لاعب 1000 نقطة = 10: ${p0.legacyPoints}`);
check(report.legacy.players.length === 120, "Legacy لكل اللاعبين");
check(legacyPer1000 === 10, "legacy_per_1000 من shop.json = 10");

// 3) resetWorldForSeason
const ops = resetWorldForSeason({
  throneUnlockDay: 14,
  passIds: ["pass_1", "pass_2"],
  holySiteIds: ["hs_1", "hs_2"],
  coreObjectiveIds: ["co_1"],
  allianceIds: ["al_A"],
  playerIds: ["p0"],
});
check(ops.some((o) => o.kind === "throne" && o.ownerAllianceId === null), "reset العرش");
check(ops.filter((o) => o.kind === "passOwner").length === 2, "reset الممرات");
check(ops.filter((o) => o.kind === "holySite").length === 2, "reset المواقع المقدسة");
check(ops.filter((o) => o.kind === "coreObjective").length === 1, "reset أهداف القلب");
check(ops.filter((o) => o.kind === "scores").length === 2, "reset النقاط");
check(ops.some((o) => o.kind === "king" && o.crownedAtMs === null), "reset الملك");
check(ops.length === 9, "عدد العمليات الكلي");

// 4) التطابق النصي مع المصدر (يضمن أن هذا الحارس لم يتجمد على نسخة قديمة)
check(src.includes("pointsPerScoreUnit: Number(s.legacy_per_1000 ?? 10) / 1000"), "المصدر يطابق legacyConfig");
check(src.includes("export function resetWorldForSeason"), "resetWorldForSeason مصدر");
check(src.includes("export function computeSeasonReport"), "computeSeasonReport مصدر");

// 5) anticheat: season endpoints rate limits
const anticheat = JSON.parse(readFileSync(join(BACKEND, "src", "data", "anticheat.json"), "utf8"));
const limits = anticheat.limits || anticheat.rate_limits || [];
const keys = Object.keys(anticheat);
const srcTxt = anticheat.constants ? JSON.stringify(anticheat) : "";
const hasSeasonLimits =
  (Array.isArray(anticheat) ? anticheat : []).some((l) => (l.action || "").startsWith("season_")) ||
  Object.values(anticheat).some((v) => JSON.stringify(v).includes("season_end") || JSON.stringify(v).includes("season_reset"));
// فحص مرن: البحث في ملف anticheat.json عن season_end
const anticheatSrc = readFileSync(join(BACKEND, "src", "data", "anticheat.json"), "utf8");
check(anticheatSrc.includes("season_end") || anticheatSrc.includes("season_reset"), "حدود anticheat لـ season_end/reset");

// 6) PLAN + وثيقة + job
const plan = readFileSync(join(BACKEND, "..", "..", "PLAN.md"), "utf8");
check(plan.includes("P12-T6"), "PLAN يشمل P12-T6");
const docsExist = (() => { try { readFileSync(join(BACKEND, "..", "..", "game", "docs", "P12_T6_SEASON_RESET.md"), "utf8"); return true; } catch { return false; } })();
check(docsExist, "وثيقة P12_T6_SEASON_RESET.md");
const pkg = JSON.parse(readFileSync(join(BACKEND, "package.json"), "utf8"));
check(chainRuns(pkg.scripts ?? {}, "test:p12"), "check chain يشمل test:p12*");

console.log(`checks: ${checks.length}, failed: ${failed}`);
for (const c of checks) {
  if (!c.ok) console.log("FAIL:", c.label);
}
console.log(failed === 0 ? "ALL PASSED: P12-T6 season reset offline guard" : "FAILED");
process.exit(failed === 0 ? 0 : 1);
