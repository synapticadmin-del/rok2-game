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
assert(data.commanders.length === 18, "roster has 18 commanders (12 P2-T1 + 6 P4-T5)");
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

// P4-T5: 6 new commanders — one per civ, each with valid 3-skill structure
const NEW_IDS = ["cmd_rome_2", "cmd_china_2", "cmd_arabia_2", "cmd_egypt_2", "cmd_vikings_2", "cmd_japan_2"];
assert(NEW_IDS.every((id) => DEFS[id]), "all 6 P4-T5 commanders present");
assert(
  NEW_IDS.every((id) => DEFS[id].nation === id.replace("cmd_", "").replace("_2", "")),
  "P4-T5 commander nation matches civ id",
);
const nations = new Set(NEW_IDS.map((id) => DEFS[id].nation));
assert(nations.size === 6, "P4-T5: 6 distinct nations covered");
assert(
  data.commanders.every((c) => c.skills.every((s) => s.effects.every((e) => e.per_level > 0 && e.per_level <= 0.2))),
  "all skill effects within sane balance range (0 < per_level <= 0.2)",
);
assert(
  data.commanders.every((c) => ["elite", "epic", "legendary"].includes(c.rarity)),
  "all rarities are valid (elite/epic/legendary)",
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

// P4-T5: combat math spot-checks for two new commanders
const saladin = { commanderId: "cmd_arabia_2", skills: [5, 1, 1] };
const salMod = mod(saladin, "attack");
assert(Math.abs(salMod - 0.4) < 1e-9, `saladin attack mod at skill 5 = ${salMod} (0.4)`);
const zhuge = { commanderId: "cmd_china_2", skills: [1, 5, 1] };
const zhugeDef = mod(zhuge, "defense");
assert(Math.abs(zhugeDef - 0.25) < 1e-9, `zhuge defense mod at skill 5 = ${zhugeDef} (0.25)`);
const zhugePassive = DEFS["cmd_china_2"].skills[2];
assert(zhugePassive.effects[0].stat === "xp_gain", "zhuge passive is xp_gain");
const newIds = NEW_IDS;
assert(
  newIds.every((id) => DEFS[id].skills.every((s) => s.max_level === 5)),
  "P4-T5: all new skills have max_level 5 (consistent with roster)",
);

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
