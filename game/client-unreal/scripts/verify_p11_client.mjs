// P11-T6 client structural guard — يتحقق من تكامل عميل UE5 مع P11 Lost Kingdom/KvK:
// 1) 6 endpoints P11 في router.ts
// 2) أنواع USTRUCT في Rok2Types.h (حالة KvK + هجرة + منشآت + زيقورة)
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

// ---- endpoints في router ----
check("endpoint /v1/lk/state", routerSrc.includes('"/v1/lk/state"'));
check("endpoint /v1/lk/migrate", routerSrc.includes('"/v1/lk/migrate"'));
check("endpoint /v1/lk/hieron", routerSrc.includes('"/v1/lk/hieron"'));
check("endpoint /v1/lk/citadel", routerSrc.includes('"/v1/lk/citadel"'));
check("endpoint /v1/lk/ziggurat", routerSrc.includes('"/v1/lk/ziggurat"'));
check("endpoint /v1/lk/season-buy", routerSrc.includes('"/v1/lk/season-buy"'));

// ---- أنواع USTRUCT في Rok2Types.h ----
check("USTRUCT FRok2LostKingdomState", typesSrc.includes("struct FRok2LostKingdomState"));
check("USTRUCT FRok2LKStructure", typesSrc.includes("struct FRok2LKStructure"));
check("USTRUCT FRok2LKCitadel", typesSrc.includes("struct FRok2LKCitadel"));
check("USTRUCT FRok2LKZiggurat", typesSrc.includes("struct FRok2LKZiggurat"));
check("USTRUCT FRok2LKMigration", typesSrc.includes("struct FRok2LKMigration"));
check("USTRUCT FRok2LKSeasonStoreItem", typesSrc.includes("struct FRok2LKSeasonStoreItem"));

// ---- Declarations + delegates في Rok2Api.h ----
check("delegate OnLostKingdomUpdated", apiHSrc.includes("OnLostKingdomUpdated"));
check("declaration FetchLostKingdomState", apiHSrc.includes("FetchLostKingdomState"));
check("declaration MigrateToLostKingdom", apiHSrc.includes("MigrateToLostKingdom"));
check("declaration CaptureHieron", apiHSrc.includes("CaptureHieron"));
check("declaration AttackCitadel", apiHSrc.includes("AttackCitadel"));
check("declaration AttackZiggurat", apiHSrc.includes("AttackZiggurat"));
check("declaration BuySeasonItem", apiHSrc.includes("BuySeasonItem"));
check("cache LostKingdomState", apiHSrc.includes("LostKingdomState") || apiHSrc.includes("lostKingdomState"));
check("ParseLostKingdomState", apiHSrc.includes("ParseLostKingdomState"));

// ---- تنفيذات في Rok2Api.cpp ----
check("cpp lk-state Get", /Get\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/state["']/.test(apiCppSrc));
check("cpp lk-migrate Post", /Post\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/migrate["']/.test(apiCppSrc));
check("cpp lk-hieron Post", /Post\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/hieron["']/.test(apiCppSrc));
check("cpp lk-citadel Post", /Post\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/citadel["']/.test(apiCppSrc));
check("cpp lk-ziggurat Post", /Post\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/ziggurat["']/.test(apiCppSrc));
check("cpp lk-season-buy Post", /Post\s*\(\s*TEXT\s*\(\s*["']v1\/lk\/season-buy["']/.test(apiCppSrc));
check("cpp ParseLostKingdomState", apiCppSrc.includes("ParseLostKingdomState"));
check("cpp OnLostKingdomUpdated broadcast", apiCppSrc.includes("OnLostKingdomUpdated.Broadcast"));

if (fails > 0) { console.log(`\nFAILED ASSERTIONS: ${fails}`); process.exit(1); }
console.log(`\nALL PASSED: ${checks} checks, ${fails} failed`);
console.log("CHECK-PASS: P11 Lost Kingdom client (6 endpoints, 6 types, delegates, executions)");
process.exit(0);
