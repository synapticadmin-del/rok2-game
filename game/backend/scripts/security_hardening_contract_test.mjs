import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "../..");
const read = (...parts) => readFileSync(join(root, ...parts), "utf8");

const shard = read("backend/src/do/KingdomShard.ts");
const router = read("backend/src/http/router.ts");
const layout = read("client-unreal/Source/Rok2/Private/Rok2CityLayoutActor.cpp");
const api = read("client-unreal/Source/Rok2/Private/Rok2Api.cpp");
const cityLayout = read("backend/src/lib/cityLayout.ts");

let failed = 0;
function assert(condition, message) {
  if (condition) console.log("OK  :", message);
  else {
    failed += 1;
    console.error("FAIL:", message);
  }
}

function routeHasAuthenticatedBody(route, field) {
  const marker = `path.endsWith("/${route}")`;
  const start = shard.indexOf(marker);
  if (start < 0) return false;
  const next = shard.indexOf("path.endsWith(", start + marker.length);
  const section = shard.slice(start, next < 0 ? undefined : next);
  return section.includes(`this.requireAuthenticatedPlayer(request, body.${field})`)
    && section.includes("if (identityError) return identityError;");
}

// لا تثق الـ Durable Object في playerId القادم من الجسم، حتى لو تجاوز عميل الراوتر.
for (const [route, field] of [
  ["upsert-city", "playerId"],
  ["march", "playerId"],
  ["queue/add", "playerId"],
  ["queue/speedup", "playerId"],
  ["scout", "ownerPlayerId"],
]) {
  assert(routeHasAuthenticatedBody(route, field), `${route} compares body ${field} to authenticated player identity`);
}
assert(shard.includes("q.playerId !== body.playerId"), "queue speedup enforces queue ownership after identity validation");
assert(router.includes('"x-rok2-player": playerId'), "router forwards the trusted player identity to the Durable Object");

// أدوات الموسم ليست نقطة عامة: الشارد يعيد التحقق من مفتاح الإدارة، وليس الراوتر وحده.
const adminStart = shard.indexOf('path.endsWith("/admin")');
const adminSection = shard.slice(adminStart, shard.indexOf('path.endsWith("/queue/list")', adminStart));
assert(adminStart >= 0 && adminSection.includes("assertAdminKey(request, this.env)"), "admin tick and set_day require server-side admin-key validation");
assert(adminSection.includes('error: "admin_unauthorized"'), "unauthorized admin request is rejected explicitly");

// تحديث tick لا يستبدل صف world_meta، ويحافظ بذلك على season_start_ms عبر إعادة التشغيل.
assert(/ON CONFLICT\(id\) DO UPDATE SET\s+season_day=excluded\.season_day,\s+last_tick_ms=excluded\.last_tick_ms/.test(shard), "season tick updates world_meta without replacing season_start_ms");
assert(!/INSERT OR REPLACE INTO world_meta\s*\(id, season_day, last_tick_ms\)\s*VALUES/.test(shard), "no three-column OR REPLACE can reset season_start_ms");

// رسائل التحالف تحفظ تحالف المرسل وتصل حصراً إلى أعضاء ذلك التحالف، بما في ذلك التاريخ بعد إعادة التشغيل.
assert(shard.includes("allianceId: r.alliance_id || null"), "chat history reloads persisted alliance identity");
assert(shard.includes("alliance_id, player_id, text, created_at"), "chat persistence stores alliance identity with each message");
assert(shard.includes("this.broadcastChat(chatMsg);"), "alliance chat uses the recipient-filtered chat broadcaster");
assert(shard.includes("m.allianceId === allianceId"), "alliance chat history is visible to all current members, not only the sender");

// التقرير مرئي للطرفين وتحالفاتهما، ولا يستخدم البث العام.
assert(shard.includes("report.attackerPlayerId === playerId || report.defenderPlayerId === playerId"), "battle report is visible to both attacker and defender");
assert(shard.includes("report.defenderAllianceId === allianceId"), "battle report is visible to the defender alliance");
assert(shard.includes("private broadcastReport(report: any)"), "battle reports use a recipient-filtered broadcaster");

// التحسينات المحددة لا تغير قواعد اللعب: لقطة tick واحدة، ومجموعة ملكية المباني.
assert(shard.includes("this.seedEventBarbarians(extraBarbs, tickInDay)"), "event seeding reuses the tick's season-time snapshot");
assert(shard.includes("this.resolveMarchArrival(m, now, tickInDay)"), "march arrival reuses the tick's season-time snapshot");
assert(shard.includes("const participants = await Promise.all(contributions.map(async"), "rally settlement processes independent participants concurrently while preserving input order");
assert(shard.includes("const [hospital] = await Promise.all(["), "hospital admission and marching-loss deduction run concurrently for each player");
assert(shard.includes("await Promise.all(Object.entries(admitted).map"), "hospital unit writes avoid serial D1 round trips");
assert(shard.includes("await Promise.all(Object.entries(losses)"), "marching-loss writes avoid serial D1 round trips");
assert(cityLayout.includes("const ownedIdSet = new Set(ownedIds)"), "city-layout ownership lookup uses Set instead of repeated array includes");

// العميل لا يضاعف delegates ولا يثبت نسخة محلية قبل موافقة الخادم.
assert(layout.includes("OnClicked.RemoveDynamic(this, &ARok2CityLayoutActor::OnAnyBuildingClicked)"), "recycled buildings remove prior click binding");
assert(layout.includes("OnClicked.AddDynamic(this, &ARok2CityLayoutActor::OnAnyBuildingClicked)"), "recycled buildings restore exactly one click binding");
assert(layout.includes("if (bAccepted && WeakThis.IsValid())") && layout.includes("WeakThis->SaveAcceptedLayoutLocally(Placements);"), "city layout persists locally only after authoritative acceptance");
assert(!/SaveLayoutToServer[\s\S]{0,1200}SaveGameToSlot/.test(layout), "layout save request does not write a local slot before server callback");

// كل callback HTTP طويل العمر يعتمد مرجعاً ضعيفاً؛ تبقى lambdas الحسابية المحلية فقط مرتبطة بـ this.
assert(api.includes("TWeakObjectPtr<URok2Api> WeakThis(this)"), "API creates weak object references for asynchronous responses");
const directThisCaptures = [...api.matchAll(/\[this\]/g)];
assert(directThisCaptures.length === 2, "only the two synchronous local calculation lambdas capture URok2Api directly");
assert(api.includes("Post(FString::Printf(TEXT(\"/v1/world/march/%s/redirect\")") && api.includes("[WeakThis](const FString& Err)"), "march redirect success and failure callbacks use weak references");

console.log("\n==== RESULT ====");
if (failed === 0) {
  console.log("ALL SECURITY HARDENING CONTRACT CHECKS PASSED");
  process.exit(0);
}
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
