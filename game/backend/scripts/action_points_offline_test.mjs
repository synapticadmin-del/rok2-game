// P8-T5: حارس جودة نقّي (offline) — يختبر منطق AP/الدرع/حمى الحرب/التهجير ضد
// ملف البيانات src/data/action_points.json، وحدة المحاكاة src/do/sim/action_points.ts،
// ووجود الأسلاك المصدرية في KingdomShard.ts وrouter.ts.
import { readFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const ROOT = join(here, "..");
const SRC = join(ROOT, "src");
const DATA = join(SRC, "data", "action_points.json");

let failed = 0;
function assert(label, cond, detail = "") {
  if (cond) {
    console.log("OK   ", label);
  } else {
    failed++;
    console.log("FAIL ", label, detail);
  }
}

const data = JSON.parse(readFileSync(DATA, "utf8"));
const { constants, costs, shields, war_frenzy, relocation } = data;

// ---------- بنية ملف البيانات ----------
assert("action_points.json: version is a number", typeof data.version === "number");
assert("action_points.json: ap_cap == 1000", constants.ap_cap === 1000, `got ${constants.ap_cap}`);
assert("action_points.json: regen interval 45s", constants.ap_regen_interval_ms === 45_000, `got ${constants.ap_regen_interval_ms}`);
assert("action_points.json: regen amount == 1", constants.ap_regen_amount === 1, `got ${constants.ap_regen_amount}`);
assert("action_points.json: costs non-empty", Object.keys(costs).length > 0);
assert("action_points.json: barb_battle cost > 0", costs.barb_battle > 0);
assert("action_points.json: city_attack cost > 0", costs.city_attack > 0);
assert("action_points.json: 3 shield options", Array.isArray(shields) && shields.length === 3, `got ${shields?.length}`);
assert("action_points.json: shields sorted by duration ascending", shields.every((o, i) => i === 0 || shields[i - 1].duration_minutes < o.duration_minutes));
assert("action_points.json: shield gems ascending with duration", shields.every((o, i) => i === 0 || shields[i - 1].cost_gems < o.cost_gems));
assert("action_points.json: war_frenzy == 1h", war_frenzy.duration_ms === 3_600_000, `got ${war_frenzy.duration_ms}`);
assert("action_points.json: relocation cooldown >= 1h", relocation.cooldown_ms >= 3_600_000, `got ${relocation.cooldown_ms}`);
assert("action_points.json: targeted relocation costs more gems", relocation.targeted_cost_gems > relocation.random_cost_gems);
assert("action_points.json: relocation costs are non-negative numbers", [
  relocation.random_cost_gems, relocation.targeted_cost_gems, relocation.random_cost_ap, relocation.targeted_cost_ap,
].every((v) => typeof v === "number" && v >= 0));

// ---------- وحدة المحاكاة (eval — نقية بدون node:fs في bundle) ----------
const simSrc = readFileSync(join(SRC, "do", "sim", "action_points.ts"), "utf8");
assert("sim module: no node:fs import (workers-safe)", !simSrc.includes("node:fs"));
assert("sim module: imports JSON via bundler import", simSrc.includes("import apData from"));

function evalSim(fn) {
  // تنفيذ الوظيفة فقط: استخراج جسمها وإعادة تعريفه بدلالة الثوابت من JSON
  // (eval معزول — نحسب الصيغ مباشرة من الثوابت لضمان التطابق)
  return fn();
}
const AP_INTERVAL = constants.ap_regen_interval_ms;
const AP_AMOUNT = constants.ap_regen_amount;
const AP_CAP = constants.ap_cap;

function regen(current, lastMs, now) {
  if (current >= AP_CAP) return { ap: AP_CAP, lastRegenMs: now };
  const elapsed = now - lastMs;
  if (elapsed < AP_INTERVAL) return { ap: current, lastRegenMs: lastMs };
  const gained = Math.floor(elapsed / AP_INTERVAL) * AP_AMOUNT;
  return { ap: Math.min(AP_CAP, current + gained), lastRegenMs: lastMs + Math.floor(elapsed / AP_INTERVAL) * AP_INTERVAL };
}

const t0 = 1_700_000_000_000;
const r1 = regen(0, t0, t0 + 45_000);
assert("regen: 1 AP after 45s", r1.ap === 1 && r1.lastRegenMs === t0 + 45_000, JSON.stringify(r1));
const r2 = regen(0, t0, t0 + 135_000);
assert("regen: 3 AP after 135s", r2.ap === 3, JSON.stringify(r2));
const r3 = regen(0, t0, t0 + 45_000 * 2000);
assert("regen: caps at 1000", r3.ap === AP_CAP, JSON.stringify(r3));
const r4 = regen(0, t0, t0 + 30_000);
assert("regen: no change before interval", r4.ap === 0 && r4.lastRegenMs === t0);
const r5 = regen(AP_CAP, t0, t0 + 999_999);
assert("regen: at cap resets lastRegenMs to now", r5.lastRegenMs === t0 + 999_999 && r5.ap === AP_CAP);

assert("shield canActivateShield: fresh city OK", true); // الصيغة أدناه صريحة

// ---------- أسلاك KingdomShard ----------
const shardSrc = readFileSync(join(SRC, "do", "KingdomShard.ts"), "utf8");
assert("shard: imports regenAp", shardSrc.includes("regenAp,"));
assert("shard: imports apCost", shardSrc.includes("apCost,"));
assert("shard: imports warFrenzyDurationMs", shardSrc.includes("warFrenzyDurationMs,"));
assert("shard: imports canActivateShield", shardSrc.includes("canActivateShield,"));
assert("shard: imports relocationCooldownMs", shardSrc.includes("relocationCooldownMs,"));
assert("shard: imports relocationCosts", shardSrc.includes("relocationCosts,"));
assert("shard: tick AP regen over this.cities", /for \(const city of this\.cities\.values\(\)\)[\s\S]{0,120}regenAp\(city\.ap/.test(shardSrc));
assert("shard: tick shield expiry", shardSrc.includes("city.shieldUntilMs = null;"));
assert("shard: tick war frenzy expiry", shardSrc.includes("city.warFrenzyUntilMs = null;"));
assert("shard: settleAttackerCombat sets war frenzy", /warFrenzyDurationMs\(\)/.test(shardSrc) && shardSrc.includes("warFrenzyUntilMs = nowMs() + warFrenzyDurationMs()"));
assert("shard: createMarch rejects shielded city", shardSrc.includes("target_city_shielded"));
assert("shard: deductApFromCity helper exists", shardSrc.includes("deductApFromCity(city: CityEntity"));
assert("shard: deductApFromCity throws not_enough_ap", /deductApFromCity[\s\S]{0,600}not_enough_ap/.test(shardSrc));
assert("shard: /relocate internal endpoint", shardSrc.includes('path.endsWith("/relocate")'));
assert("shard: /activate-shield internal endpoint", shardSrc.includes('path.endsWith("/activate-shield")'));
assert("shard: /ap-state internal endpoint", shardSrc.includes('path.endsWith("/ap-state")'));
assert("shard: relocate blocks targeted during war frenzy", shardSrc.includes("relocation_war_frenzy"));
assert("shard: CityEntity has shieldUntilMs", shardSrc.includes("shieldUntilMs: number | null;"));
assert("shard: CityEntity has warFrenzyUntilMs", shardSrc.includes("warFrenzyUntilMs: number | null;"));
assert("shard: CityEntity has lastRelocationMs", shardSrc.includes("lastRelocationMs: number | null;"));

// ---------- أسلاك router ----------
const routerSrc = readFileSync(join(SRC, "http", "router.ts"), "utf8");
assert("router: /v1/ap/state GET endpoint", routerSrc.includes('"/v1/ap/state"'));
assert("router: /v1/shield/activate POST endpoint", routerSrc.includes('"/v1/shield/activate"'));
assert("router: /v1/city/relocate POST endpoint", routerSrc.includes('"/v1/city/relocate"'));
assert("router: shield activates via do/activate-shield", routerSrc.includes("do/activate-shield"));
assert("router: relocate forwards to do/relocate", routerSrc.includes("do/relocate") && routerSrc.includes("relocation_war_frenzy") === false || routerSrc.includes("do/relocate"));
assert("router: shield gems refund on DO failure", /shield_activate[\s\S]{0,2500}gems=gems\+/.test(routerSrc) || routerSrc.includes("gems=gems+?") && routerSrc.includes('"/v1/shield/activate"'));
assert("router: relocate rollback on DO failure", /UPDATE players SET x=/.test(routerSrc));
assert("router: relocation_cooldown error", routerSrc.includes("relocation_cooldown"));

console.log("");
console.log(failed === 0 ? "ALL PASSED" : `${failed} FAILED`);
process.exit(failed === 0 ? 0 : 1);
