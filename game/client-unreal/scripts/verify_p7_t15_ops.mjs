#!/usr/bin/env node
// P7-T15 guard — health/ops monitoring (KingdomShard ops endpoints + thresholds from JSON, no hardcoding).
// Exit 0 regardless — prints PASS/FAIL counts for CI visibility.
import { readFileSync, existsSync } from "node:fs";
import { join } from "node:path";
import { fileURLToPath } from "node:url";

// script lives in game/client-unreal/scripts — repo root is four levels up (scripts→client-unreal→game→repo)
const root = join(fileURLToPath(import.meta.url), "..", "..", "..", "..");
let pass = 0, fail = 0;
function check(name, cond, detail = "") {
  if (cond) { pass++; console.log(`PASS ${name}`); }
  else { fail++; console.log(`FAIL ${name} ${detail}`); }
}

// 1. ops.json exists and has required constants
const opsPath = join(root, "game/backend/src/data/ops.json");
check("ops.json exists", existsSync(opsPath));
const ops = existsSync(opsPath) ? JSON.parse(readFileSync(opsPath, "utf8")) : {};
const requiredKeys = ["enabled", "command_error_window_ms", "tick_stale_threshold_ms", "queue_stuck_threshold", "queue_stuck_age_ms", "command_alert_threshold", "error_log_limit"];
for (const k of requiredKeys) check(`ops.json constants.${k}`, typeof ops.constants?.[k] === "number" || typeof ops.constants?.[k] === "boolean");

// 2. KingdomShard reads constants from the JSON file (no hardcoded thresholds)
const shardSrc = readFileSync(join(root, "game/backend/src/do/KingdomShard.ts"), "utf8");
check("KingdomShard imports ops.json", shardSrc.includes('import opsData from "../data/ops.json"'));
check("KingdomShard defines OPS_CONSTANTS", /const OPS_CONSTANTS\s*=\s*\{/.test(shardSrc));
check("KingdomShard has opsSnapshot()", shardSrc.includes("opsSnapshot()"));
check("KingdomShard has recordCommandError()", shardSrc.includes("recordCommandError("));
check("KingdomShard has /ops endpoint", shardSrc.includes('path === "/ops"'));
check("KingdomShard emits tick_stale alert", shardSrc.includes('"tick_stale"'));
check("KingdomShard emits queue_pressure alert", shardSrc.includes('"queue_pressure"'));
check("KingdomShard emits queue_stuck alert", shardSrc.includes('"queue_stuck"'));
check("opsSnapshot exposes health status", shardSrc.includes('healthStatus: "starting" | "healthy" | "degraded"') && shardSrc.includes("const healthStatus"));
check("opsSnapshot exposes checkedAtMs", shardSrc.includes("checkedAtMs"));
check("opsSnapshot exposes queuesStuck", shardSrc.includes("queuesStuck") && shardSrc.includes("QUEUE_STUCK_AGE_MS"));
check("KingdomShard emits command_error_X alerts", /command_error_\$\{e\.code\}/.test(shardSrc));
check("KingdomShard updates lastTickMs in tick", /lastTickMs\s*=\s*now/.test(shardSrc));
check("KingdomShard records tick duration", shardSrc.includes("lastTickDurationMs") && shardSrc.includes("totalTickDurationMs"));
check("opsSnapshot exposes tick duration metrics", /avgTickDurationMs[\s\S]*maxTickDurationMs[\s\S]*tickCount/.test(shardSrc));
check("KingdomShard tracks commandErrorCounts Map", shardSrc.includes("commandErrorCounts = new Map<string,"));
// No hardcoded thresholds — numeric literals must come from constants
check("No hardcoded 3600000 literal", !shardSrc.includes("3600000"));
check("No hardcoded 30000 literal", !/\b30000\b/.test(shardSrc));
check("No hardcoded queue stuck age literal", !/\b120000\b/.test(shardSrc));
check("No hardcoded window 60*60 pattern for thresholds", !/3600\s*\*\s*1000/.test(shardSrc));

// 3. Router exposes admin ops endpoint behind requireAdmin
const routerSrc = readFileSync(join(root, "game/backend/src/http/router.ts"), "utf8");
check("router has /v1/admin/ops", routerSrc.includes('"/v1/admin/ops"'));
check("router uses requireAdmin for ops", /\/v1\/admin\/ops"[\s\S]*?requireAdmin\(request, env\)/.test(routerSrc) || /requireAdmin\(request, env\)[\s\S]{0,400}?\/v1\/admin\/ops/.test(routerSrc));
check("router fetches /ops from shard stub", routerSrc.includes('"https://do/ops"'));

console.log(`\n${pass} passed, ${fail} failed`);
process.exit(0);
