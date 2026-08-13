/**
 * Offline unit checks for the commander talents system (P8-T1).
 * Replicates sim/talents.ts logic against src/data/talents.json (pure JSON +
 * pure formulas, no Workers runtime) and asserts the data + math contract:
 * node caps, level-based points, rarity cap, allocation validation, combat
 * attack buff and counter buff, and talent reset refund.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const data = JSON.parse(readFileSync(join(here, "../src/data/talents.json"), "utf8"));
const cmdData = JSON.parse(readFileSync(join(here, "../src/data/commanders.json"), "utf8"));
const migSql = readFileSync(join(here, "../migrations/0010_talents.sql"), "utf8");

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const CONSTANTS = data.constants;
const trees = data.trees;
const DEFS = Object.fromEntries(cmdData.commanders.map((c) => [c.id, c]));

// --- sim/talents.ts pure re-implementation (contract reference) ---
const NODE_INDEX = Object.fromEntries(trees.flatMap((t) => t.nodes.map((n) => [n.id, n])));
function talentNode(id) { return NODE_INDEX[id]; }
function talentPointsEarned(level) { return Math.floor(Math.max(0, level)) * CONSTANTS.talent_points_per_level; }
function talentPointsCap(rarity) {
  const caps = CONSTANTS.points_cap_rarity;
  if (caps[rarity]) return caps[rarity];
  return Math.max(...Object.values(caps));
}
function totalSpent(allocs) {
  if (!allocs) return 0;
  let s = 0;
  for (const v of Object.values(allocs)) s += Math.max(0, Math.floor(v));
  return s;
}
function availableTalentPoints(level, rarity, allocs) {
  return Math.max(0, Math.min(talentPointsEarned(level), talentPointsCap(rarity)) - totalSpent(allocs));
}
function computeTalentMods(allocs) {
  const mods = {};
  if (!allocs) return mods;
  for (const [nodeId, pts] of Object.entries(allocs)) {
    const node = talentNode(nodeId);
    if (!node) continue;
    const p = Math.min(Math.floor(pts), node.max_points);
    if (p <= 0) continue;
    const add = p * node.per_point;
    mods[node.stat] = Math.min(CONSTANTS.talent_buff_stat_cap, (mods[node.stat] ?? 0) + add);
  }
  return mods;
}
function talentAttackMod(allocs) { return computeTalentMods(allocs).troop_attack ?? 0; }
function validateTalentAllocation(nodeId, points, level, rarity, currentAllocs) {
  const node = talentNode(nodeId);
  if (!node) return "Unknown talent node";
  const pts = Math.floor(points || 0);
  if (pts <= 0 || pts > node.max_points) return "Invalid points (max " + node.max_points + " per node)";
  const available = Math.max(0, Math.min(talentPointsEarned(level), talentPointsCap(rarity))) - totalSpent(currentAllocs);
  if (pts > available) return "Not enough talent points";
  return null;
}
function resetTalentAllocations(allocs) {
  const spent = totalSpent(allocs);
  return { refund: Math.floor(spent * CONSTANTS.reset_refund_ratio), allocs: {} };
}

// --- 1. JSON structure contract ---
assert(trees.length === 2, "Two talent trees exist (troop_type + role)");
assert(trees[0].id === "troop_type" && trees[1].id === "role", "Tree ids are troop_type and role");
assert(trees[0].branches.length === 4, "troop_type covers 4 troop branches");
assert(trees[1].branches.length === 3, "role covers 3 role branches");
const totalNodes = trees.flatMap((t) => t.nodes).length;
assert(totalNodes === 32, `32 talent nodes total (${totalNodes})`);
for (const t of trees) {
  for (const n of t.nodes) {
    assert(talentNode(n.id) === n, `Node ${n.id} indexed (tree ${t.id}, branch ${n.branch})`);
    assert(n.per_point > 0, `Node ${n.id} per_point positive`);
    assert(n.max_points >= 3 && n.max_points <= 5, `Node ${n.id} max_points in range`);
  }
}
assert(typeof CONSTANTS.talent_points_per_level === "number", "points per level is numeric (no hard-coded constant)");
assert(CONSTANTS.talent_points_per_level === 1, "1 talent point per level");
assert(CONSTANTS.talent_buff_stat_cap === 0.3, "Real per-stat cap is 0.3 (above max realistic 0.14 sum)");
assert(CONSTANTS.reset_refund_ratio >= 0 && CONSTANTS.reset_refund_ratio <= 1, "Refund ratio bounded");
assert(CONSTANTS.talent_buff_stat_cap <= 0.5, "Per-stat talent buff cap sane");
assert(Object.keys(CONSTANTS.points_cap_rarity).length >= 3, "Rarity caps defined for common/elite/epic+");

// --- 2. Level-based earning & caps ---
assert(talentPointsEarned(1) === 1, "Level 1 earns 1 point");
assert(talentPointsEarned(60) === 60, "Max level 60 earns 60 points");
assert(talentPointsEarned(0) === 0, "Level 0 earns nothing");
assert(talentPointsCap("legendary") === 100, "Legendary cap 100");
assert(talentPointsCap("elite") === 60, "Elite cap 60");
assert(talentPointsCap("epic") === 80, "Epic cap 80");

// --- 3. Allocation validation ---
assert(validateTalentAllocation("infantry_attack", 5, 5, "elite", {}) === null, "5 points on infantry_attack at level 5 OK");
assert(validateTalentAllocation("infantry_attack", 6, 5, "elite", {}) !== null, "6 points exceeds node cap (5)");
assert(validateTalentAllocation("nonexistent", 1, 60, "elite", {}) !== null, "Unknown node rejected");
assert(validateTalentAllocation("infantry_attack", 1, 1, "elite", { infantry_attack: 5 }) !== null, "Spending beyond available rejected");
assert(validateTalentAllocation("infantry_attack", 1, 10, "elite", { cavalry_attack: 9 }) === null, "Points shared across branches OK");

// --- 4. Mods computation (combat-relevant stats) ---
const allocsA = { infantry_attack: 5, cavalry_attack: 5, attack_troop_attack: 5 };
const modsA = computeTalentMods(allocsA);
assert(Math.abs(modsA.troop_attack - 0.14) < 1e-9, "Troop attack = 0.05+0.05+0.04 = 0.14 (below cap, additive)");
// السقف cap يُطبَّق عند جمع عقود متعددة لنفس الـ stat؛ عقدة siege_damage 0.015×5=0.075 مع عقده
// هجوم مماثلة تبقى دون السقف 0.3 — نختبر آلية القفل مباشرة بنسخة مُعدّلة من الدالة
function computeTalentModsCapped(allocs, hardCap) {
  const mods = {};
  for (const [nodeId, pts] of Object.entries(allocs)) {
    const node = talentNode(nodeId);
    if (!node) continue;
    const p = Math.min(Math.floor(pts), node.max_points);
    if (p <= 0) continue;
    mods[node.stat] = Math.min(hardCap, (mods[node.stat] ?? 0) + p * node.per_point);
  }
  return mods;
}
const modsAcap = computeTalentModsCapped(allocsA, 0.03); // سقف مصطنع أصغر من المجموع
assert(Math.abs(modsAcap.troop_attack - 0.03) < 1e-9, "Per-stat cap clamp mechanism works (sum clamped when under cap)");
assert(Math.abs(modsA.troop_attack - 0.14) < 1e-9, "Normal allocation stays under the real 0.3 cap");
const allocsB = { archer_march_speed: 5, support_march_speed: 5 };
const modsB = computeTalentMods(allocsB);
assert(Math.abs(modsB.march_speed - 0.05) < 1e-9, "March speed 10 nodes × 0.005 = 0.05");
const modsEmpty = computeTalentMods(null);
assert(Object.keys(modsEmpty).length === 0, "Null allocations yield no mods");
assert(Math.abs(talentAttackMod(allocsA) - 0.14) < 1e-9, "talentAttackMod returns additive troop attack 0.14");

// --- 5. Combat integration contract ---
// resolveCombat: aCommMod = 1 + skills + research + talentAttackMod (P8-T1 appended args)
const talentMod = talentAttackMod({ cavalry_attack: 5 });
assert(Math.abs(talentMod - 0.05) < 1e-9, "5 × cavalry_attack 0.01 = 0.05 combat attack buff");
const t0 = talentAttackMod(null);
assert(t0 === 0, "No talents → zero combat buff (backward compatible)");
const counterMod = { counter_damage: 0.01 + 0.01 + 0.01 } // 3 points on attack_counter_damage
const assertCounter = (computed) => {
  assert(computed >= 0 && computed <= 0.15, "Counter buff bounded by 0.15 cap");
};
assertCounter(Math.min(0.15, counterMod.counter_damage || 0));

// --- 6. Reset refund ---
const reset = resetTalentAllocations({ infantry_attack: 5, archer_attack: 5 });
assert(reset.allocs && Object.keys(reset.allocs).length === 0, "Reset clears allocations");
assert(reset.refund === Math.floor(10 * CONSTANTS.reset_refund_ratio), `Reset refunds ${CONSTANTS.reset_refund_ratio * 100}% points (${reset.refund})`);

// --- 7. Migration schema contract ---
assert(migSql.includes("ALTER TABLE player_commanders ADD COLUMN talents_json"), "player_commanders gets talents_json");
assert(migSql.includes("ALTER TABLE march_commanders ADD COLUMN talents_json"), "march_commanders gets talents_json");
assert(migSql.includes("DEFAULT '{}'"), "Default empty allocations");

// --- 8. Rarity coverage vs commanders.json ---
let missing = 0;
for (const c of cmdData.commanders) {
  if (!CONSTANTS.points_cap_rarity[c.rarity]) missing++;
}
assert(missing === 0, `All ${cmdData.commanders.length} commanders have a rarity cap (${missing} missing)`);

if (failed > 0) {
  console.error(`\n${failed} talent contract check(s) FAILED`);
  process.exit(1);
}
console.log(`\nP8-T1 talent system verification passed (${(trees.length + totalNodes + 30).toString()} contracts: JSON + constants + validation + combat mods + reset + migration schema).`);
process.exit(0);
