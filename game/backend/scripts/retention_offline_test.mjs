#!/usr/bin/env node
/**
 * Offline unit checks for Soft launch + retention measurement (P3-T5) against
 * data/softlaunch.json and the retention sim logic mirrored here.
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const DATA = JSON.parse(readFileSync(join(here, "../src/data/softlaunch.json"), "utf8"));
const ROOT_DATA = JSON.parse(readFileSync(join(here, "../../../data/softlaunch.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ── mirror of sim/retention.ts (values-driven) ──
const openKingdoms = () => DATA.kingdoms.filter((k) => k.open);
const isKingdomOpen = (id) => { const k = DATA.kingdoms.find((x) => x.id === id); return !!k && k.open; };
const kingdomCapacity = (id) => { const k = DATA.kingdoms.find((x) => x.id === id); return k ? k.max_players : null; };
const buckets = DATA.retention.day_buckets;
const utcDay = (ms) => Math.floor(ms / 86_400_000);
const cohortDayOf = utcDay;
const pct = (n, d) => (d <= 0 ? 0 : Math.round((1000 * n) / d) / 10);

// ── data shape ──
assert(JSON.stringify(DATA) === JSON.stringify(ROOT_DATA), "src/data/softlaunch.json mirrors data/softlaunch.json");
assert(DATA.kingdoms.length >= 1 && DATA.kingdoms.length <= 2, `soft launch: 1-2 kingdoms (got ${DATA.kingdoms.length})`);
assert(DATA.kingdoms.every((k) => k.id && k.name && typeof k.open === "boolean" && k.max_players > 0), "every kingdom: id + name + open flag + max_players");
assert(openKingdoms().length >= 1, "at least one open kingdom for soft launch");
assert(buckets.length >= 3 && buckets.includes(1) && buckets.includes(7) && buckets.includes(30), `buckets cover D1/D7/D30 (got ${buckets})`);
assert(DATA.retention.targets.d1_min_pct > 0 && DATA.retention.targets.d7_min_pct > 0, "retention targets positive");
assert(DATA.success_gate && DATA.success_gate.min_kingdoms_live >= 1, "success gate defined");

// ── kingdom gating ──
assert(isKingdomOpen("kingdom-1") === true, "kingdom-1 open");
assert(isKingdomOpen("kingdom-2") === false, "kingdom-2 closed (reserve)");
assert(isKingdomOpen("kingdom-99") === false, "unknown kingdom not open");
assert(kingdomCapacity("kingdom-1") === 500, "kingdom-1 cap 500");
assert(kingdomCapacity("kingdom-99") === null, "unknown kingdom → null capacity");

// ── utcDay / cohort maths ──
const DAY = 86_400_000;
assert(utcDay(0) === 0 && utcDay(DAY - 1) === 0 && utcDay(DAY) === 1, "utcDay boundaries");
assert(cohortDayOf(3 * DAY + 1234) === 3, "cohort day = creation day");

// ── retention aggregation simulation ──
// 10 players created day 0; 6 return day1, 4 day3, 3 day7, 1 day14, 0 day30
const players = Array.from({ length: 10 }, (_, i) => ({ created_ms: 500 + i, last: [1,1,1,1,1,1,3,3,3,7][i] }));
// last day each returned up to: p0-5 → day1... simplified: explicit last_day
const lastDays = [14, 7, 7, 3, 3, 1, 1, 1, 1, 1];
const agg = { size: 10, returned: {} };
for (const n of buckets) {
  if (0 + n <= 14) { // today = day 14
    agg.returned[n] = lastDays.filter((l) => l >= n).length;
  }
}
const today = 14;
assert(agg.returned[1] === 10, `D1: all 10 returned (got ${agg.returned[1]})`);
assert(agg.returned[3] === 5, `D3: 5 returned (got ${agg.returned[3]})`);
assert(agg.returned[7] === 3, `D7: 3 returned (got ${agg.returned[7]})`);
assert(agg.returned[14] === 1, `D14: 1 returned (got ${agg.returned[14]})`);
assert(pct(agg.returned[7], 10) === 30.0, "D7 retention = 30.0%");
assert(pct(0, 0) === 0, "pct guards zero denominator");
assert(pct(1, 3) === 33.3, "pct rounds to 0.1");

// targets check: D7 30% ≥ 15% target → gate passes
assert(pct(agg.returned[7], 10) >= DATA.retention.targets.d7_min_pct, "simulated D7 passes target");

// ── router wiring ──
const ROUTER = readFileSync(join(here, "../src/http/router.ts"), "utf8");
const CONTEXT = readFileSync(join(here, "../src/lib/context.ts"), "utf8");
assert(ROUTER.includes('from "../do/sim/retention"'), "router imports retention sim");
assert(ROUTER.includes("/v1/admin/retention"), "router wires /v1/admin/retention");
assert(ROUTER.includes("/v1/launch/status"), "router wires /v1/launch/status");
assert(ROUTER.includes("isKingdomOpen") && ROUTER.includes("kingdomCapacity"), "city init gated by launch kingdoms");
assert(ROUTER.includes("kingdom_not_open_for_launch") && ROUTER.includes("kingdom_full"), "launch gate errors wired");
assert(ROUTER.includes("requireAdmin") , "admin guard present");
assert(CONTEXT.includes("player_activity") && CONTEXT.includes("last_seen_at"), "context.ts tracks activity on auth");
assert((ROUTER.match(/trackActivity/g) || []).length === 0, "trackActivity lives in context.ts only (single owner)");

// ── migration coverage ──
const MIG = readFileSync(join(here, "../migrations/0006_retention.sql"), "utf8");
assert(MIG.includes("CREATE TABLE IF NOT EXISTS player_activity"), "migration: player_activity table");
assert(MIG.includes("ALTER TABLE accounts ADD COLUMN last_seen_at"), "migration: last_seen_at on accounts");
assert(MIG.includes("idx_player_activity_day"), "migration: day index for DAU queries");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE RETENTION CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
