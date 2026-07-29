#!/usr/bin/env node
/**
 * Offline unit checks for kingdom matchmaking (P4-T6).
 * Replicates sim/matchmaking.ts logic against data/matchmaking.json + data/softlaunch.json
 * (Workers TS isn't runnable directly in plain node, so we re-implement the pure
 * formulas here and assert the data + math contract).
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const cfg = JSON.parse(readFileSync(join(here, "../src/data/matchmaking.json"), "utf8"));
const softlaunch = JSON.parse(readFileSync(join(here, "../src/data/softlaunch.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ---- replica of sim/matchmaking.ts ----
function fillRatio(playerCount, maxPlayers) {
  if (maxPlayers <= 0) return 1;
  return playerCount / maxPlayers;
}

function chooseKingdom(candidates, counts, rrCounters = {}) {
  const open = candidates.filter((k) => k.open && k.max_players > 0);
  if (open.length === 0) return null;
  const g = cfg.guardrails;
  const viable = open.filter((k) => fillRatio(counts[k.id] || 0, k.max_players) < g.hard_cap_ratio);
  if (viable.length === 0) return null;

  const strategy = cfg.strategy;
  if (strategy === "round_robin") {
    const sorted = [...viable].sort((a, b) => {
      const da = rrCounters[a.id] || 0, db = rrCounters[b.id] || 0;
      if (da !== db) return da - db;
      return a.id.localeCompare(b.id);
    });
    const pick = sorted[0];
    return { kingdomId: pick.id, strategy, fillRatio: fillRatio(counts[pick.id] || 0, pick.max_players), reason: "rr" };
  }
  const sorted = [...viable].sort((a, b) => {
    const fa = fillRatio(counts[a.id] || 0, a.max_players);
    const fb = fillRatio(counts[b.id] || 0, b.max_players);
    if (fa !== fb) return fa - fb;
    const ca = counts[a.id] || 0, cb = counts[b.id] || 0;
    if (ca !== cb) return ca - cb;
    return a.id.localeCompare(b.id);
  });
  const pick = sorted[0];
  return { kingdomId: pick.id, strategy, fillRatio: fillRatio(counts[pick.id] || 0, pick.max_players), reason: "lf" };
}

// ---- data contract ----
assert(["least_fill", "round_robin"].includes(cfg.strategy), "strategy is a known value");
assert(cfg.strategies[cfg.strategy], "active strategy documented in strategies map");
assert(cfg.guardrails.hard_cap_ratio > 0 && cfg.guardrails.hard_cap_ratio <= 1.5, "hard_cap_ratio sane");
assert(cfg.guardrails.prefer_below_ratio > 0 && cfg.guardrails.prefer_below_ratio <= cfg.guardrails.hard_cap_ratio, "prefer_below_ratio <= hard_cap");
assert(typeof cfg.guardrails.fallback_to_env_kingdom === "boolean", "fallback_to_env_kingdom boolean");

// softlaunch kingdoms shape for candidates
assert(Array.isArray(softlaunch.kingdoms) && softlaunch.kingdoms.length >= 1, "softlaunch has kingdoms");
for (const k of softlaunch.kingdoms) {
  assert(typeof k.id === "string" && typeof k.open === "boolean" && Number.isInteger(k.max_players), `kingdom ${k.id} shape valid`);
}

// ---- fillRatio ----
assert(fillRatio(0, 500) === 0, "fillRatio empty = 0");
assert(fillRatio(250, 500) === 0.5, "fillRatio half = 0.5");
assert(fillRatio(500, 500) === 1, "fillRatio full = 1");
assert(fillRatio(10, 0) === 1, "fillRatio zero capacity = 1 (treated full)");

// ---- least_fill behavior ----
const K2 = [
  { id: "kingdom-1", open: true, max_players: 500 },
  { id: "kingdom-2", open: true, max_players: 500 },
];

// empty both → alphabetical first (kingdom-1)
assert(chooseKingdom(K2, {}).kingdomId === "kingdom-1", "empty both: kingdom-1 (tie → alphabetical)");

// kingdom-1 fuller → kingdom-2
assert(chooseKingdom(K2, { "kingdom-1": 300, "kingdom-2": 100 }).kingdomId === "kingdom-2", "k1 fuller: picks kingdom-2");

// kingdom-2 fuller → kingdom-1
assert(chooseKingdom(K2, { "kingdom-1": 50, "kingdom-2": 400 }).kingdomId === "kingdom-1", "k2 fuller: picks kingdom-1");

// equal counts → alphabetical
assert(chooseKingdom(K2, { "kingdom-1": 100, "kingdom-2": 100 }).kingdomId === "kingdom-1", "equal fill: alphabetical kingdom-1");

// unequal capacity: ratio wins over raw count
const KUnequal = [
  { id: "big", open: true, max_players: 1000 },
  { id: "small", open: true, max_players: 100 },
];
// big has 500 (50%), small has 40 (40%) → small less filled
assert(chooseKingdom(KUnequal, { big: 500, small: 40 }).kingdomId === "small", "unequal capacity: ratio decides (small 40% < big 50%)");

// hard cap: full kingdom excluded
assert(chooseKingdom(K2, { "kingdom-1": 500, "kingdom-2": 100 }).kingdomId === "kingdom-2", "k1 at hard cap: excluded, picks kingdom-2");

// all full → null
assert(chooseKingdom(K2, { "kingdom-1": 500, "kingdom-2": 500 }) === null, "all full: null (no assignment)");

// closed kingdom never picked even if empty
const KClosed = [
  { id: "kingdom-1", open: true, max_players: 500 },
  { id: "kingdom-2", open: false, max_players: 500 },
];
assert(chooseKingdom(KClosed, { "kingdom-1": 499 }).kingdomId === "kingdom-1", "closed kingdom never picked");

// no open kingdoms → null
assert(chooseKingdom([{ id: "x", open: false, max_players: 100 }], {}) === null, "no open kingdoms: null");

// ---- round_robin behavior (forced via temp cfg override in replica) ----
const savedStrategy = cfg.strategy;
cfg.strategy = "round_robin";
assert(chooseKingdom(K2, {}, {}).kingdomId === "kingdom-1", "rr empty: kingdom-1 first");
assert(chooseKingdom(K2, {}, { "kingdom-1": 1 }).kingdomId === "kingdom-2", "rr: next goes to least assigned (kingdom-2)");
assert(chooseKingdom(K2, {}, { "kingdom-1": 1, "kingdom-2": 1 }).kingdomId === "kingdom-1", "rr tied: alphabetical");
// rr ignores fill ratio (by design — alternation)
assert(chooseKingdom(K2, { "kingdom-1": 400, "kingdom-2": 10 }, { "kingdom-1": 0, "kingdom-2": 5 }).kingdomId === "kingdom-1", "rr: assignment count wins over fill");
cfg.strategy = savedStrategy;

// single open kingdom → always that one (matchmaking degenerate case)
assert(chooseKingdom([{ id: "kingdom-1", open: true, max_players: 500 }], { "kingdom-1": 499 }).kingdomId === "kingdom-1", "single kingdom: picked when not full");
assert(chooseKingdom([{ id: "kingdom-1", open: true, max_players: 500 }], { "kingdom-1": 500 }) === null, "single kingdom full: null");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE MATCHMAKING CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
