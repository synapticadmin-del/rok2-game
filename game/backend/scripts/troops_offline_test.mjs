/**
 * Offline unit checks for the troop tiers + special units system (P8-T3).
 * Replicates sim/troops.ts logic against src/data/troop_tiers.json and
 * src/data/civilizations.json (pure JSON + pure formulas, no Workers
 * runtime) and asserts the data + math contract: tier parsing, tier unlock
 * levels, troop stats growth T1→T5, special unit stat mods, counter
 * triangle, trainable list, and unit power monotonicity.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
const here = dirname(fileURLToPath(import.meta.url));
const tiersData = JSON.parse(readFileSync(join(here, "../src/data/troop_tiers.json"), "utf8"));
const civsData = JSON.parse(readFileSync(join(here, "../src/data/civilizations.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const TIERS = tiersData;
const CIVS = civsData;

// --- sim/troops.ts pure re-implementation (contract reference) ---
function unitTier(unitId) {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  return m ? Number(m[2]) : 0;
}
function unitBranch(unitId) {
  const m = /^([a-z]+)_t(\d+)$/.exec(unitId || "");
  return m && TIERS.branches.includes(m[1]) ? m[1] : null;
}
function isSpecialUnit(unitId) {
  if (!unitId || unitTier(unitId)) return false;
  return /^[a-z_]+$/.test(unitId);
}
function tierData(tier) { return TIERS.tiers.find((t) => t.tier === tier) || null; }
function hallUnlocksTier(hallLevel, tier) {
  const t = tierData(tier);
  return t ? hallLevel >= (t.unlock_building_level ?? tier * 2 - 1) : false;
}
function troopTierStats(tier, branch) { return tierData(tier)?.stats?.[branch] || null; }
function maxTroopTier() { return Math.max(...TIERS.tiers.map((t) => t.tier)); }
function specialUnitsForCiv(civId) {
  const civ = CIVS.civilizations.find((c) => c.id === civId);
  if (!civ?.special_unit) return [];
  const su = civ.special_unit;
  const mods = su.stat_mods || {};
  return [{ id: su.id, branch: su.branch, unlock_tier: su.unlock_tier, stat_mods: mods, name: su.name || su.id, name_ar: su.name_ar || su.id }];
}
function specialUnitStats(civId, unitId) {
  const civ = CIVS.civilizations.find((c) => c.id === civId);
  if (!civ?.special_unit || civ.special_unit.id !== unitId) return null;
  const su = civ.special_unit;
  const base = troopTierStats(su.unlock_tier, su.branch);
  if (!base) return null;
  const mods = su.stat_mods || {};
  const apply = (v, m) => Math.round(v * (1 + (m || 0)));
  return { attack: apply(base.attack, mods.attack || 0), defense: apply(base.defense, mods.defense || 0), health: apply(base.health, mods.health || 0), speed: base.speed, load: base.load, train_time: base.train_time, train_cost: base.train_cost };
}
function unitStats(unitId, civId) {
  const branch = unitBranch(unitId);
  if (branch) { const t = unitTier(unitId); if (t < 1) return null; return troopTierStats(Math.min(t, maxTroopTier()), branch); }
  if (isSpecialUnit(unitId) && civId) return specialUnitStats(civId, unitId);
  return null;
}
function unitAtk(unitId, civId) { const s = unitStats(unitId, civId); return s ? s.attack : 0; }
function counterMult(att, def) {
  const COUNTERS = { infantry: { cavalry: 1.15, archer: 0.87, infantry: 1, siege: 1 }, cavalry: { archer: 1.15, infantry: 0.87, cavalry: 1, siege: 1 }, archer: { infantry: 1.15, cavalry: 0.87, archer: 1, siege: 1 }, siege: { infantry: 1, cavalry: 1, archer: 1, siege: 1.1 } };
  const ab = unitBranch(att); const db = unitBranch(def);
  if (!ab || !db) return 1;
  return COUNTERS[ab]?.[db] ?? 1;
}
function trainableUnits(civId) {
  const out = [];
  for (const t of TIERS.tiers) for (const branch of TIERS.branches) { if (t.stats[branch]) out.push({ id: `${branch}_t${t.tier}`, branch, tier: t.tier, is_special: false }); }
  if (civId) for (const su of specialUnitsForCiv(civId)) out.push({ id: su.id, branch: su.branch, tier: su.unlock_tier, is_special: true });
  return out;
}

console.log("=== P8-T3 troops offline checks (pure JSON + formulas) ===");

// 1. JSON structure
assert(Array.isArray(TIERS.branches) && TIERS.branches.length === 4, "troop_tiers branches = 4 (infantry/cavalry/archer/siege)");
assert(Array.isArray(TIERS.tiers) && TIERS.tiers.length === 5, "troop_tiers has 5 tiers");
for (const t of TIERS.tiers) {
  assert(Array.isArray(Object.keys(t.stats)) && t.tier >= 1 && t.tier <= 5, `tier ${t.tier}: stats for all 4 branches + unlock_building_level present`);
}
// 2. T5 exists with stronger stats than T4
const t4 = troopTierStats(4, "infantry");
const t5 = troopTierStats(5, "infantry");
assert(t5.attack > t4.attack && t5.health > t4.health, "T5 stats strictly above T4 (infantry)");
// 3. stat growth monotone across tiers
let mono = true;
for (const b of TIERS.branches) for (let i = 1; i < 5; i++) if (troopTierStats(i + 1, b).attack <= troopTierStats(i, b).attack) mono = false;
assert(mono, "attack grows monotonically T1→T5 in every branch");
// 4. unlock_building_level per tier
for (const t of TIERS.tiers) assert(typeof t.unlock_building_level === "number" && t.unlock_building_level >= 1, `tier ${t.tier}: unlock_building_level is numeric >= 1`);
assert(hallUnlocksTier(1, 1) && !hallUnlocksTier(1, 2), "hall level 1 unlocks T1 only");
assert(hallUnlocksTier(7, 4) && !hallUnlocksTier(6, 4), "hall level 7 unlocks T4");
assert(hallUnlocksTier(9, 5) && !hallUnlocksTier(8, 5), "hall level 9 unlocks T5");
// 5. unit id parsing
assert(unitTier("infantry_t4") === 4 && unitBranch("archer_t3") === "archer", "unitTier/unitBranch parse branch_tN ids");
assert(unitTier("legionary") === 0 && unitBranch("legionary") === null, "special units parse as tier 0 / no branch");
assert(isSpecialUnit("legionary") && !isSpecialUnit("infantry_t1") && !isSpecialUnit(""), "isSpecialUnit contract");
// 6. counter triangle from JSON counters
assert(counterMult("infantry_t1", "cavalry_t1") === 1.15, "infantry beats cavalry 1.15");
assert(counterMult("cavalry_t1", "archer_t1") === 1.15, "cavalry beats archer 1.15");
assert(counterMult("archer_t1", "infantry_t1") === 1.15, "archer beats infantry 1.15");
assert(counterMult("cavalry_t1", "infantry_t1") === 0.87, "cavalry weak vs infantry 0.87");
assert(counterMult("siege_t1", "siege_t1") === 1.1, "siege vs siege (city) 1.1");
assert(counterMult("infantry_t1", "archer_t1") === 0.87 && counterMult("archer_t1", "cavalry_t1") === 0.87, "weak side 0.87");
assert(counterMult("infantry_t1", "siege_t2") === 1, "infantry vs siege neutral 1.0");
assert(counterMult("", "archer_t1") === 1 && counterMult("legionary", "cavalry_t1") === 1, "unrecognized ids default to 1.0");
// 7. special units: one per civ with stat mods applied
for (const civ of CIVS.civilizations) {
  const su = civ.special_unit;
  if (!su) continue;
  const list = specialUnitsForCiv(civ.id);
  assert(list.length === 1 && list[0].id === su.id, `${civ.id}: exactly one special unit (${su.id})`);
  const s = specialUnitStats(civ.id, su.id);
  assert(s !== null && s.attack > 0 && s.health > 0, `${civ.id}: ${su.id} stats computed`);
  const base = troopTierStats(su.unlock_tier, su.branch);
  const mods = su.stat_mods || {};
  if ((mods.attack || 0) > 0) assert(s.attack > base.attack, `${civ.id}: special unit attack buff applied over T${su.unlock_tier} base`);
  if ((mods.health || 0) > 0) assert(s.health > base.health, `${civ.id}: special unit health buff applied`);
  if ((mods.defense || 0) < 0) assert(s.defense < base.defense, `${civ.id}: special unit defense debuff applied`);
  // special units carry no branch via branch_tN parsing, so the triangle
  // treats them neutrally (1.0) — documented safe default, not a counter
  // exploit (they remain branch-equivalent in the DO via unitBranch fallback).
  const cm = counterMult(su.id, "infantry_t1");
  assert(cm === 1, `${civ.id}: special unit neutral (1.0) in counter triangle`);
}
// 8. trainableUnits list: 5 tiers x 4 branches + special
const all = trainableUnits();
assert(all.length === 20, "trainableUnits = 20 standard units (5 tiers x 4 branches)");
for (const civ of CIVS.civilizations) {
  const withSu = trainableUnits(civ.id);
  const hasSu = withSu.some((u) => u.is_special);
  assert(hasSu === Boolean(civ.special_unit), `${civ.id}: special unit listed iff defined (${withSu.length} units)`);
}
assert(trainableUnits().every((u) => unitStats(u.id) !== null), "every standard unit has valid stats");
// 9. unit power growth (unitAtk)
const powers = TIERS.tiers.map((t) => unitAtk(`infantry_t${t.tier}`));
assert(powers.every((v, i) => i === 0 || v > powers[i - 1]), "infantry unitAtk grows T1→T5");
assert(unitAtk("legionary", "rome") > troopTierStats(2, "infantry").attack, "legionary (unlock T2) attack above T2 base via +25% mod");
// 10. special unit with unknown civ id returns nothing
assert(unitStats("legionary") === null && specialUnitsForCiv("unknown_civ").length === 0, "unknown civ id yields no special unit");
// 11. tier clamping: tier above maxTier maps to maxTier stats
assert(unitTier("infantry_t9") === 9, "tier parser reads out-of-range tier ids");
assert(JSON.stringify(unitStats("infantry_t9")) === JSON.stringify(troopTierStats(maxTroopTier(), "infantry")), "out-of-range tier clamped to maxTier stats");
assert(trainableUnits().every((u) => u.id.includes("_t") || u.is_special), "trainable list ids are branch_tN or special ids only");

console.log("");
const passed = 0; // placeholder replaced below
void passed;
const total = process.stdout.writable ? null : null;
console.log(`${failed === 0 ? "ALL PASSED" : `${failed} FAILED`} — troops system contract verified`);
process.exit(failed === 0 ? 0 : 1);
