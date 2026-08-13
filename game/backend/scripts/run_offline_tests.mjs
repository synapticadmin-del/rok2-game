import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import path from "node:path";

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const testScripts = [
  "anticheat_offline_test.mjs",
  "commanders_offline_test.mjs",
  "hospital_offline_test.mjs",
  "research_offline_test.mjs",
  "alliance_offline_test.mjs",
  "zones_offline_test.mjs",
  "season_offline_test.mjs",
  "season_scoring_offline_test.mjs",
  "events_offline_test.mjs",
  "shop_offline_test.mjs",
  "building_economy_offline_test.mjs",
  "retention_offline_test.mjs",
  "battlepass_offline_test.mjs",
  "matchmaking_offline_test.mjs",
  "talents_offline_test.mjs",
  "equipment_offline_test.mjs",
  "troops_offline_test.mjs",
  "holy_sites_offline_test.mjs",
  "action_points_offline_test.mjs",
  "daily_quests_offline_test.mjs"
];

for (const script of testScripts) {
  console.log(`\n=== ${script} ===`);
  const result = spawnSync(process.execPath, [path.join(scriptDirectory, script)], {
    cwd: scriptDirectory,
    stdio: "inherit"
  });

  if (result.error) {
    console.error(`Unable to run ${script}: ${result.error.message}`);
    process.exit(1);
  }

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

console.log(`\nAll ${testScripts.length} offline game-rule suites passed.`);
