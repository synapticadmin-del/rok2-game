#!/usr/bin/env node
/** P10-T7: Tavern/chest asset and client wiring guard. */
import fs from "node:fs";
import path from "node:path";

const ROOT = path.resolve(import.meta.dirname, "..", "..", "..");
const CLIENT = path.join(ROOT, "game", "client-unreal");
const BACKEND = path.join(ROOT, "game", "backend");
let passed = 0;
let failed = 0;
function check(label, condition) {
  if (condition) { console.log(`PASS: ${label}`); passed += 1; }
  else { console.error(`FAIL: ${label}`); failed += 1; }
}
function read(file) {
  const absolute = path.join(ROOT, file);
  return fs.existsSync(absolute) ? fs.readFileSync(absolute, "utf8") : "";
}

const tavernData = read("game/backend/src/data/tavern.json");
const gameMode = read("game/client-unreal/Source/Rok2/Private/Rok2GameMode.cpp");
const detailWidget = read("game/client-unreal/Source/Rok2/Private/Rok2BuildingDetailWidget.cpp");
const iconLibrary = read("game/client-unreal/Source/Rok2/Private/Rok2IconLibrary.cpp");

check("tavern data exists", tavernData.length > 0);
let parsed = null;
try { parsed = JSON.parse(tavernData); } catch { /* reported below */ }
check("tavern data is valid JSON", parsed !== null);
check("tavern data defines boxes and opening rates", Array.isArray(parsed?.boxes) && parsed.boxes.length >= 3 && parsed.boxes.every((box) => box.id && box.key && Array.isArray(box.pool)) && parsed.rateTargets && parsed.limits);
check("tavern building asset exists", fs.existsSync(path.join(CLIENT, "Content/Art/kaykit/building_tavern.glb")));
check("tavern event art exists", fs.existsSync(path.join(CLIENT, "Content/Art/Events/event_tavern.png")));
check("tavern icon is registered", iconLibrary.includes('TEXT("tavern")'));
check("building detail exposes tavern action", detailWidget.includes('BuildingId == TEXT("tavern")') && detailWidget.includes('TEXT("chests")'));
check("GameMode handles chests action", gameMode.includes('ActionKind == TEXT("chests")'));
check("GameMode emits honest chest feedback", gameMode.includes("EmitToast"));
check("procedural tavern generator exists", fs.existsSync(path.join(ROOT, "scripts/generate_tavern_assets.py")));

console.log(`P10-T7 tavern assets: ${passed} passed, ${failed} failed`);
process.exitCode = failed ? 1 : 0;
