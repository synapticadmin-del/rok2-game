#!/usr/bin/env node
/**
 * Offline unit checks for the hospital system (P2-T2).
 * Replicates sim/hospital.ts math against data/buildings.json.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const buildingsData = JSON.parse(readFileSync(join(here, "../src/data/buildings.json"), "utf8"));
const troopTiers = JSON.parse(readFileSync(join(here, "../src/data/troop_tiers.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const CFG = buildingsData.hospital;
assert(!!CFG && CFG.base_capacity > 0 && CFG.capacity_per_level > 0, "buildings.json exposes hospital config");

// trainCost replica (from gameData.ts)
const unitCosts = {
  infantry_t1: { food: 50, wood: 20, stone: 0, gold: 0 },
  cavalry_t1: { food: 60, wood: 40, stone: 0, gold: 10 },
  archer_t1: { food: 40, wood: 50, stone: 0, gold: 5 },
};
const trainCost = (u, n) => ({
  food: (unitCosts[u]?.food ?? 50) * n,
  wood: (unitCosts[u]?.wood ?? 20) * n,
  stone: (unitCosts[u]?.stone ?? 0) * n,
  gold: (unitCosts[u]?.gold ?? 0) * n,
});

// hospital.ts replicas
const hospitalCapacity = (lvl) => (lvl <= 0 ? 0 : CFG.base_capacity + CFG.capacity_per_level * (lvl - 1));
const healCost = (troops) => {
  const t = { food: 0, wood: 0, stone: 0, gold: 0 };
  for (const [u, c] of Object.entries(troops)) {
    const cc = trainCost(u, c);
    t.food += Math.floor(cc.food * CFG.heal_cost_factor);
    t.wood += Math.floor(cc.wood * CFG.heal_cost_factor);
    t.stone += Math.floor(cc.stone * CFG.heal_cost_factor);
    t.gold += Math.floor(cc.gold * CFG.heal_cost_factor);
  }
  return t;
};
const healDurationSec = (n) => CFG.heal_seconds_per_troop * n;
const admitWounded = (severely, already, lvl) => {
  const cap = hospitalCapacity(lvl);
  const cur = Object.values(already).reduce((s, n) => s + n, 0);
  let free = Math.max(0, cap - cur);
  const admitted = {}, died = {};
  for (const [u, c] of Object.entries(severely)) {
    const take = Math.min(c, free);
    if (take > 0) admitted[u] = take;
    if (c - take > 0) died[u] = c - take;
    free -= take;
  }
  return { admitted, died };
};

// capacity curve
assert(hospitalCapacity(0) === 0, "no hospital → no capacity");
assert(hospitalCapacity(1) === 200, `hospital L1 capacity = ${hospitalCapacity(1)} (200)`);
assert(hospitalCapacity(5) === 800, `hospital L5 capacity = ${hospitalCapacity(5)} (800)`);

// heal cost = half train cost
const cost = healCost({ infantry_t1: 100, cavalry_t1: 50 });
const trainHalf = trainCost("infantry_t1", 100).food / 2 + trainCost("cavalry_t1", 50).food / 2;
assert(cost.food === trainHalf, `heal food cost = ${cost.food} (${trainHalf} = half train)`);
assert(cost.gold === Math.floor(trainCost("cavalry_t1", 50).gold / 2), "heal gold cost half of cavalry train gold");

// heal duration
assert(healDurationSec(40) === 200, `heal 40 troops = ${healDurationSec(40)}s (200)`);

// admission respects capacity (fresh hospital)
let r = admitWounded({ infantry_t1: 250 }, {}, 1);
assert(r.admitted.infantry_t1 === 200 && r.died.infantry_t1 === 50, `250 wounded vs cap 200 → 200 admitted + 50 died (got ${JSON.stringify(r)})`);

// admission respects already-wounded occupancy
r = admitWounded({ infantry_t1: 100 }, { archer_t1: 150 }, 1);
assert(r.admitted.infantry_t1 === 50 && r.died.infantry_t1 === 50, `cap 200 with 150 inside → 50 admitted + 50 died (got ${JSON.stringify(r)})`);

// zero hospital level → everyone dies
r = admitWounded({ infantry_t1: 100 }, {}, 0);
assert(!r.admitted.infantry_t1 && r.died.infantry_t1 === 100, "no hospital → all wounded die");

// upgrade raises capacity
r = admitWounded({ infantry_t1: 700 }, {}, 5);
assert(r.admitted.infantry_t1 === 700, "hospital L5 (cap 800) admits 700");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE HOSPITAL CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
