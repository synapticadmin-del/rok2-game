#!/usr/bin/env node

/**
 * P7-T6 structural guard.
 *
 * Verifies the project keeps the code paths required to collect a reproducible
 * world-map performance baseline in UE PIE. It intentionally does not claim
 * device FPS, GPU time, or memory measurements; those require a packaged run
 * on the named Android hardware.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const unreal = path.resolve(here, "..");
const root = path.resolve(unreal, "..");

const read = (relative) => fs.readFileSync(path.join(root, relative), "utf8");
const requireText = (content, token, label, failures) => {
  if (!content.includes(token)) failures.push(`${label}: missing ${token}`);
};

const failures = [];
const perfHeader = read("client-unreal/Source/Rok2/Public/Rok2Perf.h");
const budgetDoc = read("docs/P7_T6_WORLD_PERFORMANCE_BUDGET.md");
const perfSource = read("client-unreal/Source/Rok2/Private/Rok2Perf.cpp");
const rendererHeader = read("client-unreal/Source/Rok2/Public/Rok2WorldRenderer.h");
const rendererSource = read("client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp");
const engineConfig = read("client-unreal/Config/DefaultEngine.ini");
const packageJson = read("backend/package.json");

for (const [content, token, label] of [
  [perfHeader, "RecordWorldFrame", "Rok2Perf header"],
  [perfHeader, "GetWorldFrameAverageMs", "Rok2Perf header"],
  [perfHeader, "GetWorldFramePeakMs", "Rok2Perf header"],
  [perfSource, "WorldFrameAverageMs +=", "Rok2Perf source"],
  [perfSource, "ResetWorldFrameTelemetry", "Rok2Perf source"],
  [rendererHeader, "FRok2WorldPerfSnapshot", "WorldRenderer header"],
  [rendererHeader, "GetPerformanceSnapshot", "WorldRenderer header"],
  [rendererHeader, "ResetPerformanceSnapshot", "WorldRenderer header"],
  [rendererSource, "Perf->RecordWorldFrame(DeltaSeconds)", "WorldRenderer source"],
  [rendererSource, "CityHISM->GetInstanceCount()", "WorldRenderer source"],
  [rendererSource, "Perf->PoolSize()", "WorldRenderer source"],
  [rendererSource, "UHierarchicalInstancedStaticMeshComponent", "WorldRenderer source"],
  [rendererSource, "AcquireMarkerActor", "WorldRenderer source"],
  [engineConfig, "[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]", "DefaultEngine.ini"],
  [engineConfig, "bSupportsVulkan=True", "DefaultEngine.ini"],
  [engineConfig, "r.Nanite.ProjectEnabled=False", "DefaultEngine.ini"],
  [engineConfig, "r.MobileContentScaleFactor=1.0", "DefaultEngine.ini"],
  [packageJson, '"test:world-performance-budget"', "backend/package.json"],
  [budgetDoc, 'Rok2Main', "المشهد القياسي لا يحدد خريطة Rok2Main"],
  [budgetDoc, 'Tactical', "المشهد القياسي لا يحدد طبقة Tactical"],
  [budgetDoc, 'Regional', "المشهد القياسي لا يحدد طبقة Regional"],
  [budgetDoc, 'Kingdom', "المشهد القياسي لا يحدد طبقة Kingdom"],
  [budgetDoc, 'GetPerformanceSnapshot', "القياس لا يشير إلى لقطة GetPerformanceSnapshot"],
  [budgetDoc, 'stat unit', "إجراءات القياس لا تشمل stat unit"],
  [budgetDoc, 'stat memory', "إجراءات القياس لا تشمل stat memory"],
  [budgetDoc, '30 FPS', "الميزانية لا تحدد هدف 30 FPS على الجهاز المستهدف"],
]) requireText(content, token, label, failures);

if (failures.length) {
  console.error("P7-T6 world performance budget verification failed:");
  for (const failure of failures) console.error(`- ${failure}`);
  process.exit(1);
}

console.log("P7-T6 world performance budget verification passed (26 contracts: telemetry + snapshot + engine settings + benchmark-scene doc + budget targets).");
