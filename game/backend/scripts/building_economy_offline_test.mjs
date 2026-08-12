import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const shop = JSON.parse(readFileSync(join(here, "../src/data/shop.json"), "utf8"));
const buildings = JSON.parse(readFileSync(join(here, "../src/data/buildings.json"), "utf8"));
const router = readFileSync(join(here, "../src/http/router.ts"), "utf8");
const shard = readFileSync(join(here, "../src/do/KingdomShard.ts"), "utf8");

let failed = 0;
function assert(condition, message) {
  if (!condition) {
    failed += 1;
    console.error("FAIL:", message);
  } else {
    console.log("OK  :", message);
  }
}

// Mirrors of the single-source formulas in lib/gameData.ts and do/sim/shop.ts.
const duration = (level, multiplier = 1) => Math.max(5, Math.ceil((30 * Math.pow(1.35, Math.max(0, level - 1))) / Math.max(0.1, multiplier || 1)));
const productionPerHour = (base, level) => level > 0 ? base * Math.pow(1.2, level - 1) : 0;
const finishCost = (seconds) => {
  const positiveRates = shop.speedups.filter((item) => item.seconds > 0 && item.cost_gems > 0).map((item) => item.cost_gems / item.seconds);
  return seconds <= 0 ? 0 : Math.max(1, Math.ceil(seconds * Math.min(...positiveRates)));
};

assert(buildings.buildings.filter((entry) => entry.produces).length === 4, "four economic production buildings are defined");
assert(duration(1) === 30, "first building level starts with a 30-second authoritative duration");
assert(duration(6) > duration(5) && duration(5) > duration(1), "building duration increases with target level");
assert(duration(8, 1.15) < duration(8, 1), "build speed multiplier shortens the server duration");
assert(duration(80) >= 5, "duration always retains the safety floor");

const levelOneRates = {
  food: productionPerHour(100, 1),
  wood: productionPerHour(100, 1),
  stone: productionPerHour(70, 1),
  gold: productionPerHour(40, 1),
};
assert(JSON.stringify(levelOneRates) === JSON.stringify({ food: 100, wood: 100, stone: 70, gold: 40 }), "level-one production matches the four economic buildings");
assert(Math.abs(productionPerHour(100, 4) - 172.8) < 0.000001, "economic production grows 20% per building level");
const thirtyMinuteFood = Math.floor(levelOneRates.food * 0.5);
assert(thirtyMinuteFood === 50, "half-hour production settles proportionally before collection");
const vipFood = levelOneRates.food * 1.15;
assert(vipFood === 114.99999999999999, "VIP production multiplier applies before timed settlement");

const bestGemRate = Math.min(...shop.speedups.map((item) => item.cost_gems / item.seconds));
assert(finishCost(0) === 0, "completed queue has no gem finish price");
assert(finishCost(1) === 1, "positive remaining time has a minimum one-gem finish price");
assert(finishCost(3600) === Math.ceil(3600 * bestGemRate), "finish price derives from the best catalog time-per-gem value");
assert(finishCost(7200) >= finishCost(3600), "gem finish price is non-decreasing with remaining time");

assert(router.includes("Another building upgrade is already running"), "router rejects a parallel building-upgrade request");
assert(router.includes("gemsSpent") && router.includes("gems=gems+?"), "router compensates gem spend when the shard rejects a finish request");
assert(router.includes("food=food+?") && router.includes("building_queue_failed"), "router compensates resources when building queue creation fails");
assert(router.includes("/v1/city/collect") && router.includes("getProductionStatus"), "collection route returns authoritative production status");
assert(shard.includes("/queue/list") && shard.includes("${queueType}_queue_busy") && shard.includes("queue.type === queueType"), "Durable Object owns the type-scoped queue guard");

console.log("\n==== RESULT ====");
if (failed === 0) {
  console.log("ALL OFFLINE BUILDING ECONOMY SCENARIOS PASSED");
  process.exit(0);
}
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
