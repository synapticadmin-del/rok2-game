// P12-T6 client structural guard — يتحقق من تكامل عميل UE5 مع P12 نهاية الموسم/إعادة الضبط:
// 1) 3 endpoints P12 في router.ts (season/report + admin/season-end + admin/season-reset)
// 2) أنواع USTRUCT في Rok2Types.h (SeasonReport + Legacy)
// 3) Declarations + delegates في Rok2Api.h
// 4) تنفيذات في Rok2Api.cpp
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../../..");
const routerSrc = fs.readFileSync(`${ROOT}/game/backend/src/http/router.ts`, "utf8");
const typesSrc = fs.readFileSync(`${ROOT}/game/client-unreal/Source/Rok2/Public/Rok2Types.h`, "utf8");
const apiHSrc = fs.readFileSync(`${ROOT}/game/client-unreal/Source/Rok2/Public/Rok2Api.h`, "utf8");
const apiCppSrc = fs.readFileSync(`${ROOT}/game/client-unreal/Source/Rok2/Private/Rok2Api.cpp`, "utf8");
let fails = 0, checks = 0;
function check(name, cond) { checks++; if (!cond) { console.log("FAIL:", name); fails++; } else { console.log("PASS:", name); } }

// ---- endpoints في router.ts ----
check("endpoint /v1/season/report", routerSrc.includes('"/v1/season/report"'));
check("endpoint /v1/admin/season-end", routerSrc.includes('"/v1/admin/season-end"'));
check("endpoint /v1/admin/season-reset", routerSrc.includes('"/v1/admin/season-reset"'));

// ---- أنواع USTRUCT في Rok2Types.h ----
check("USTRUCT FRok2SeasonReport", typesSrc.includes("struct FRok2SeasonReport"));
check("USTRUCT FRok2SeasonLeaderboardEntry", typesSrc.includes("struct FRok2SeasonLeaderboardEntry"));
check("USTRUCT FRok2LegacyPoints", typesSrc.includes("struct FRok2LegacyPoints"));
check("USTRUCT FRok2SeasonState", typesSrc.includes("struct FRok2SeasonState"));

// ---- Declarations + delegates في Rok2Api.h ----
check("delegate OnSeasonReportUpdated", apiHSrc.includes("OnSeasonReportUpdated"));
check("delegate OnSeasonEnded", apiHSrc.includes("OnSeasonEnded"));
check("declaration FetchSeasonReport", apiHSrc.includes("FetchSeasonReport"));
check("declaration GetSeasonState", apiHSrc.includes("GetSeasonState"));
check("cache SeasonReport", apiHSrc.includes("SeasonReport") || apiHSrc.includes("seasonReport"));
check("ParseSeasonReport", apiHSrc.includes("ParseSeasonReport"));

// ---- تنفيذات في Rok2Api.cpp ----
check("cpp season-report Get", /Get\s*\(\s*TEXT\s*\(\s*["']v1\/season\/report["']/.test(apiCppSrc));
check("cpp ParseSeasonReport impl", apiCppSrc.includes("ParseSeasonReport"));
check("cpp OnSeasonReportUpdated broadcast", apiCppSrc.includes("OnSeasonReportUpdated.Broadcast"));
check("cpp seasonState cache", apiCppSrc.includes("SeasonState") || apiCppSrc.includes("seasonState"));

if (fails > 0) { console.log(`\nFAILED ASSERTIONS: ${fails}`); process.exit(1); }
console.log(`\nALL PASSED: ${checks} checks, ${fails} failed`);
console.log("CHECK-PASS: P12 season-end/reset client (3 endpoints, 4 types, delegates, executions)");
process.exit(0);
