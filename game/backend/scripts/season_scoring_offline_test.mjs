#!/usr/bin/env node
/**
 * Offline unit checks for Zone 3 core contest + season scoring (P3-T2):
 * driven only by data/zones.json (core_contest) and data/map_spec_coordinates.json
 * (zone3_objectives). Mirrors src/do/sim/zones.ts contest logic and the
 * KingdomShard scoring/capture behaviour.
 *
 * Covers:
 *  - core_contest config present in zones.json (no hardcoded scoring in code)
 *  - holdScorePerTick / coreGarrison / coreCaptureGain per kind (throne/fort/altar)
 *  - firstCaptureBonus value
 *  - coreContestActive gates scoring to throne open day (40)
 *  - zone3_objectives in map_spec: 4 outer forts + 4 side altars with valid positions
 *  - scoring accumulation simulation: hold + first-capture + leaderboard ordering
 *  - capture gain caps at 100 and uses power divisor per kind
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const ZONES = JSON.parse(readFileSync(join(here, "../src/data/zones.json"), "utf8"));
const MAP = JSON.parse(readFileSync(join(here, "../src/data/map_spec_coordinates.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ---- mirror of sim/zones.ts (P3-T2 contest helpers) ----
const BY_ID = Object.fromEntries(ZONES.zones.map((z) => [z.zone_id, z]));
const CORE = BY_ID[3] ?? {};
const CC = CORE.core_contest ?? {};
function contestCfg(kind) {
  const c = CC[kind] ?? {};
  return {
    holdScorePerTick: c.hold_score_per_tick ?? (kind === "throne" ? 1 : kind === "outer_fort" ? 0.5 : 0.25),
    captureGainBase: c.capture_gain_base ?? (kind === "side_altar" ? 30 : 35),
    captureGainPowerDiv: c.capture_gain_power_div ?? (kind === "side_altar" ? 25 : 20),
    garrison: c.garrison ?? (kind === "throne" ? 2000 : kind === "outer_fort" ? 800 : 400),
    firstCaptureBonus: CC.first_capture_bonus ?? 25,
  };
}
const holdScorePerTick = (k) => contestCfg(k).holdScorePerTick;
const coreGarrison = (k) => contestCfg(k).garrison;
const coreCaptureGain = (k, power) => {
  const c = contestCfg(k);
  return Math.min(100, c.captureGainBase + Math.floor(power / c.captureGainPowerDiv));
};
const firstCaptureBonus = () => CC.first_capture_bonus ?? 25;
const throneUnlockDay = () => (CORE.core_objective?.open_day ?? 40);
const coreContestActive = (day) => day >= throneUnlockDay();

// ---- 1. core_contest config present & complete ----
assert(CORE.core_contest && typeof CORE.core_contest === "object", "zone3 has core_contest config");
for (const kind of ["throne", "outer_fort", "side_altar"]) {
  const c = CC[kind];
  assert(c && typeof c.hold_score_per_tick === "number", `core_contest.${kind}.hold_score_per_tick is number`);
  assert(typeof c.capture_gain_base === "number", `core_contest.${kind}.capture_gain_base is number`);
  assert(typeof c.garrison === "number", `core_contest.${kind}.garrison is number`);
}
assert(typeof CC.first_capture_bonus === "number" && CC.first_capture_bonus > 0, `first_capture_bonus > 0 (${CC.first_capture_bonus})`);

// ---- 2. scoring rates ordered: throne > fort > altar ----
assert(holdScorePerTick("throne") > holdScorePerTick("outer_fort"), `throne rate (${holdScorePerTick("throne")}) > fort (${holdScorePerTick("outer_fort")})`);
assert(holdScorePerTick("outer_fort") > holdScorePerTick("side_altar"), `fort rate (${holdScorePerTick("outer_fort")}) > altar (${holdScorePerTick("side_altar")})`);
assert(holdScorePerTick("throne") === 1, "throne hold = 1/tick");
assert(holdScorePerTick("outer_fort") === 0.5, "fort hold = 0.5/tick");
assert(holdScorePerTick("side_altar") === 0.25, "altar hold = 0.25/tick");

// ---- 3. garrisons ordered & from JSON ----
assert(coreGarrison("throne") === 2000, "throne garrison 2000");
assert(coreGarrison("outer_fort") === 800, "fort garrison 800");
assert(coreGarrison("side_altar") === 400, "altar garrison 400");
assert(coreGarrison("throne") > coreGarrison("outer_fort") && coreGarrison("outer_fort") > coreGarrison("side_altar"), "garrisons descend throne>fort>altar");

// ---- 4. capture gain: base + power/div, capped at 100 ----
assert(coreCaptureGain("throne", 0) === 35, "throne gain base 35 at 0 power");
assert(coreCaptureGain("throne", 400) === 35 + 20, `throne gain 400 power = 55 (got ${coreCaptureGain("throne", 400)})`);
assert(coreCaptureGain("throne", 999999) === 100, "throne gain caps at 100");
assert(coreCaptureGain("side_altar", 0) === 30, "altar gain base 30 (div 25)");
assert(coreCaptureGain("side_altar", 250) === 30 + 10, `altar gain 250 power = 40 (got ${coreCaptureGain("side_altar", 250)})`);
assert(coreCaptureGain("outer_fort", 100) === 35 + 5, `fort gain 100 power = 40 (got ${coreCaptureGain("outer_fort", 100)})`);

// ---- 5. contest gating: active only from throne open day (40) ----
assert(throneUnlockDay() === 40, `throne open day 40 (got ${throneUnlockDay()})`);
assert(!coreContestActive(39) && coreContestActive(40), "contest locked@39, active@40");
assert(coreContestActive(60), "contest active through end of season");

// ---- 6. zone3_objectives in map_spec: 4 forts + 4 altars, valid positions inside CORE ----
const z3o = MAP.zone3_objectives;
assert(z3o && Array.isArray(z3o.outer_forts) && z3o.outer_forts.length === 4, `4 outer forts (got ${z3o.outer_forts?.length})`);
assert(Array.isArray(z3o.side_altars) && z3o.side_altars.length === 4, `4 side altars (got ${z3o.side_altars?.length})`);
const coreAabb = MAP.regions.find((r) => r.id === "CORE")?.aabb;
assert(coreAabb && coreAabb.length === 4, "CORE region aabb present");
const inCore = (p) => p[0] >= coreAabb[0] && p[1] >= coreAabb[1] && p[0] <= coreAabb[2] && p[1] <= coreAabb[3];
for (const f of z3o.outer_forts) assert(inCore(f.pos), `fort ${f.id} inside CORE (${f.pos})`);
for (const a of z3o.side_altars) assert(inCore(a.pos), `altar ${a.id} inside CORE (${a.pos})`);
// unique ids
const ids = [...z3o.outer_forts.map(f=>f.id), ...z3o.side_altars.map(a=>a.id)];
assert(new Set(ids).size === 8, `8 unique objective ids (got ${new Set(ids).size})`);

// ---- 7. scoring accumulation simulation (hold + first-capture bonus + leaderboard) ----
// Simulate 100 ticks of season day 40+: alliance A owns throne+1 fort, B owns 2 altars.
const scores = new Map();
function award(al, pts) { scores.set(al, (scores.get(al) || 0) + pts); }
// first-capture bonuses (one-time)
award("A", firstCaptureBonus()); // throne
award("A", firstCaptureBonus()); // fort
award("B", firstCaptureBonus()); // altar1
award("B", firstCaptureBonus()); // altar2
// 100 ticks of holding
for (let t = 0; t < 100; t++) {
  award("A", holdScorePerTick("throne") + holdScorePerTick("outer_fort"));
  award("B", holdScorePerTick("side_altar") * 2);
}
const aFinal = scores.get("A"), bFinal = scores.get("B");
// A: 25+25 + 100*(1+0.5) = 50+150 = 200 ; B: 25+25 + 100*0.5 = 50+50 = 100
assert(aFinal === 200, `A total 200 after 100 ticks (got ${aFinal})`);
assert(bFinal === 100, `B total 100 after 100 ticks (got ${bFinal})`);
const leaderboard = [...scores.entries()].map(([allianceId, points]) => ({ allianceId, points })).sort((x, y) => y.points - x.points);
assert(leaderboard[0].allianceId === "A", "A leads the season scoreboard");
assert(leaderboard[0].points > leaderboard[1].points, "leader has strictly more points");

// ---- 8. no contest scoring before day 40 (gate) ----
const scoresEarly = new Map();
function tickEarly(day) { if (coreContestActive(day)) scoresEarly.set("A", (scoresEarly.get("A")||0) + holdScorePerTick("throne")); }
for (let d = 35; d < 40; d++) tickEarly(d);
assert((scoresEarly.get("A") || 0) === 0, "no points scored before day 40 (contest locked)");

// ---- 9. capture reset semantics: enemy capture starts from gain, own capture accumulates ----
function applyCapture(obj, gain, attackerAlliance) {
  if (obj.ownerAllianceId && obj.ownerAllianceId !== attackerAlliance) obj.captureProgress = gain;
  else obj.captureProgress = Math.min(100, obj.captureProgress + gain);
  obj.state = "contested";
  if (obj.captureProgress >= 100) { obj.ownerAllianceId = attackerAlliance; obj.captureProgress = 100; obj.state = "open"; }
  return obj;
}
let fort = { ownerAllianceId: "B", captureProgress: 60, state: "open" };
fort = applyCapture(fort, 40, "A");
assert(fort.captureProgress === 40 && fort.ownerAllianceId === "B", "enemy capture resets progress to gain (40), still B-owned");
fort = applyCapture(fort, 100, "A");
assert(fort.captureProgress === 100 && fort.ownerAllianceId === "A" && fort.state === "open", "a strong enough hit (gain>=100) captures fort for A");
// own-alliance reinforcement accumulates to 100
let ownFort = { ownerAllianceId: "A", captureProgress: 70, state: "contested" };
ownFort = applyCapture(ownFort, 40, "A");
assert(ownFort.captureProgress === 100 && ownFort.ownerAllianceId === "A" && ownFort.state === "open", "own-alliance reinforce accumulates 70+40->100 and secures");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE SEASON SCORING CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
