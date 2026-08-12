#!/usr/bin/env node
import { existsSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, "../../..");
const errors = [];

function requireFile(relativePath) {
  const absolutePath = join(ROOT, relativePath);
  if (!existsSync(absolutePath)) {
    errors.push(`ملف خط الأساس مفقود: ${relativePath}`);
    return "";
  }
  return readFileSync(absolutePath, "utf8");
}

function requireText(relativePath, content, pattern, description) {
  if (!pattern.test(content)) {
    errors.push(`دليل مفقود في ${relativePath}: ${description}`);
  }
}

const packageJsonPath = "game/backend/package.json";
const packageJson = requireFile(packageJsonPath);
const plan = requireFile("PLAN.md");
const audit = requireFile("game/docs/P7_T0_BASELINE_AUDIT.md");
const artAssets = requireFile("game/client-unreal/Source/Rok2/Public/Rok2ArtAssets.h");
const iconography = requireFile("game/client-unreal/Source/Rok2/Public/Rok2WorldIconography.h");
const seasonWidget = requireFile("game/client-unreal/Source/Rok2/Public/Rok2SeasonStoryWidget.h");
const kingdomShard = requireFile("game/backend/src/do/KingdomShard.ts");

[
  "game/backend/scripts/smoke.mjs",
  "game/client-unreal/Rok2.uproject",
  "game/client-unreal/scripts/verify_ui_sfx.mjs",
  "game/client-unreal/scripts/verify_world_iconography.mjs",
  "game/client-unreal/scripts/verify_season_story.mjs",
  "game/client-unreal/scripts/verify_alliance_rally_loop.mjs",
  "game/client-unreal/scripts/verify_march_combat_queues.mjs",
  "game/client-unreal/scripts/verify_city_layout_sync.mjs",
].forEach(requireFile);

requireText(packageJsonPath, packageJson, /"check"\s*:\s*"[^"]*test:offline[^"]*test:march-redirect-notifications/, "بوابة check المتسلسلة");
requireText(packageJsonPath, packageJson, /"smoke"\s*:\s*"node scripts\/smoke\.mjs"/, "اختبار smoke التشغيلي");
requireText("PLAN.md", plan, /### المرحلة 7[\s\S]*?\*\*P7-T0\*\*/, "تعريف P7-T0 في المرحلة السابعة");
requireText("PLAN.md", plan, /P7-T1[\s\S]*?P7-T9/, "خريطة مهام P7-T1 إلى P7-T9");
requireText("game/docs/P7_T0_BASELINE_AUDIT.md", audit, /INT-01[\s\S]*?INT-05/, "مصفوفة فجوات التكامل INT-01 إلى INT-05");
requireText("Rok2ArtAssets.h", artAssets, /GetUiSfxAssetPath[\s\S]*?GetCivilizationWhisperAssetPath/, "فهرس أصوات P6-T8");
requireText("Rok2WorldIconography.h", iconography, /URok2WorldIconography[\s\S]*?Resolve/, "واجهة أيقونات P6-T9");
requireText("Rok2SeasonStoryWidget.h", seasonWidget, /URok2SeasonStoryWidget[\s\S]*?SetStoryEvents/, "ودجة حكاية المملكة P6-T10");
requireText("KingdomShard.ts", kingdomShard, /seasonStory[\s\S]*?season_story_event/, "سجل وبث القصة الموسمية في الخادم");

const sourceRoot = join(ROOT, "game/client-unreal/Source/Rok2");
const sourceFiles = [
  join(sourceRoot, "Private/Rok2AudioManager.cpp"),
  join(sourceRoot, "Private/Rok2WorldRenderer.cpp"),
  join(sourceRoot, "Private/Rok2Api.cpp"),
  join(sourceRoot, "Private/Rok2HudWidget.cpp"),
];
const runtimeSource = sourceFiles
  .filter(existsSync)
  .map((filePath) => readFileSync(filePath, "utf8"))
  .join("\n");

const integrations = [
  ["INT-01", /GetUiSfxAssetPath|GetCivilizationWhisperAssetPath/, "صوت الواجهة والحضارة"],
  ["INT-02", /URok2WorldIconography|FRok2WorldIconStyle/, "أيقونات عالم P6"],
  ["INT-03", /seasonStory|season_story_event|Rok2SeasonStoryWidget/, "حكاية المملكة"],
];

if (errors.length) {
  console.error("فشل تدقيق P7-T0:");
  errors.forEach((error) => console.error(`- ${error}`));
  process.exit(1);
}

console.log("✓ تدقيق P7-T0: أدلة خط الأساس الأساسية موجودة.");
console.log("حالة الربط التشغيلي (مرجعية لـ P7-T1):");
for (const [id, pattern, label] of integrations) {
  console.log(`- ${id} ${label}: ${pattern.test(runtimeSource) ? "موصول/يحتاج تحقق PIE" : "مفتوح — جاهز للربط"}`);
}
console.log("✓ لا يحول الفحص فجوات P7-T1 المتعمدة إلى إخفاق، لكنه يمنع فقدان دليلها أو فقدان أصولها.");
