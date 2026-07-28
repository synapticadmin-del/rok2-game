#!/usr/bin/env node
/**
 * Offline unit checks for Battle Pass (P4-T1) against data/battlepass.json
 * and the battlepass sim logic mirrored here (no constants in code — all from JSON).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const DATA = JSON.parse(readFileSync(join(here, "../src/data/battlepass.json"), "utf8"));
const ROOT_DATA = JSON.parse(readFileSync(join(here, "../../../data/battlepass.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ── mirror of sim/battlepass.ts (values-driven) ──
const C = DATA.constants;
const levels = DATA.levels;
const bpXpFor = (a) => DATA.xp_sources[a] || 0;
const bpLevelForXp = (xp) => Math.min(Math.floor(xp / C.xp_per_level), C.max_level);
const bpXpRequiredFor = (l) => l * C.xp_per_level;
const bpProgressInLevel = (xp) => {
  const l = bpLevelForXp(xp);
  if (l >= C.max_level) return { inLevel: C.xp_per_level, perLevel: C.xp_per_level, atMax: true };
  return { inLevel: xp - bpXpRequiredFor(l), perLevel: C.xp_per_level, atMax: false };
};
const bpRewardFor = (l, t) => { const x = levels.find((y) => y.level === l); return x ? x[t] : null; };
const bpClaimableLevels = (pl) => Array.from({ length: Math.min(pl, C.max_level) }, (_, i) => i + 1);

// ── data shape ──
assert(JSON.stringify(DATA) === JSON.stringify(ROOT_DATA), "src/data/battlepass.json mirrors data/battlepass.json");
assert(DATA.season_id === "alpha_s1", `season id alpha_s1 (got ${DATA.season_id})`);
assert(C.premium_cost_gems > 0 && C.xp_per_level > 0 && C.max_level === 20, `constants: premium ${C.premium_cost_gems}, xp/level ${C.xp_per_level}, max ${C.max_level}`);
assert(levels.length === C.max_level, `levels array == max_level (${levels.length})`);
assert(levels.every((l, i) => l.level === i + 1 && l.free && l.premium), "levels sequential 1..20 with free+premium");

// reward types valid
const validTypes = new Set(DATA.reward_types);
assert(levels.every((l) => validTypes.has(l.free.type) && validTypes.has(l.premium.type)), "every reward type ∈ reward_types");
// resources have amount; speedups have item_id + count
assert(levels.every((l) => {
  const chk = (r) => r.type === "speedup" ? (r.item_id && r.count > 0) : (r.amount > 0);
  return chk(l.free) && chk(l.premium);
}), "resources have amount; speedups have item_id+count");

// xp sources: all actions present & positive
for (const a of ["build", "train", "research", "heal", "march", "pass_attack"]) {
  assert(bpXpFor(a) > 0, `xp source: ${a} = ${bpXpFor(a)}`);
}
assert(bpXpFor("nonexistent") === 0, "unknown action → 0 xp");
assert(bpXpFor("pass_attack") > bpXpFor("build"), "pass_attack worth more than build");

// level curve (linear: xp_per_level per level, cap at max)
assert(bpLevelForXp(0) === 0, "0 xp → level 0");
assert(bpLevelForXp(99) === 0, "99 xp → level 0");
assert(bpLevelForXp(100) === 1, "100 xp → level 1");
assert(bpLevelForXp(1999) === 19 && bpLevelForXp(2000) === 20, "2000 xp → level 20 (max)");
assert(bpLevelForXp(999999) === 20, "huge xp → capped at 20");

// progress in level
assert(bpProgressInLevel(150).inLevel === 50 && bpProgressInLevel(150).atMax === false, "150 xp → 50/100 in level 1");
assert(bpProgressInLevel(2000).atMax === true, "max level → atMax");

// claimable levels
assert(JSON.stringify(bpClaimableLevels(0)) === "[]", "level 0 → nothing claimable");
assert(JSON.stringify(bpClaimableLevels(3)) === "[1,2,3]", "level 3 → claim levels 1,2,3");
assert(bpClaimableLevels(99).length === 20, "over-max → still 20 levels");

// rewards lookup
assert(bpRewardFor(1, "free").type === "food" && bpRewardFor(1, "free").amount === 2000, "L1 free = 2000 food");
assert(bpRewardFor(1, "premium").type === "gems", "L1 premium = gems");
assert(bpRewardFor(20, "premium").type === "speedup", "L20 premium = speedup");
assert(bpRewardFor(21, "free") === null, "L21 → null");
// speedup rewards reference real shop items
const SHOP = JSON.parse(readFileSync(join(here, "../src/data/shop.json"), "utf8"));
const shopIds = new Set(SHOP.speedups.map((s) => s.id));
assert(levels.every((l) => {
  const chk = (r) => r.type !== "speedup" || shopIds.has(r.item_id);
  return chk(l.free) && chk(l.premium);
}), "every speedup reward references a real shop item");

// premium economics: total premium gems cost 500; total free-gems-in-premium-track rewards
const premiumGemRewards = levels.reduce((s, l) => s + (l.premium.type === "gems" ? l.premium.amount : 0), 0);
assert(premiumGemRewards > C.premium_cost_gems, `premium track returns ${premiumGemRewards} gems > ${C.premium_cost_gems} cost (net positive)`);

// ── router wiring ──
const ROUTER = readFileSync(join(here, "../src/http/router.ts"), "utf8");
const GAMEDATA = readFileSync(join(here, "../src/lib/gameData.ts"), "utf8");
assert(ROUTER.includes('from "../do/sim/battlepass"'), "router imports battlepass sim");
for (const ep of ["/v1/battlepass", "/v1/battlepass/claim", "/v1/battlepass/unlock-premium"]) {
  assert(ROUTER.includes(ep), `router wires ${ep}`);
}
assert((ROUTER.match(/grantBpXp/g) || []).length >= 7, "grantBpXp defined + granted for all 6 actions");
for (const a of ['"build"', '"train"', '"research"', '"heal"', '"march"', '"pass_attack"']) {
  assert(ROUTER.includes(`grantBpXp(env, player.id, ${a})`), `xp granted for ${a}`);
}
assert(ROUTER.includes("battlepass_claims") && ROUTER.includes("ON CONFLICT(player_id, item_id)"), "claims + inventory upsert wired");
assert(GAMEDATA.includes("getBattlePass") && GAMEDATA.includes("battlePassSpec"), "gameData exports getBattlePass");

// ── migration coverage ──
const MIG = readFileSync(join(here, "../migrations/0007_battlepass.sql"), "utf8");
assert(MIG.includes("CREATE TABLE IF NOT EXISTS player_battlepass"), "migration: player_battlepass");
assert(MIG.includes("CREATE TABLE IF NOT EXISTS battlepass_claims"), "migration: battlepass_claims");
assert(MIG.includes("PRIMARY KEY (player_id, season_id, level, track)"), "migration: claim uniqueness per level+track");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE BATTLEPASS CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
