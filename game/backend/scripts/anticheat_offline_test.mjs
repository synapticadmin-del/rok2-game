#!/usr/bin/env node
/**
 * Offline unit checks for anti-cheat (P4-T5).
 * Replicates sim/anticheat.ts logic against data/anticheat.json
 * (Workers TS isn't runnable directly in plain node, so we re-implement the
 * pure formulas here and assert the data + math contract).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const data = JSON.parse(readFileSync(join(here, "../src/data/anticheat.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

const CONSTANTS = data.constants;
const RATE_LIMITS = data.rate_limits;
const ANOMALY = data.anomaly;

// ---- replica of sim/anticheat.ts ----
class RateLimiter {
  constructor() { this.buckets = new Map(); }
  check(playerId, action, now) {
    if (!CONSTANTS.enabled) return { allowed: true };
    const rule = RATE_LIMITS[action];
    const key = `${playerId}:${action}`;
    let b = this.buckets.get(key);
    if (!b) {
      this.buckets.set(key, { windowStart: now, count: 1, lastAction: now });
      return { allowed: true };
    }
    if (now - b.windowStart >= rule.window_ms) { b.windowStart = now; b.count = 0; }
    if (rule.cooldown_ms > 0 && now - b.lastAction < rule.cooldown_ms) {
      return { allowed: false, reason: "cooldown", retryAfterMs: rule.cooldown_ms - (now - b.lastAction) };
    }
    if (b.count >= rule.max_actions) {
      return { allowed: false, reason: "window_exceeded", retryAfterMs: rule.window_ms - (now - b.windowStart) };
    }
    b.count += 1; b.lastAction = now;
    return { allowed: true };
  }
  reset(playerId) {
    for (const k of [...this.buckets.keys()]) if (k.startsWith(playerId + ":")) this.buckets.delete(k);
  }
}

function checkMarchPayload(troops, activeMarches) {
  if (!CONSTANTS.enabled) return null;
  if (activeMarches >= ANOMALY.max_active_marches_per_player) return "max_active_marches_exceeded";
  let total = 0;
  for (const c of Object.values(troops)) {
    const n = Number(c);
    if (!Number.isInteger(n) || n <= 0) return "invalid_troop_count";
    if (n > ANOMALY.max_single_unit_per_march) return "single_unit_cap_exceeded";
    total += n;
  }
  if (total > ANOMALY.max_troops_per_march) return "total_troops_cap_exceeded";
  return null;
}

function checkShopBuyPayload(count) {
  if (!CONSTANTS.enabled) return null;
  if (!Number.isInteger(count) || count <= 0) return "invalid_buy_count";
  if (count > ANOMALY.max_shop_buy_count) return "buy_count_cap_exceeded";
  return null;
}

// ---- data contract ----
assert(typeof CONSTANTS.enabled === "boolean", "constants.enabled is boolean");
assert(Number.isInteger(CONSTANTS.violation_log_limit) && CONSTANTS.violation_log_limit > 0, "violation_log_limit positive int");
const ACTIONS = ["march", "pass_attack", "help", "shop_buy", "use_speedup", "rally"];
assert(ACTIONS.every((a) => RATE_LIMITS[a]), "all 6 rate-limit actions present");
for (const a of ACTIONS) {
  const r = RATE_LIMITS[a];
  assert(Number.isInteger(r.window_ms) && r.window_ms > 0, `${a}: window_ms positive`);
  assert(Number.isInteger(r.max_actions) && r.max_actions > 0, `${a}: max_actions positive`);
  assert(Number.isInteger(r.cooldown_ms) && r.cooldown_ms >= 0, `${a}: cooldown_ms >= 0`);
  assert(r.cooldown_ms <= r.window_ms, `${a}: cooldown <= window`);
}
assert(ANOMALY.max_troops_per_march >= ANOMALY.max_single_unit_per_march, "total troops cap >= single unit cap");
assert(ANOMALY.max_active_marches_per_player > 0, "max active marches positive");
assert(ANOMALY.max_shop_buy_count >= 1, "shop buy cap >= 1");

// ---- rate limiter behavior ----
const T0 = 1_000_000;
const rl = new RateLimiter();

// march: max 10/min, cooldown 1s
let blocked = null;
for (let i = 0; i < RATE_LIMITS.march.max_actions; i++) {
  const r = rl.check("p1", "march", T0 + i * RATE_LIMITS.march.cooldown_ms);
  if (!r.allowed) { blocked = r; break; }
}
assert(blocked === null, "march: exactly max_actions pass within window at cooldown pace");

const over = rl.check("p1", "march", T0 + RATE_LIMITS.march.max_actions * RATE_LIMITS.march.cooldown_ms);
assert(!over.allowed && over.reason === "window_exceeded", `march: 11th action in window blocked (got ${JSON.stringify(over)})`);
assert(over.retryAfterMs > 0, "march: retryAfterMs positive on window block");

// cooldown: two actions too close
const rl2 = new RateLimiter();
rl2.check("p2", "march", T0);
const cd = rl2.check("p2", "march", T0 + 100);
assert(!cd.allowed && cd.reason === "cooldown", "march: 100ms gap blocked by 1s cooldown");
const cdOk = rl2.check("p2", "march", T0 + RATE_LIMITS.march.cooldown_ms);
assert(cdOk.allowed, "march: exactly at cooldown boundary allowed");

// window reset
const rl3 = new RateLimiter();
for (let i = 0; i < RATE_LIMITS.march.max_actions; i++) rl3.check("p3", "march", T0 + i * 1000);
const afterWindow = rl3.check("p3", "march", T0 + RATE_LIMITS.march.window_ms);
assert(afterWindow.allowed, "march: allowed again after window expiry");

// isolation between players + actions
const rl4 = new RateLimiter();
rl4.check("p4", "march", T0);
assert(rl4.check("p5", "march", T0).allowed, "different player unaffected");
assert(rl4.check("p4", "help", T0).allowed, "different action unaffected");

// rally: max 3/min with 5s cooldown
const rl5 = new RateLimiter();
for (let i = 0; i < RATE_LIMITS.rally.max_actions; i++) {
  assert(rl5.check("p6", "rally", T0 + i * RATE_LIMITS.rally.cooldown_ms).allowed, `rally ${i + 1}/3 allowed`);
}
assert(!rl5.check("p6", "rally", T0 + 3 * RATE_LIMITS.rally.cooldown_ms).allowed, "rally 4th blocked");

// help: max 15/min with 0.5s cooldown
const rl6 = new RateLimiter();
let helpBlocked = null;
for (let i = 0; i < RATE_LIMITS.help.max_actions; i++) {
  const r = rl6.check("p7", "help", T0 + i * RATE_LIMITS.help.cooldown_ms);
  if (!r.allowed) { helpBlocked = r; break; }
}
assert(helpBlocked === null, "help: 15 helps pass within window");
assert(!rl6.check("p7", "help", T0 + 15 * RATE_LIMITS.help.cooldown_ms).allowed, "help: 16th blocked");

// ---- anomaly checks ----
assert(checkMarchPayload({ infantry_t1: 1000 }, 0) === null, "normal march payload passes");
assert(checkMarchPayload({ infantry_t1: ANOMALY.max_single_unit_per_march + 1 }, 0) === "single_unit_cap_exceeded", "single unit over cap rejected");
assert(checkMarchPayload({ a: 60000, b: 60000, c: 90000 }, 0) === "total_troops_cap_exceeded", "total over cap rejected");
assert(checkMarchPayload({ infantry_t1: 100 }, ANOMALY.max_active_marches_per_player) === "max_active_marches_exceeded", "active march cap enforced");
assert(checkMarchPayload({ infantry_t1: -5 }, 0) === "invalid_troop_count", "negative count rejected");
assert(checkMarchPayload({ infantry_t1: 2.5 }, 0) === "invalid_troop_count", "fractional count rejected");
assert(checkShopBuyPayload(5) === null, "normal buy count passes");
assert(checkShopBuyPayload(ANOMALY.max_shop_buy_count + 1) === "buy_count_cap_exceeded", "buy count over cap rejected");
assert(checkShopBuyPayload(0) === "invalid_buy_count", "zero buy rejected");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE ANTICHEAT CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
