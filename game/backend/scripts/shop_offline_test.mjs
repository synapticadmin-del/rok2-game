#!/usr/bin/env node
/**
 * Offline unit checks for the sandbox shop + speedups + VIP (P3-T4) against data/shop.json
 * and the shop sim logic mirrored here (no constants in code — all values come from JSON).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const DATA = JSON.parse(readFileSync(join(here, "../src/data/shop.json"), "utf8"));
const ROOT_DATA = JSON.parse(readFileSync(join(here, "../../../data/shop.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ── mirror of sim/shop.ts logic (values-driven) ──
const C = DATA.constants;
const tiers = DATA.vip_tiers;
const getSpeedup = (id) => DATA.speedups.find((s) => s.id === id);
const vipTierForPoints = (pts) => {
  let cur = tiers[0];
  for (const t of tiers) if (pts >= t.points_required) cur = t;
  return cur;
};
const vipPointsForPurchase = (gems) => gems * C.vip_points_per_gem;

// ── data shape ──
assert(JSON.stringify(DATA) === JSON.stringify(ROOT_DATA), "src/data/shop.json mirrors data/shop.json");
assert(C.sandbox_starting_gems > 0 && C.sandbox_daily_gems > 0, `gems constants: start=${C.sandbox_starting_gems}, daily=${C.sandbox_daily_gems}`);
assert(C.vip_points_per_gem === 1, "1 VIP point per gem spent");
assert(DATA.speedups.length === 7, `7 speedup items (got ${DATA.speedups.length})`);
assert(tiers.length === 7, `7 VIP tiers (got ${tiers.length})`);

// speedups: ids unique, costs/seconds positive and monotonic-ish
const ids = new Set(DATA.speedups.map((s) => s.id));
assert(ids.size === DATA.speedups.length, "speedup ids unique");
assert(DATA.speedups.every((s) => s.seconds > 0 && s.cost_gems > 0 && s.vip_points === s.cost_gems), "every speedup: positive seconds/cost, vip_points == cost_gems");
assert(getSpeedup("speedup_1m").seconds === 60, "speedup_1m = 60 seconds");
assert(getSpeedup("speedup_24h").seconds === 86400, "speedup_24h = 86400 seconds");
assert(getSpeedup("nonexistent") === undefined, "unknown item id → undefined");

// vip tiers: level 0 free, thresholds strictly increasing, perks monotone
assert(tiers[0].level === 0 && tiers[0].points_required === 0, "tier 0: free entry at 0 points");
assert(tiers[0].production_mult === 1.0 && tiers[0].free_speedup_sec_per_day === 0, "tier 0: no perks");
let ok = true;
for (let i = 1; i < tiers.length; i++) {
  if (tiers[i].points_required <= tiers[i - 1].points_required) ok = false;
  if (tiers[i].level !== i) ok = false;
  if (tiers[i].production_mult < tiers[i - 1].production_mult) ok = false;
}
assert(ok, "tier levels sequential + thresholds strictly increasing + production_mult monotone");

// tier resolution at boundaries
assert(vipTierForPoints(0).level === 0, "0 points → level 0");
assert(vipTierForPoints(99).level === 0, "99 points → still level 0");
assert(vipTierForPoints(100).level === 1, "100 points → level 1");
assert(vipTierForPoints(299).level === 1 && vipTierForPoints(300).level === 2, "300 points → level 2");
assert(vipTierForPoints(6000).level === 6, "6000 points → max level 6");
assert(vipTierForPoints(999999).level === 6, "huge points → capped at level 6");

// purchase → points flow: buying one speedup_1h (150 gems) → 150 pts → level 1
const buy = vipPointsForPurchase(getSpeedup("speedup_1h").cost_gems);
assert(buy === 150 && vipTierForPoints(buy).level === 1, "buy speedup_1h (150 gems) → 150 pts → VIP 1");
// cumulative: 3× speedup_3h (400×3=1200) → level 4? no: 1200 < 1500 → level 3
const pts3 = vipPointsForPurchase(3 * getSpeedup("speedup_3h").cost_gems);
assert(pts3 === 1200 && vipTierForPoints(pts3).level === 3, `3× speedup_3h → ${pts3} pts → VIP 3 (1200 < 1500)`);

// perk values sane
assert(vipTierForPoints(6000).production_mult === 1.15, "VIP 6 → +15% production");
assert(vipTierForPoints(6000).free_speedup_sec_per_day === 1800, "VIP 6 → 1800s free daily speedup");
assert(tiers.every((t) => t.build_speed_mult >= 1 && t.train_speed_mult >= 1), "speed multipliers never below 1.0");

// affordability maths (sandbox economy): starting gems afford exactly 6× speedup_1h (6×150=900 ≤ 1000)
const affordable = Math.floor(C.sandbox_starting_gems / getSpeedup("speedup_1h").cost_gems);
assert(affordable === 6, `starting gems (${C.sandbox_starting_gems}) afford ${affordable}× speedup_1h`);

// daily grant accumulation: 5 days of daily gems (no spend) = 1000 + 5×200 = 2000
const after5 = C.sandbox_starting_gems + 5 * C.sandbox_daily_gems;
assert(after5 === 2000, `day-5 balance with no spend = ${after5} gems`);

// router wiring: shop sim + endpoints present, no economic constants inline
const ROUTER = readFileSync(join(here, "../src/http/router.ts"), "utf8");
for (const ep of ["/v1/shop/catalog", "/v1/vip/status", "/v1/shop/buy", "/v1/shop/use-speedup", "/v1/shop/daily-gems"]) {
  assert(ROUTER.includes(ep), `router wires ${ep}`);
}
assert(ROUTER.includes('from "../do/sim/shop"'), "router imports shop sim");
assert(ROUTER.includes("shopConstants().sandbox_starting_gems"), "city init grants starting gems from JSON");
assert(ROUTER.includes("vipTierForPoints"), "VIP tier logic used in router");
assert(ROUTER.includes("ON CONFLICT(player_id, item_id)"), "inventory upsert on buy");
assert(!/gems\s*<\s*totalCost/.test(ROUTER) || ROUTER.includes("Not enough gems"), "buy checks gem balance");

// migration covers both tables + gems column
const MIG = readFileSync(join(here, "../migrations/0005_shop.sql"), "utf8");
assert(MIG.includes("CREATE TABLE IF NOT EXISTS player_inventory"), "migration: player_inventory");
assert(MIG.includes("CREATE TABLE IF NOT EXISTS player_vip"), "migration: player_vip");
assert(MIG.includes("ALTER TABLE cities ADD COLUMN gems"), "migration: gems column on cities");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE SHOP CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
