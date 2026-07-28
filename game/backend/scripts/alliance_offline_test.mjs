#!/usr/bin/env node
/**
 * Offline unit checks for the full alliance system (P2-T5):
 * ranks + helps + rally — rules driven only by data/zones.json → alliance
 * (mirrors src/do/sim/alliance.ts logic).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const ZONES = JSON.parse(readFileSync(join(here, "../src/data/zones.json"), "utf8"));
const CFG = ZONES.alliance;

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ---- mirror of sim/alliance.ts ----
const PERMS = CFG.rank_permissions;
const RANKS = CFG.ranks;
const rankHas = (rank, perm) => (PERMS[rank] || []).includes(perm);
const rankLevel = (rank) => { const i = RANKS.indexOf(rank); return i < 0 ? 0 : i + 1; };
const isValidRank = (rank) => RANKS.includes(rank);
const canModerate = (a, t) => rankLevel(a) > rankLevel(t) && rankLevel(a) >= 4;
const helpSpeedupSec = (remainingMs, helpsCount) => {
  const h = CFG.help;
  const capped = Math.min(helpsCount, h.max_helps_per_queue);
  const rawSec = capped * h.speedup_per_help_sec;
  const maxSec = (remainingMs / 1000) * h.max_reduction_ratio;
  return Math.min(rawSec, maxSec);
};
const helpsCapped = (n) => n >= CFG.help.max_helps_per_queue;
const canLaunchRally = (rank, targetType) =>
  rankLevel(rank) >= rankLevel(CFG.rally.min_rank) && CFG.rally.allowed_targets.includes(targetType);
const rallyFull = (n) => n >= CFG.rally.max_participants;

// ---- 1. config shape ----
assert(Array.isArray(RANKS) && RANKS.length === 5 && RANKS[0] === "R1" && RANKS[4] === "R5", `5 ranks R1..R5 (${RANKS})`);
assert(CFG.max_members === 50, "max_members = 50");
assert(CFG.help.speedup_per_help_sec === 60 && CFG.help.max_helps_per_queue === 10 && CFG.help.max_reduction_ratio === 0.3,
  "help rules: 60s/help, max 10 helps, 30% max reduction");
assert(CFG.rally.min_rank === "R3" && CFG.rally.max_participants === 5 && CFG.rally.prep_seconds === 300,
  "rally rules: min R3, max 5 participants, 300s prep");
assert(JSON.stringify(CFG.rally.allowed_targets) === JSON.stringify(["pass", "throne"]), "rally targets: pass + throne only");

// ---- 2. rank permissions hierarchy ----
assert(rankHas("R5", "kick") && rankHas("R5", "promote") && rankHas("R5", "rally") && rankHas("R5", "help"), "R5 has all permissions");
assert(rankHas("R4", "kick") && rankHas("R4", "rally"), "R4 can kick + rally");
assert(!rankHas("R3", "kick") && rankHas("R3", "rally"), "R3 can rally but not kick");
assert(!rankHas("R2", "rally") && rankHas("R2", "help"), "R2 can only help");
assert(!rankHas("R1", "promote") && rankHas("R1", "help"), "R1 can only help");
assert(!rankHas("R0", "help"), "unknown rank has no permissions");
assert(rankLevel("R5") === 5 && rankLevel("R1") === 1 && rankLevel("XX") === 0, "rankLevel numeric mapping");
assert(isValidRank("R3") && !isValidRank("R6"), "isValidRank validates membership");

// ---- 3. moderation rules: only strictly higher rank moderates lower ----
assert(canModerate("R5", "R4") && canModerate("R4", "R3"), "higher rank can moderate lower");
assert(!canModerate("R3", "R1"), "R3 cannot moderate despite higher level (below officer tier)");
assert(!canModerate("R4", "R4") && !canModerate("R4", "R5"), "cannot moderate equal or higher rank");

// ---- 4. help speedup math (cumulative + capped) ----
const hour = 3600_000;
assert(helpSpeedupSec(hour, 1) === 60, `1 help on 1h queue = 60s (got ${helpSpeedupSec(hour, 1)})`);
assert(helpSpeedupSec(hour, 5) === 300, `5 helps = 300s (got ${helpSpeedupSec(hour, 5)})`);
assert(helpSpeedupSec(hour, 10) === 600, "10 helps = 600s");
assert(helpSpeedupSec(hour, 15) === 600, "helps beyond cap still 600s (max_helps_per_queue)");
const short = 1000_000; // 1000s queue → 30% cap = 300s
assert(helpSpeedupSec(short, 10) === 300, `30% reduction cap on short queue (got ${helpSpeedupSec(short, 10)})`);
assert(helpSpeedupSec(short, 2) === 120, "2 helps on short queue = 120s (below cap)");
assert(helpsCapped(10) && !helpsCapped(9), "helpsCapped boundary at 10");

// ---- 5. rally rules ----
assert(canLaunchRally("R3", "pass") && canLaunchRally("R5", "throne"), "R3+ can rally pass/throne");
assert(!canLaunchRally("R2", "pass"), "R2 cannot rally");
assert(!canLaunchRally("R4", "city"), "rally on city not allowed");
assert(rallyFull(5) && !rallyFull(4), "rally full at 5 participants");

// ---- 6. merged rally troops math (DO aggregation mirror) ----
const parts = [
  { infantry_t1: 100, archer_t1: 50 },
  { infantry_t1: 80 },
  { archer_t1: 20, cavalry_t1: 10 },
];
const merged = {};
for (const p of parts) for (const [u, c] of Object.entries(p)) merged[u] = (merged[u] || 0) + c;
assert(merged.infantry_t1 === 180 && merged.archer_t1 === 70 && merged.cavalry_t1 === 10,
  `rally troop merge aggregates all participants (${JSON.stringify(merged)})`);

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE ALLIANCE CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
