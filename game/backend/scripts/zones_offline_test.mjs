#!/usr/bin/env node
/**
 * Offline unit checks for Zone 2 stubs (P2-T4):
 * time-gated zones + richer resource areas, driven only by data/zones.json
 * and data/map_spec_coordinates.json (mirrors src/do/sim/zones.ts logic).
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

// ---- mirror of sim/zones.ts ----
const C = {
  resourceAmountPerLevel: ZONES.constants.resource_amount_per_level,
  resourceBaseAmount: ZONES.constants.resource_base_amount,
  zone2RichnessMult: ZONES.constants.zone2_richness_mult,
  barbHpPerLevel: ZONES.constants.barb_hp_per_level,
};
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
function nodeLevelForRegion(regionId, zoneId, seed) {
  const [min, max] = BY_ID[zoneId].resource_level_range;
  const span = Math.max(1, max - min + 1);
  let h = 0;
  const s = `${regionId ?? ""}:${seed}`;
  for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) >>> 0;
  return min + (h % span);
}
function nodeRichness(kind, level, zoneId) {
  if (kind === "barb") return C.barbHpPerLevel * level;
  const base = C.resourceBaseAmount + C.resourceAmountPerLevel * (level - 1);
  const mult = zoneId >= 2 ? C.zone2RichnessMult : 1;
  return Math.floor(base * mult);
}

// ---- 1. data shape ----
assert(ZONES.constants && ZONES.constants.zone2_richness_mult > 1, `zone2 richness mult > 1 (${ZONES.constants.zone2_richness_mult})`);
const z2 = BY_ID[2];
assert(z2 && Array.isArray(z2.unlock_schedule) && z2.unlock_schedule.length === 2, "zone 2 has a 2-step unlock schedule");
assert(JSON.stringify(z2.resource_level_range) === "[3,6]", `zone2 resource range [3,6] (got ${z2.resource_level_range})`);
assert(JSON.stringify(BY_ID[1].resource_level_range) === "[1,4]", "zone1 resource range [1,4]");

// ---- 2. zone1 regions never time-locked ----
const z1Regions = MAP.regions.filter((r) => r.zone_id === 1);
assert(z1Regions.every((r) => regionUnlockDay(r.id, 1) === null), "zone1 regions: no time lock (open from day 0)");
assert(z1Regions.every((r) => isRegionUnlocked(r.id, 1, 0)), "zone1 regions unlocked at day 0");

// ---- 3. zone2 regions locked until day 10, open from day 10 ----
const z2Regions = MAP.regions.filter((r) => r.zone_id === 2);
assert(z2Regions.length === 4, `4 zone2 regions in map spec (got ${z2Regions.length})`);
assert(z2Regions.every((r) => regionUnlockDay(r.id, 2) === 10), "all zone2 regions unlock at day 10");
assert(z2Regions.every((r) => !isRegionUnlocked(r.id, 2, 9)), "zone2 regions locked at day 9 (stubs visible but closed)");
assert(z2Regions.every((r) => isRegionUnlocked(r.id, 2, 10)), "zone2 regions unlocked at day 10");
assert(z2Regions.every((r) => isRegionUnlocked(r.id, 2, 40)), "zone2 regions still unlocked at day 40");

// ---- 4. passes match the timer schedule (half at day 10, rest at day 14) ----
const innerPasses = MAP.passes.filter((p) => JSON.stringify(p.zone_link) === "[1,2]");
assert(innerPasses.length === 8, `8 zone1→zone2 inner passes (got ${innerPasses.length})`);
const day10 = innerPasses.filter((p) => passUnlockDay(p.id, p.zone_link) === 10).map((p) => p.id).sort();
const day14 = innerPasses.filter((p) => passUnlockDay(p.id, p.zone_link) === 14).map((p) => p.id).sort();
assert(day10.length === 4 && day14.length === 4, `inner passes split 4@day10 + 4@day14 (got ${day10.length}/${day14.length})`);
assert(JSON.stringify(day10) === JSON.stringify(["P_R2_Z2", "P_R4_Z2", "P_R6_Z2", "P_R8_Z2"]), `day-10 cardinal passes: ${day10}`);
assert(JSON.stringify(day14) === JSON.stringify(["P_R1_Z2", "P_R3_Z2", "P_R5_Z2", "P_R7_Z2"]), `day-14 corner passes: ${day14}`);
// pass unlock days already embedded in map_spec must agree with zones.json schedule
assert(innerPasses.every((p) => p.unlock_day === passUnlockDay(p.id, p.zone_link)), "map_spec pass unlock_day matches zones.json schedule");

// ---- 5. node levels stay inside each zone's resource range (higher in zone 2) ----
for (let k = 0; k < 5; k++) {
  const l1 = nodeLevelForRegion("R4", 1, `node_R4_${k}`);
  assert(l1 >= 1 && l1 <= 4, `zone1 node level in [1,4] (got ${l1})`);
  const l2 = nodeLevelForRegion("Z2N", 2, `node_Z2N_${k}`);
  assert(l2 >= 3 && l2 <= 6, `zone2 node level in [3,6] (got ${l2})`);
}
// deterministic: same seed → same level
assert(nodeLevelForRegion("Z2N", 2, "node_Z2N_0") === nodeLevelForRegion("Z2N", 2, "node_Z2N_0"), "node level deterministic per id");

// ---- 6. zone2 nodes are richer than zone1 at the same level ----
const r1 = nodeRichness("food", 3, 1);
const r2 = nodeRichness("food", 3, 2);
assert(r2 === Math.floor(r1 * C.zone2RichnessMult), `zone2 richness = zone1 × ${C.zone2RichnessMult} at same level (${r1} → ${r2})`);
const low = nodeRichness("food", 1, 1);
const high = nodeRichness("food", 6, 2);
assert(high > low * 2, `max zone2 node (${high}) much richer than base zone1 node (${low})`);
assert(nodeRichness("barb", 4, 2) === C.barbHpPerLevel * 4, "barb remaining scales with level only");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE ZONES CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
