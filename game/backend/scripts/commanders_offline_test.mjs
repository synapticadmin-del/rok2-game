#!/usr/bin/env node
/**
 * Offline unit checks for the commander combat math (P2-T1).
 * Replicates sim/commanders.ts + sim/combat.ts logic against data/commanders.json
 * (Workers TS isn't runnable directly in plain node, so we re-implement the
 * pure formulas here and assert the data + math contract).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const data = JSON.parse(readFileSync(join(here, "../src/data/commanders.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const DEFS = Object.fromEntries(data.commanders.map((c) => [c.id, c]));
const CONSTANTS = data.constants;

function mod(inst, type) {
  const def = DEFS[inst.commanderId];
  if (!def) return 0;
  let m = 0;
  def.skills.forEach((s, i) => {
    if (s.type !== type) return;
    const lvl = Math.max(0, Math.min(inst.skills[i] || 0, s.max_level));
    for (const e of s.effects) m += e.per_level * lvl;
  });
  return m;
}

function xpForLevel(level) {
  return CONSTANTS.xp_base + level * CONSTANTS.xp_level_mult;
}

function unitAtk(unitId) {
  let base = 10;
  if (unitId.includes("cavalry")) base = 12;
  else if (unitId.includes("archer")) base = 11;
  return base;
}
const troopPower = (t) => Object.entries(t).reduce((s, [u, c]) => s + unitAtk(u) * c, 0);

// data contract
assert(data.commanders.length === 12, "roster has 12 commanders");
assert(
  data.commanders.every((c) => c.skills.length === 3 && new Set(c.skills.map((s) => s.type)).size === 3),
  "each commander: exactly one attack + one defense + one passive",
);
const civs = JSON.parse(readFileSync(join(here, "../src/data/civilizations.json"), "utf8"));
assert(
  civs.civilizations.every((c) => DEFS[c.starter_commander]),
  "every civ starter_commander exists in commanders.json",
);
assert(
  civs.civilizations.every((c) => DEFS[c.starter_commander].nation === c.id),
  "starter commander nation matches civ",
);

// combat math: identical armies, attacker has commander with maxed attack skill
const army = { infantry_t1: 300, archer_t1: 150 };
const base = troopPower(army);
const caesar = { commanderId: "julius_caesar", skills: [5, 1, 1] };
const aMod = mod(caesar, "attack");
assert(Math.abs(aMod - 0.4) < 1e-9, `caesar attack mod at skill 5 = ${aMod} (0.4)`);

// defense mod reduces attacker effectiveness vs that defender (cap 50%)
const richard = { commanderId: "richard_lionheart", skills: [1, 5, 1] };
const dMod = mod(richard, "defense");
assert(Math.abs(dMod - 0.4) < 1e-9, `richard defense mod at skill 5 = ${dMod} (0.4)`);
const dDefMod = 1 - Math.min(0.5, dMod);
const aEff = base * (1 + 0) * dDefMod;
const aEffBuffed = base * (1 + aMod) * dDefMod;
assert(aEffBuffed > aEff, "attacker with attack commander beats same attacker without");
const plainEff = base;
assert(aEff < plainEff, "defender defense skill reduces attacker effectiveness");

// xp curve
assert(xpForLevel(1) === 1200, "xpForLevel(1) = 1200");
let level = 1, xp = 0;
xp += 3 * CONSTANTS.tome_xp; // 1500
while (level < CONSTANTS.max_level && xp >= xpForLevel(level)) { xp -= xpForLevel(level); level++; }
assert(level === 2 && xp === 300, `3 tomes from level 1 → level 2 with 300 xp (got level=${level}, xp=${xp})`);

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE COMMANDER CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
