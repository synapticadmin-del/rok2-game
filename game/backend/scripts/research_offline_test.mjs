#!/usr/bin/env node
/**
 * Offline unit checks for the research tree (P2-T3) against data/research.json.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const DATA = JSON.parse(readFileSync(join(here, "../src/data/research.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const BY_ID = Object.fromEntries(DATA.technologies.map((t) => [t.id, t]));
const cost = (id, lvl) => {
  const t = BY_ID[id]; const m = Math.pow(DATA.cost_mult, lvl - 1);
  return Math.floor(t.base_cost.food * m);
};
const dur = (id, lvl) => Math.floor(BY_ID[id].base_duration_sec * Math.pow(DATA.duration_mult, lvl - 1));
const academyReq = (id, lvl) => BY_ID[id].academy_base_req * lvl;
const buff = (levels, stat) => {
  let t = 0;
  for (const [id, lvl] of Object.entries(levels)) {
    const d = BY_ID[id];
    if (d && d.buff.stat === stat) t += d.buff.per_level * Math.min(lvl, d.max_level);
  }
  return t;
};
const prereqsMet = (id, levels) => BY_ID[id].prerequisites.every((p) => (levels[p.id] || 0) >= p.level);

// tree shape
const branches = {};
for (const t of DATA.technologies) branches[t.branch] = (branches[t.branch] || 0) + 1;
assert(branches.economy === 5 && branches.military === 5, `two branches: economy=5, military=5 (got ${JSON.stringify(branches)})`);
assert(DATA.technologies.every((t) => t.max_level === 5 && t.buff && t.base_cost), "every tech: max_level 5 + buff + base_cost");

// cost/duration scaling
assert(cost("eco_production", 1) === 400, `eco_production L1 food cost = ${cost("eco_production", 1)} (400)`);
assert(cost("eco_production", 3) === Math.floor(400 * DATA.cost_mult ** 2), "cost scales by cost_mult^level");
assert(dur("mil_attack", 1) === 60 && dur("mil_attack", 2) === 90, `duration scales: L1=60s, L2=${dur("mil_attack", 2)}s (90)`);
assert(academyReq("eco_masonry", 2) === 4, `masonry L2 academy req = ${academyReq("eco_masonry", 2)} (4)`);

// buffs accumulate
assert(Math.abs(buff({ eco_production: 5 }, "resource_production") - 0.15) < 1e-9, "eco_production L5 → +15% production");
assert(Math.abs(buff({ mil_attack: 5, mil_tactics: 5 }, "troop_attack") - 0.30) < 1e-9, "mil_attack L5 + mil_tactics L5 → +30% attack");
assert(buff({}, "troop_attack") === 0, "no research → no buff");

// prerequisites
assert(prereqsMet("eco_masonry", { eco_production: 2 }), "masonry unlocked at eco_production L2");
assert(!prereqsMet("eco_masonry", { eco_production: 1 }), "masonry blocked below eco_production L2");
assert(prereqsMet("mil_tactics", { mil_attack: 3, mil_defense: 3 }), "mil_tactics needs attack L3 + defense L3");
assert(!prereqsMet("mil_tactics", { mil_attack: 3, mil_defense: 2 }), "mil_tactics blocked when defense < L3");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE RESEARCH CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
