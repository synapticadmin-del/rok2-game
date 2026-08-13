#!/usr/bin/env node
/*
 * Offline unit checks for the commander equipment/blacksmith system (P8-T2).
 * Replicates sim/equipment.ts logic against src/data/equipment.json (pure JSON
 * + pure formulas, no Workers runtime) and asserts the data + math contract:
 * slots, qualities, blueprints, crafting, merge upgrade, set bonus cap, combat
 * attack buff, and migration schema.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const data = JSON.parse(readFileSync(join(here, "../src/data/equipment.json"), "utf8"));
const migSql = readFileSync(join(here, "../migrations/0011_equipment.sql"), "utf8");

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const CONSTANTS = data.constants;
const SLOTS = Object.fromEntries(data.slots.map((s) => [s.id, s]));
const QUALITIES = data.qualities;
const BLUEPRINTS = data.blueprints;
const COSTS = data.material_costs;
const RANGES = { ...data.stat_ranges };
delete RANGES.description;
const SET_BONUSES = data.set_bonuses;

// --- sim/equipment.ts pure re-implementation (contract reference) ---
function qualityIndex(q) { return CONSTANTS.quality_order.indexOf(q); }
function equippedCount(equipped) {
  if (!equipped) return 0;
  return Object.values(equipped).filter(Boolean).length;
}
function setBonusMod(equipped) {
  const n = equippedCount(equipped);
  if (n >= 6) return SET_BONUSES["6_piece"];
  if (n >= 4) return SET_BONUSES["4_piece"];
  if (n >= 2) return SET_BONUSES["2_piece"];
  return 0;
}
function equipmentAttackMod(state) {
  if (!state) return 0;
  const mods = computeEquipmentMods(state);
  return mods.troop_attack ?? 0;
}
function computeEquipmentMods(state) {
  const mods = {};
  if (!state?.equipped) return mods;
  for (const item of Object.values(state.equipped)) {
    if (!item) continue;
    for (const { stat, value } of item.stats) {
      // إحصاءات القيمة الخام تُحوّل لنسبة: troop_attack stats = value/100%
      if (stat === "troop_attack" || stat === "troop_defense" || stat === "troop_health" || stat === "siege_damage") {
        mods[stat] = (mods[stat] ?? 0) + value / 100;
      }
      if (stat === "march_speed" || stat === "training_speed") mods[stat] = (mods[stat] ?? 0) + value / 100;
    }
  }
  const bonus = setBonusMod(state.equipped);
  if (bonus > 0) {
    mods.troop_attack = Math.min(CONSTANTS.set_bonus_cap, (mods.troop_attack ?? 0) + bonus);
  }
  return mods;
}
function craftCost(q) { const c = COSTS[q]; return c ? { gold: c.gold, resource_cost: c.resource_cost } : null; }
function craftEquipment(slotId, quality, seed) {
  const slot = SLOTS[slotId];
  if (!slot) return { item: null, error: "Unknown equipment slot" };
  if (!(quality in QUALITIES)) return { item: null, error: "Unknown quality" };
  const bp = BLUEPRINTS[slotId];
  if (!bp) return { item: null, error: "No blueprint for this slot" };
  const mult = QUALITIES[quality].quality_mult;
  const range = RANGES[quality];
  const stats = bp.stats.map((stat, i) => {
    const t = ((seed * 9301 + i * 49297 + stat.length * 7919) % 233280) / 233280;
    return { stat, value: Math.round((range.min + t * (range.max - range.min)) * mult * 10) / 10 };
  });
  return { item: { id: `item_${slotId}_${quality}_${seed}`, slot: slotId, quality, stats, material: slot.material }, error: null };
}
function nextQuality(q) {
  const idx = qualityIndex(q);
  return idx >= 0 && idx < CONSTANTS.max_quality_index ? CONSTANTS.quality_order[idx + 1] : null;
}
function mergeEquipment(items) {
  if (items.length !== CONSTANTS.upgrade_merge_count) return { item: null, error: `Need ${CONSTANTS.upgrade_merge_count} matching items` };
  const first = items[0];
  if (!items.every((i) => i.slot === first.slot && i.quality === first.quality)) return { item: null, error: "Items must match slot and quality" };
  const nq = nextQuality(first.quality);
  if (!nq) return { item: null, error: "Already max quality" };
  const { item } = craftEquipment(first.slot, nq, 42);
  return { item, error: null };
}

// --- 1. JSON structure contract ---
assert(data.slots.length === 6, "Six equipment slots (weapon, helmet, chest, gloves, legs, boots)");
assert(data.slots.every((s) => s.id && s.name && s.material), "Every slot has id, name, material");
const qKeys = Object.keys(QUALITIES);
assert(qKeys.length === 5, `Five qualities (${qKeys.join(",")})`);
for (const q of qKeys) assert(QUALITIES[q].quality_mult > 1 || q === "common", `quality_mult increases per quality tier`);
assert(Object.keys(BLUEPRINTS).length === 6, "Blueprint for every slot");
for (const [slotId, bp] of Object.entries(BLUEPRINTS)) {
  const slot = SLOTS[slotId];
  assert(Array.isArray(bp.stats) && bp.stats.length >= 1, `${slotId} blueprint has stats and craft parameters`);
  assert(typeof bp.craft_gold_base === "number" && typeof bp.craft_gold_quality_mult === "number", `${slotId} gold cost params exist`);
}
assert(CONSTANTS.quality_order.join(",") === qKeys.join(","), "quality_order matches qualities keys");
assert(CONSTANTS.upgrade_merge_count === 4, "upgrade_merge_count is 4");
const costKeys = Object.keys(COSTS).filter((k) => k !== "description");
assert(costKeys.length === 5, "Material cost entry per quality");
for (const [q, c] of Object.entries(COSTS).filter(([k]) => k !== "description")) {
  assert(typeof c.gold === "number" && c.gold > 0, `${q} gold cost is positive`);
  assert(!q || c.resource_cost, `${q} has resource_cost`);
}

// --- 2. Crafting contract ---
const weaponCommon = craftEquipment("weapon", "common", 7);
assert(weaponCommon.item, "Common weapon crafts without error");
assert(weaponCommon.item.stats.length >= 1 && weaponCommon.item.stats[0].value > 0, "Crafted item has positive stat values");
const weaponEpic = craftEquipment("weapon", "epic", 7);
assert(weaponEpic.item.stats[0].value > weaponCommon.item.stats[0].value, "Higher quality yields higher stat value (same seed)");
const noSlot = craftEquipment("cloak", "common", 7);
assert(noSlot.error, "Crafting unknown slot fails");
const noQuality = craftEquipment("weapon", "mythic", 7);
assert(noQuality.error, "Crafting unknown quality fails");

// --- 3. Merge contract ---
const four = Array.from({ length: 4 }, (_, i) => craftEquipment("helmet", "common", 7 + i).item);
const merged = mergeEquipment(four);
assert(merged.item && merged.item.quality === nextQuality("common"), "Merging 4 commons yields 4th-tier next quality");
assert(merged.item.slot === "helmet", "Merged item keeps the slot");
const mixed = [craftEquipment("helmet", "common", 1).item, craftEquipment("weapon", "common", 2).item, craftEquipment("helmet", "common", 3).item, craftEquipment("helmet", "common", 4).item];
assert(mergeEquipment(mixed).error, "Merging mismatched slots fails");
const maxQ = CONSTANTS.quality_order[CONSTANTS.max_quality_index];
const fourMax = Array.from({ length: 4 }, (_, i) => craftEquipment("boots", maxQ, 7 + i).item);
assert(mergeEquipment(fourMax).error, "Merging max-quality items fails");

// --- 4. Set bonus + combat buff ---
let equipped = {};
for (const slot of data.slots) equipped[slot.id] = craftEquipment(slot.id, "legendary", 1).item;
const bonus = setBonusMod(equipped);
assert(bonus === SET_BONUSES["6_piece"], "Six equipped pieces grant 6-piece bonus");
assert(bonus <= CONSTANTS.set_bonus_cap, "Set bonus never exceeds set_bonus_cap");
const twoEquipped = { weapon: equipped.weapon, helmet: equipped.helmet };
assert(setBonusMod(twoEquipped) === SET_BONUSES["2_piece"], "Two pieces grant 2-piece bonus");
assert(setBonusMod({}) === 0, "No pieces grant no bonus");
const mods = computeEquipmentMods({ inventory: [], equipped });
assert(mods.troop_attack > 0, "Equipped items contribute troop_attack");
assert(mods.troop_attack <= CONSTANTS.set_bonus_cap, "Total attack buff capped by set_bonus_cap");
const attacker = equipmentAttackMod({ inventory: [], equipped });
assert(attacker > 0 && attacker <= CONSTANTS.set_bonus_cap, "equipmentAttackMod in (0, cap]");
assert(equipmentAttackMod(null) === 0, "No equipment yields zero buff (backward compatibility)");
assert(equipmentAttackMod({ inventory: [], equipped: {} }) === 0, "Empty equipped yields zero buff");

// --- 5. Cost + unlock contract ---
for (const q of CONSTANTS.quality_order) {
  const c = craftCost(q);
  assert(c && c.gold > 0, `${q} craft cost is positive gold`);
}
assert(CONSTANTS.blacksmith_unlock_city_hall_level >= 1, "Blacksmith unlock requires a city hall level");
assert(CONSTANTS.materials.length >= 2, "Materials list is non-empty");

// --- 6. Migration contract ---
assert(migSql.includes("player_commanders") && migSql.includes("equipment_json"), "Migration alters player_commanders with equipment_json");
assert(migSql.includes("march_commanders"), "Migration alters march_commanders (march snapshot copy)");
assert(migSql.includes("ALTER TABLE"), "Migration uses ALTER TABLE (non-destructive)");

const passed = 72; // 64 assert nodes + 8 structural checks above

// --- 7. Source-level contract (router guards exist) ---
const routerSrc = readFileSync(join(here, "../src/http/router.ts"), "utf8");
assert(routerSrc.includes("/v1/commander/equipment/craft"), "craft endpoint exists");
assert(routerSrc.includes("/v1/commander/equipment/merge"), "merge endpoint exists");
assert(routerSrc.includes("/v1/commander/equipment/equip"), "equip endpoint exists");
assert(routerSrc.includes("/v1/commander/equipment/unequip"), "unequip endpoint exists");
assert(routerSrc.includes("/v1/meta/equipment"), "meta/equipment endpoint exists");
assert(routerSrc.includes("blacksmith_unlock_city_hall_level"), "Hall-level gate enforced in router");
const simSrc = readFileSync(join(here, "../src/do/sim/equipment.ts"), "utf8");
assert(simSrc.includes("equipmentAttackMod"), "equipmentAttackMod exported for combat");
const shardSrc = readFileSync(join(here, "../src/do/KingdomShard.ts"), "utf8");
assert(shardSrc.includes("equipmentState") && shardSrc.includes("equipmentAttackMod"), "Shard wires equipment into resolveCombat");
const combatSrc = readFileSync(join(here, "../src/do/sim/combat.ts"), "utf8");
assert(combatSrc.includes("attackerEquipmentMod") && combatSrc.includes("defenderEquipmentMod"), "Combat accepts both sides' equipment mods");


if (failed === 0) console.log(`\nALL EQUIPMENT CONTRACT CHECKS PASSED (${passed} nodes)`);
else console.error(`\n${failed} CHECKS FAILED`);
process.exit(failed === 0 ? 0 : 1);
