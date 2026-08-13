// P10-T6: حارس تحقق بنيوي لمسار العميل C++ — لا Unreal في بيئة التنفيذ.
// يتحقق من أن الأنواع (Rok2Types.h) والتصريحات والتنفيذات (Rok2Api.h/cpp)
// لأوضاع اللعب المتكررة (P10) موجودة ومتطابقة مع endpoints الخادم في router.ts.
import { readFileSync } from "node:fs";
import { resolve } from "node:path";
const root = resolve(import.meta.dirname, "../../..");
const read = (relativePath) => readFileSync(resolve(root, relativePath), "utf8");
const requireMatch = (source, pattern, description) => {
  if (!pattern.test(source)) { console.error(`FAIL: ${description}`); process.exit(1); }
  console.log(`  PASS: ${description}`);
};
console.log("P10-T6 client structural guard");
const types = read("game/client-unreal/Source/Rok2/Public/Rok2Types.h");
const apiH = read("game/client-unreal/Source/Rok2/Public/Rok2Api.h");
const apiC = read("game/client-unreal/Source/Rok2/Private/Rok2Api.cpp");
const router = read("game/backend/src/http/router.ts");
// ---------------------------------------------------------------------------
// 1. endpoints الخادم موجودة في router.ts (21 endpoint لـ P10).
// ---------------------------------------------------------------------------
requireMatch(router, /\/v1\/tavern\/state/, "endpoint /v1/tavern/state");
requireMatch(router, /\/v1\/tavern\/open/, "endpoint /v1/tavern/open");
requireMatch(router, /\/v1\/tavern\/keys/, "endpoint /v1/tavern/keys");
requireMatch(router, /\/v1\/tavern\/daily-key/, "endpoint /v1/tavern/daily-key");
requireMatch(router, /\/v1\/expedition\/state/, "endpoint /v1/expedition/state");
requireMatch(router, /\/v1\/expedition\/battle/, "endpoint /v1/expedition/battle");
requireMatch(router, /\/v1\/expedition\/medal-buy/, "endpoint /v1/expedition/medal-buy");
requireMatch(router, /\/v1\/canyon\/state/, "endpoint /v1/canyon/state");
requireMatch(router, /\/v1\/canyon\/challenge/, "endpoint /v1/canyon/challenge");
requireMatch(router, /\/v1\/canyon\/complete/, "endpoint /v1/canyon/complete");
requireMatch(router, /\/v1\/canyon\/buff/, "endpoint /v1/canyon/buff");
requireMatch(router, /\/v1\/canyon\/token-buy/, "endpoint /v1/canyon/token-buy");
requireMatch(router, /\/v1\/canyon\/season/, "endpoint /v1/canyon/season");
requireMatch(router, /\/v1\/osiris\/register/, "endpoint /v1/osiris/register");
requireMatch(router, /\/v1\/osiris\/attack/, "endpoint /v1/osiris/attack");
requireMatch(router, /\/v1\/osiris\/move-ark/, "endpoint /v1/osiris/move-ark");
requireMatch(router, /\/v1\/osiris\/league-result/, "endpoint /v1/osiris/league-result");
requireMatch(router, /\/v1\/events\/state/, "endpoint /v1/events/state");
requireMatch(router, /\/v1\/events\/wheel-window/, "endpoint /v1/events/wheel-window");
requireMatch(router, /\/v1\/events\/mg-score/, "endpoint /v1/events/mg-score");
requireMatch(router, /\/v1\/events\/mg-leaderboard/, "endpoint /v1/events/mg-leaderboard");
requireMatch(router, /\/v1\/events\/wheel-spin/, "endpoint /v1/events/wheel-spin");
// ---------------------------------------------------------------------------
// 2. الأنواع في Rok2Types.h — مرآة للباک اند.
// ---------------------------------------------------------------------------
requireMatch(types, /FRok2TavernState/, "FRok2TavernState حالة الحانة");
requireMatch(types, /FRok2TavernRoll/, "FRok2TavernRoll نتيجة فتح صندوق الحانة");
requireMatch(types, /FRok2ExpeditionState/, "FRok2ExpeditionState حالة Expedition");
requireMatch(types, /FRok2ExpeditionBattleResult/, "FRok2ExpeditionBattleResult نتيجة معركة Expedition");
requireMatch(types, /FRok2CanyonState/, "FRok2CanyonState حالة Sunset Canyon");
requireMatch(types, /FRok2CanyonChallenge/, "FRok2CanyonChallenge تحدي Canyon");
requireMatch(types, /FRok2OsirisState/, "FRok2OsirisState حالة Ark of Osiris");
requireMatch(types, /FRok2OsirisFacility/, "FRok2OsirisFacility منشأة Osiris");
requireMatch(types, /FRok2EventsState/, "FRok2EventsState حالة الأحداث الكبرى");
requireMatch(types, /FRok2WheelSpinResult/, "FRok2WheelSpinResult نتيجة دوران العجلة");
// ---------------------------------------------------------------------------
// 3. Rok2Api.h — declarations لوظائف P10 مع UFUNCTION.
// ---------------------------------------------------------------------------
requireMatch(apiH, /void FetchTavernState\(\)/, "FetchTavernState (GET tavern/state)");
requireMatch(apiH, /void OpenTavernBox\(/, "OpenTavernBox (POST tavern/open)");
requireMatch(apiH, /void FetchExpeditionState\(\)/, "FetchExpeditionState (GET expedition/state)");
requireMatch(apiH, /void AttemptExpeditionBattle\(/, "AttemptExpeditionBattle (POST expedition/battle)");
requireMatch(apiH, /void FetchCanyonState\(\)/, "FetchCanyonState (GET canyon/state)");
requireMatch(apiH, /void CreateCanyonChallenge\(\)/, "CreateCanyonChallenge (POST canyon/challenge)");
requireMatch(apiH, /void CompleteCanyonChallenge\(/, "CompleteCanyonChallenge (POST canyon/complete)");
requireMatch(apiH, /void ActivateCanyonBuff\(/, "ActivateCanyonBuff (POST canyon/buff)");
requireMatch(apiH, /void FetchOsirisState\(\)/, "FetchOsirisState (GET osiris/state)");
requireMatch(apiH, /void RegisterOsiris\(/, "RegisterOsiris (POST osiris/register)");
requireMatch(apiH, /void AttackOsirisFacility\(/, "AttackOsirisFacility (POST osiris/attack)");
requireMatch(apiH, /void MoveOsirisArk\(/, "MoveOsirisArk (POST osiris/move-ark)");
requireMatch(apiH, /void FetchEventsState\(\)/, "FetchEventsState (GET events/state)");
requireMatch(apiH, /void SpinWheel\(/, "SpinWheel (POST events/wheel-spin)");
requireMatch(apiH, /void SubmitMGScore\(/, "SubmitMGScore (POST events/mg-score)");
// ---------------------------------------------------------------------------
// 4. التنفيذات في Rok2Api.cpp — Post/Get مع مسارات مطابقة للراوتر.
// ---------------------------------------------------------------------------
requireMatch(apiC, /"v1\/tavern\/state"/, "تنفيذ FetchTavernState → tavern/state");
requireMatch(apiC, /"v1\/tavern\/open"/, "تنفيذ OpenTavernBox → tavern/open");
requireMatch(apiC, /"v1\/expedition\/battle"/, "تنفيذ AttemptExpeditionBattle → expedition/battle");
requireMatch(apiC, /"v1\/canyon\/challenge"/, "تنفيذ CreateCanyonChallenge → canyon/challenge");
requireMatch(apiC, /"v1\/canyon\/complete"/, "تنفيذ CompleteCanyonChallenge → canyon/complete");
requireMatch(apiC, /"v1\/canyon\/buff"/, "تنفيذ ActivateCanyonBuff → canyon/buff");
requireMatch(apiC, /"v1\/osiris\/register"/, "تنفيذ RegisterOsiris → osiris/register");
requireMatch(apiC, /"v1\/osiris\/attack"/, "تنفيذ AttackOsirisFacility → osiris/attack");
requireMatch(apiC, /"v1\/osiris\/move-ark"/, "تنفيذ MoveOsirisArk → osiris/move-ark");
requireMatch(apiC, /"v1\/events\/wheel-spin"/, "تنفيذ SpinWheel → events/wheel-spin");
requireMatch(apiC, /"v1\/events\/mg-score"/, "تنفيذ SubmitMGScore → events/mg-score");
// ---------------------------------------------------------------------------
console.log("CHECK-PASS: P10-T6 client paths (21 endpoints من router.ts)");
