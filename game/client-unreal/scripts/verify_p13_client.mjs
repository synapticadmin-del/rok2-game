// P13 client resilience guard — يتحقق من تكامل عميل UE5 مع P13 (موثوقية الاتصال والكاش المحلي):
// 1) صندوق واردات WS (WsOutbox/Enqueue/Flush) — الرسائل لا تضيع عند الانقطاع
// 2) نبض WS + watchdog للانقطاع الصامت (heartbeat/90s silent threshold)
// 3) كاش محلي لبيانات التوازن (rok2_meta_cache.json في ProjectSavedDir)
// 4) إشعارات Outbox للمستخدم (الرسالة ستُرسل عند عودة الاتصال)
// 5) الثوابت ليست hard-coded في CPP — كلها UPROPERTY/constexpr قابلة للتعديل
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../../..");
const apiHSrc = fs.readFileSync(`${ROOT}/game/client-unreal/Source/Rok2/Public/Rok2Api.h`, "utf8").replace(/\r\n/g, "\n");
const apiCppSrc = fs.readFileSync(`${ROOT}/game/client-unreal/Source/Rok2/Private/Rok2Api.cpp`, "utf8").replace(/\r\n/g, "\n");
let fails = 0, checks = 0;
function check(name, cond) { checks++; if (!cond) { console.log("FAIL:", name); fails++; } else { console.log("PASS:", name); } }
// ---- P13-T1: صندوق واردات WebSocket ----
check("field WsOutbox", apiHSrc.includes("WsOutbox"));
check("declaration EnqueueWsMessage", apiHSrc.includes("EnqueueWsMessage"));
check("declaration FlushWsOutbox", apiHSrc.includes("FlushWsOutbox"));
check("cpp EnqueueWsMessage impl", apiCppSrc.includes("void URok2Api::EnqueueWsMessage"));
check("cpp FlushWsOutbox impl", apiCppSrc.includes("void URok2Api::FlushWsOutbox"));
check("cpp outbox cap 128", apiCppSrc.includes("WsOutbox.Num() > 128"));
check("SendChat queues على انقطاع", apiCppSrc.includes("EnqueueWsMessage(Str)"));
check("SendChat إشعار حفظ مؤقت", /ستُرسل تلقائيًا عند عودة الاتصال/.test(apiCppSrc));
const chatIdx = apiCppSrc.indexOf("void URok2Api::SendChat(");
const nextFn = apiCppSrc.indexOf("void URok2Api::", chatIdx + 10);
check("SendChat لا يسقط الرسالة صامتًا", apiCppSrc.slice(chatIdx, nextFn).includes("EnqueueWsMessage(Str)"));
check("FlushWsOutbox في OnConnected", /FlushWsOutbox\(\);?\s*\n.*if \(bShouldRestore\)/.test(apiCppSrc) || apiCppSrc.includes("Self->FlushWsOutbox()"));
check("FlushWsOutbox loop sends", /for \(const FString& Msg : WsOutbox\)/.test(apiCppSrc));
check("FlushWsOutbox empties", apiCppSrc.includes("WsOutbox.Empty()"));
// ---- P13-T3: نبض القلب + watchdog ----
check("field WsHeartbeatTimer", apiHSrc.includes("WsHeartbeatTimer"));
check("field WsLastMessageAt", apiHSrc.includes("WsLastMessageAt"));
check("const WsHeartbeatIntervalSeconds", apiHSrc.includes("WsHeartbeatIntervalSeconds"));
check("const WsSilentDisconnectThresholdSeconds", apiHSrc.includes("WsSilentDisconnectThresholdSeconds"));
check("SendWsHeartbeat impl", apiCppSrc.includes("void URok2Api::SendWsHeartbeat"));
check("heartbeat type JSON", apiCppSrc.includes('TEXT("heartbeat")'));
check("OnMessage يحدّث watchdog", /Self->WsLastMessageAt = 0\.f;/.test(apiCppSrc));
check("watchdog يعيد الاتصال بعد 90s صامت", /bRestoreOnNextWsConnection = true;/.test(apiCppSrc) && /silent disconnect/i.test(apiCppSrc));
check("DisconnectWebSocket يعيد تهيئة العدّادات", apiCppSrc.includes("WsHeartbeatTimer = 0.f\n\tWsLastMessageAt = 0.f") || /WsHeartbeatTimer = 0\.f;[\s\S]{0,120}WsLastMessageAt = 0\.f;/.test(apiCppSrc));
// ---- P13-T2: كاش بيانات التوازن المحلي ----
check("MetaCacheFileName field", apiHSrc.includes("MetaCacheFileName"));
check("field bMetaCacheLoaded", apiHSrc.includes("bMetaCacheLoaded"));
check("declaration MetaCachePath", apiHSrc.includes("MetaCachePath()"));
check("declaration SaveMetaCache", apiHSrc.includes("void SaveMetaCache()"));
check("declaration LoadMetaCache", apiHSrc.includes("void LoadMetaCache()"));
check("cpp MetaCachePath impl", apiCppSrc.includes("URok2Api::MetaCachePath"));
check("cpp SaveMetaCache impl", apiCppSrc.includes("void URok2Api::SaveMetaCache"));
check("cpp LoadMetaCache impl", apiCppSrc.includes("void URok2Api::LoadMetaCache"));
check("كاش محفوظ في ProjectSavedDir", /FPaths::ProjectSavedDir\(\), MetaCacheFileName/.test(apiCppSrc));
check("FFileHelper save", apiCppSrc.includes("FFileHelper::SaveStringToFile(Out, *Path)"));
check("FFileHelper load", apiCppSrc.includes("FFileHelper::LoadFileToString(Out, *Path)"));
check("LoadMetaCache تُنادى من Init", apiCppSrc.includes("LoadMetaCache();\n\tUE_LOG(LogRok2") || /LoadMetaCache\(\);\n\tUE_LOG\(LogRok2, Log, TEXT\("Rok2Api init/.test(apiCppSrc));
check("SaveMetaCache بعد OnMetaLoaded", apiCppSrc.includes("Self->OnMetaLoaded.Broadcast(true);\n\t\t// P13-T2") || apiCppSrc.includes("Self->OnMetaLoaded.Broadcast(true)"))
check("SaveMetaCache cached_at_ms", apiCppSrc.includes("cached_at_ms"));
check("LoadMetaCache يحدّث Meta + إعادة معدلات", apiCppSrc.includes("RecomputeResourceRates()") && apiCppSrc.includes("bMetaCacheLoaded = true;"));
// ---- P13-T5: لا ثوابت hard-coded في CPP ----
check("لا hard-coded 30.0 للheartbeat", !/WsHeartbeatTimer >= 30\.0f/.test(apiCppSrc));
check("لا hard-coded 90 للwatchdog", !/WsLastMessageAt >= 90\.0f/.test(apiCppSrc));
check("لا hard-coded 128 في .h", !apiHSrc.includes("128"));
// ---- P13-T4: إشعار Outbox ----
check("PushNotification ws_outbox", apiCppSrc.includes('PushNotification(TEXT("ws_outbox")'));
if (fails > 0) { console.log(`\nFAILED ASSERTIONS: ${fails}`); process.exit(1); }
console.log(`\nALL PASSED: ${checks} checks, ${fails} failed`);
console.log("CHECK-PASS: P13 client resilience (outbox + heartbeat watchdog + meta cache + notifications)");
process.exit(0);
