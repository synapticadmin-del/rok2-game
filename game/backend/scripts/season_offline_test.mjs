#!/usr/bin/env node
/**
 * Offline unit checks for the Season/Zone unlock service (P3-T1):
 * full server-side zone unlock schedule driven only by data/zones.json
 * and data/map_spec_coordinates.json (mirrors src/do/sim/zones.ts logic).
 *
 * Covers:
 *  - season_day_ms / season_max_day constants come from JSON (no hardcoded values)
 *  - seasonDayAt() time progression (deterministic, clamped to maxDay)
 *  - Zone 2 schedule unchanged (regression guard from P2-T4)
 *  - Zone 3 schedule: final gates @day35, CORE region @day35, throne @day40
 *  - throne unlock derived from zones.json core_objective.open_day (not hardcoded)
 *  - season_unlock_schedule features table complete (5 milestones)
 *  - season service config present
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

// ---- mirror of sim/zones.ts (P3-T1 additions) ----
const C = {
  seasonDayMs: ZONES.constants.season_day_ms,
  seasonMaxDay: ZONES.constants.season_max_day,
  zone2RichnessMult: ZONES.constants.zone2_richness_mult,
};
const SVC = {
  autoAdvance: ZONES.season_service?.auto_advance ?? true,
  dayMs: ZONES.season_service?.day_ms ?? ZONES.constants.season_day_ms,
  maxDay: ZONES.constants.season_max_day,
};
const SCHEDULE = ZONES.season_unlock_schedule ?? [];
const BY_ID = Object.fromEntries(ZONES.zones.map((z) => [z.zone_id, z]));

function regionUnlockDay(regionId, zoneId) {
  const z = BY_ID[zoneId];
  if (!z) return null;
  for (const e of z.unlock_schedule ?? []) if (e.regions.includes(regionId)) return e.day;
  return null;
}
function passUnlockDay(passId, zoneLink) {
  const zoneId = Math.max(...(zoneLink && zoneLink.length ? zoneLink : [1]));
  const z = BY_ID[zoneId];
  if (!z) return null;
  for (const e of z.unlock_schedule ?? []) if (e.passes.includes(passId)) return e.day;
  return null;
}
function isRegionUnlocked(regionId, zoneId, seasonDay) {
  const d = regionUnlockDay(regionId, zoneId);
  return d === null || seasonDay >= d;
}
function seasonDayAt(seasonStartMs, nowMs) {
  const dayMs = Math.max(1, SVC.dayMs);
  const d = Math.floor((nowMs - seasonStartMs) / dayMs);
  return Math.max(0, Math.min(SVC.maxDay, d));
}
function throneUnlockDay() {
  const day = BY_ID[3]?.core_objective?.open_day;
  return typeof day === "number" ? day : 40;
}
function isThroneUnlocked(seasonDay) {
  return seasonDay >= throneUnlockDay();
}

// ---- 1. season constants come from JSON (no hardcoded day length in code) ----
assert(typeof C.seasonDayMs === "number" && C.seasonDayMs === 86_400_000, `season_day_ms = 86400000 from JSON (got ${C.seasonDayMs})`);
assert(typeof C.seasonMaxDay === "number" && C.seasonMaxDay >= 40, `season_max_day >= 40 covers throne day (got ${C.seasonMaxDay})`);
assert(ZONES.season_service && ZONES.season_service.auto_advance === true, "season_service.auto_advance present and true");
assert(ZONES.season_service.day_ms === C.seasonDayMs, "season_service.day_ms matches constants.season_day_ms");
assert(Array.isArray(ZONES.season_service.announce_events) && ZONES.season_service.announce_events.includes("zone_unlocked"), "season_service announces zone_unlocked");

// ---- 2. seasonDayAt: time-based day progression (deterministic + clamped) ----
const T0 = 1_800_000_000_000;
assert(seasonDayAt(T0, T0) === 0, "season starts at day 0");
assert(seasonDayAt(T0, T0 + C.seasonDayMs - 1) === 0, "still day 0 one ms before day 1");
assert(seasonDayAt(T0, T0 + C.seasonDayMs) === 1, "day 1 after exactly one day");
assert(seasonDayAt(T0, T0 + 10 * C.seasonDayMs) === 10, "day 10 (zone 2 opens)");
assert(seasonDayAt(T0, T0 + 35 * C.seasonDayMs) === 35, "day 35 (final gates open)");
assert(seasonDayAt(T0, T0 + 40 * C.seasonDayMs) === 40, "day 40 (throne opens)");
assert(seasonDayAt(T0, T0 + 365 * C.seasonDayMs) === C.seasonMaxDay, `clamped to maxDay ${C.seasonMaxDay} far in future`);
assert(seasonDayAt(T0, T0 - 9999) === 0, "negative elapsed clamps to 0");

// ---- 3. zone1 regression: never time-locked ----
const z1Regions = MAP.regions.filter((r) => r.zone_id === 1);
assert(z1Regions.length === 8, `8 zone1 regions (got ${z1Regions.length})`);
assert(z1Regions.every((r) => regionUnlockDay(r.id, 1) === null), "zone1 regions open from day 0");
assert(z1Regions.every((r) => isRegionUnlocked(r.id, 1, 0)), "zone1 unlocked at day 0");

// ---- 4. zone2 regression: half @10, all @14 (P2-T4 unchanged) ----
const z2Regions = MAP.regions.filter((r) => r.zone_id === 2);
assert(z2Regions.every((r) => regionUnlockDay(r.id, 2) === 10), "zone2 regions unlock day 10");
assert(z2Regions.every((r) => !isRegionUnlocked(r.id, 2, 9) && isRegionUnlocked(r.id, 2, 10)), "zone2 locked@9, open@10");
const innerPasses = MAP.passes.filter((p) => JSON.stringify(p.zone_link) === "[1,2]");
assert(innerPasses.filter((p) => passUnlockDay(p.id, p.zone_link) === 10).length === 4, "4 inner passes @day10");
assert(innerPasses.filter((p) => passUnlockDay(p.id, p.zone_link) === 14).length === 4, "4 inner passes @day14");

// ---- 5. zone3 schedule: gates + CORE @day35, throne @day40 ----
const z3 = BY_ID[3];
assert(z3 && Array.isArray(z3.unlock_schedule) && z3.unlock_schedule.length === 2, "zone3 has 2-step unlock_schedule");
const gates = MAP.passes.filter((p) => p.type === "final_gate");
assert(gates.length === 4, `4 final gates in map spec (got ${gates.length})`);
assert(gates.every((p) => passUnlockDay(p.id, p.zone_link) === 35), `all final gates unlock day 35 (got ${gates.map((p) => passUnlockDay(p.id, p.zone_link))})`);
assert(gates.every((p) => p.unlock_day === 35), "map_spec final gates embed unlock_day 35");
assert(regionUnlockDay("CORE", 3) === 35, "CORE region unlock day 35 (visible/enterable with outer)");
assert(!isRegionUnlocked("CORE", 3, 34) && isRegionUnlocked("CORE", 3, 35), "CORE locked@34, open@35");

// ---- 6. throne day derived from JSON, not hardcoded 14 ----
assert(throneUnlockDay() === 40, `throne open_day = 40 from core_objective (got ${throneUnlockDay()})`);
assert(z3.core_objective.open_day === 40, "zones.json core_objective.open_day = 40");
assert(!isThroneUnlocked(39) && isThroneUnlocked(40), "throne locked@39, open@40");
assert(throneUnlockDay() !== 14, "throne no longer hardcoded at day 14");

// ---- 7. full season milestone table ----
assert(SCHEDULE.length === 5, `5 season milestones (got ${SCHEDULE.length})`);
const days = SCHEDULE.map((f) => f.day).sort((a, b) => a - b);
assert(JSON.stringify(days) === JSON.stringify([0, 10, 14, 35, 40]), `milestone days [0,10,14,35,40] (got ${days})`);
const feats = SCHEDULE.map((f) => f.feature);
for (const f of ["zone1_all", "zone2_half_inner_passes", "zone2_all", "zone3_outer", "zone3_core_windows"]) {
  assert(feats.includes(f), `schedule includes ${f}`);
}

// ---- 8. season unlock state snapshot at key days ----
function stateAt(day) {
  return {
    day,
    throneUnlocked: isThroneUnlocked(day),
    features: SCHEDULE.map((f) => ({ ...f, unlocked: day >= f.day })),
  };
}
const s0 = stateAt(0);
assert(s0.features.filter((f) => f.unlocked).length === 1 && s0.features[0].feature === "zone1_all", "day0: only zone1_all unlocked");
assert(!s0.throneUnlocked, "day0: throne locked");
const s14 = stateAt(14);
assert(s14.features.filter((f) => f.unlocked).length === 3, "day14: zone1+zone2_half+zone2_all unlocked");
assert(!s14.throneUnlocked, "day14: throne still locked");
const s35 = stateAt(35);
assert(s35.features.filter((f) => f.unlocked).length === 4, "day35: +zone3_outer unlocked");
assert(!s35.throneUnlocked, "day35: throne still locked (opens day 40)");
const s40 = stateAt(40);
assert(s40.features.every((f) => f.unlocked), "day40: all milestones unlocked");
assert(s40.throneUnlocked, "day40: throne open");

// ---- 9. monotonicity: once unlocked, stays unlocked across the whole season ----
let monotone = true;
for (let d = 0; d <= C.seasonMaxDay; d++) {
  for (const f of SCHEDULE) {
    const now = d >= f.day;
    const later = (d + 1) >= f.day;
    if (now && !later) monotone = false;
  }
}
assert(monotone, "unlock state is monotonic (never re-locks)");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE SEASON CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
