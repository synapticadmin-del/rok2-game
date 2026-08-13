/**
 * Offline checks for the holy sites + Lost Temple Cycle system (P8-T4).
 * Replicates sim/holy_sites.ts pure logic against src/data/holy_sites.json
 * (pure JSON + pure formulas, no Workers runtime) and asserts the contract:
 * structure, kind tiers + hold durations, temple composition summing to
 * guard_total, hold durations for custody vs kingship, capture gain curve,
 * and the buff hierarchy (temple overrides all).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
const here = dirname(fileURLToPath(import.meta.url));
const data = JSON.parse(readFileSync(join(here, "../src/data/holy_sites.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const H = data;

console.log("=== P8-T4 holy sites offline checks (pure JSON + formulas) ===");

// 1. structure
assert(H.version === 1, "holy_sites version 1");
assert(Array.isArray(H.sites), "sites array present");
assert(H.sites.length === 12, "12 holy sites total (sanctum x6 / altar x4 / shrine x2)");
assert(H.sites.filter((s) => s.kind === "sanctum").length === 6, "sanctum: exactly 6 sites");
assert(H.sites.filter((s) => s.kind === "altar").length === 4, "altar: exactly 4 sites");
assert(H.sites.filter((s) => s.kind === "shrine").length === 2, "shrine: exactly 2 sites");
assert(H.temple.id === "lost_temple", "temple id is lost_temple");
assert(!H.sites.some((s) => s.id === "lost_temple"), "temple is NOT a regular site entry (dedicated temple config)");
// 2. kinds config: guard tiers T1/T2/T3 with counts, hold = 4h
for (const [kind, cfg] of Object.entries(H.kinds)) {
  assert(cfg.guard_tier >= 1 && cfg.guard_tier <= 3 && cfg.guard_count > 0, `${kind}: guard tier T${cfg.guard_tier}, count ${cfg.guard_count}`);
  assert(cfg.hold_duration_ms === 4 * 3_600_000, `${kind}: hold duration is 4 hours`);
  assert(typeof H.buffs[kind] === "object" && Object.keys(H.buffs[kind]).length > 0, `${kind}: buff defined`);
}
// 3. temple: composition across tiers sums to guard_total
const totalFromShares = H.temple.guard_tiers.reduce(
  (sum, gt) => sum + gt.tier * gt.share,
  0,
);
void totalFromShares;
let compSum = 0;
for (const branch of ["infantry", "archer", "cavalry", "siege"]) {
  for (const gt of H.temple.guard_tiers) {
    compSum += Math.round((H.temple.guard_total * gt.share) / 4);
  }
}
assert(H.temple.guard_tiers.length > 0 && H.temple.guard_total > 10_000, `temple guard_total is substantial (${H.temple.guard_total})`);
assert(Math.abs(compSum - H.temple.guard_total) <= H.temple.guard_tiers.length * 4, "temple per-tier composition rounds to guard_total (±rounding)");
assert(H.temple.hold_for_king_ms === 8 * 3_600_000, "temple hold_for_king is 8 hours");
assert(H.temple.wounded_dead_share === 0.5, "temple wounded_dead_share is 50%");
assert(typeof H.temple.unlock_day === "number" && H.temple.unlock_day >= 1, "temple unlock_day numeric >= 1");
// 4. constants
const c = H.constants;
assert(c.hold_duration_ms === 4 * 3_600_000, "constants hold_duration_ms = 4h");
assert(c.contest_cycle_days === 3, "constants contest cycle = 3 days");
assert(c.capture_gain_base > 0 && c.capture_gain_power_div > 0, "capture gain constants positive");
// 5. capture gain curve: saturates at 100, bigger power = bigger gain
function siteCaptureGain(remainingPower) {
  return Math.min(100, c.capture_gain_base + Math.floor(remainingPower / c.capture_gain_power_div));
}
assert(siteCaptureGain(0) === Math.min(100, c.capture_gain_base), "zero power yields base gain only");
assert(siteCaptureGain(c.capture_gain_power_div) > siteCaptureGain(0), "positive power yields more than base");
assert(siteCaptureGain(1_000_000_000) === 100, "huge power saturates at 100");
// 6. buff hierarchy: temple overrides everything; kinds do not stack
function bestHeldSiteBuff(heldKinds) {
  const hierarchy = ["temple", "shrine", "altar", "sanctum"];
  for (const kind of hierarchy) {
    if (heldKinds.has(kind) && H.buffs[kind]) return H.buffs[kind];
  }
  return {};
}
const buffAll = bestHeldSiteBuff(new Set(["sanctum", "altar", "shrine"]));
assert(buffAll === H.buffs.shrine, "holding sanctum+altar+shrine yields shrine buff only (no stacking)");
const buffTemple = bestHeldSiteBuff(new Set(["sanctum", "altar", "shrine", "temple"]));
assert(buffTemple === H.buffs.temple, "holding temple overrides all other kinds");
const none = bestHeldSiteBuff(new Set([]));
assert(Object.keys(none).length === 0, "no held sites yields no buff");
// 7. site pos + id uniqueness
const ids = H.sites.map((s) => s.id);
assert(new Set(ids).size === ids.length, "all site ids unique");
for (const s of H.sites) {
  assert(Array.isArray(s.pos) && s.pos.length === 2 && Number.isFinite(s.pos[0]) && Number.isFinite(s.pos[1]), `${s.id}: valid [x,y] pos`);
}
assert(Array.isArray(H.temple.pos) && H.temple.pos.length === 2 && Number.isFinite(H.temple.pos[0]) && Number.isFinite(H.temple.pos[1]), "temple.pos valid [x,y]");
// 8. kinds guard tiers ascending with site strength: shrine > altar > sanctum
const kindPower = (k) => (H.kinds[k].guard_tier * H.kinds[k].guard_count);
assert(kindPower("shrine") > kindPower("altar") && kindPower("altar") > kindPower("sanctum"), "guard strength scales sanctum < altar < shrine");

console.log("");
console.log(`${failed === 0 ? "ALL PASSED" : `${failed} FAILED`} — holy sites contract verified`);
process.exit(failed === 0 ? 0 : 1);
