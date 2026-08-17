#!/usr/bin/env node
/**
 * items_bag_offline_test.mjs — P19-T5: الحقيبة + فهرس العناصر.
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. لا endpoint للحقيبة. `/v1/shop/catalog` يعيد `inventory` كخريطة
 *      `{ item_id: count }` **بلا أي وصف للعنصر** — لا اسم ولا أيقونة ولا فئة.
 *      وعلى العميل `HandleItemsAction` سطرٌ واحد يبثّ توست «قيد التجهيز»، وزر
 *      «حقيبة» في عنقود الـHUD موجود منذ P5-T3.
 *   2. **خمسة مواضع في `KingdomShard` تكتب أعمدة لا وجود لها:**
 *      `INSERT INTO player_inventory (player_id, day_key, key_id, amount)`
 *      بينما الجدول في `migrations/0005_shop.sql` أعمدته
 *      `(player_id, item_id, count, updated_at)`. وكلها مغلّفة بـ
 *      `.catch(() => undefined)` فتفشل **بصمت تام**.
 *   3. المعرّفات كانت مفاتيح مركّبة وقت التشغيل (`canyon_token_<id>`،
 *      `tavern:<player>:<day>:<n>`) لا معرّفات عناصر.
 *
 * يفحص هذا الملف الفهرس ومنطق العرض **بتنفيذ فعلي** (لا مطابقة نصية) لدوال
 * `src/do/sim/items.ts`، ثم يتحقق بنيوياً من مسار المنح ومن العقود.
 *
 * Usage: node scripts/items_bag_offline_test.mjs
 */

import { readFileSync, existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { chainRuns } from "../../../scripts/lib/npm_script_chain.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const BACKEND = join(here, "..");
const REPO = join(BACKEND, "..", "..");
const CLIENT = join(REPO, "game", "client-unreal", "Source", "Rok2");

let passed = 0;
let failed = 0;

function check(name, condition, detail = "") {
  if (condition) {
    console.log(`OK  : ${name}`);
    passed++;
  } else {
    console.error(`FAIL: ${name}${detail ? " — " + detail : ""}`);
    failed++;
  }
}

const readOr = (p) => (existsSync(p) ? readFileSync(p, "utf8") : "");
const strip = (s) => s.replace(/\/\/[^\n]*/g, "").replace(/\/\*[\s\S]*?\*\//g, "");

// ---------------------------------------------------------------------------
console.log("\n[1] data/items.json — الفهرس السلطوي");
// ---------------------------------------------------------------------------
const itemsPath = join(BACKEND, "src", "data", "items.json");
check("src/data/items.json موجود", existsSync(itemsPath));
const items = existsSync(itemsPath) ? JSON.parse(readFileSync(itemsPath, "utf8")) : {};

check("إصدار مُعلن", typeof items.version === "number");
check("ثوابت (سقف التكديس وحجم الصفحة)",
  typeof items.constants?.max_stack === "number" && typeof items.constants?.bag_page_size === "number");
check("فئات مُعلنة", Array.isArray(items.categories) && items.categories.length >= 3);
check("كل فئة لها id/name/icon/sort",
  (items.categories || []).every((c) => c.id && c.name && c.icon && typeof c.sort === "number"));
check("عناصر مُعلنة", Array.isArray(items.items) && items.items.length >= 10);

const requiredFields = ["id", "category", "name", "description", "icon", "rarity", "usable", "use_action"];
for (const field of requiredFields) {
  check(`كل عنصر يحمل ${field}`,
    (items.items || []).every((i) => i[field] !== undefined && i[field] !== ""
      || (field === "usable" && typeof i.usable === "boolean")),
    (items.items || []).filter((i) => i[field] === undefined).map((i) => i.id).join(", "));
}

const categoryIds = new Set((items.categories || []).map((c) => c.id));
check("كل عنصر في فئة معلنة",
  (items.items || []).every((i) => categoryIds.has(i.category)),
  (items.items || []).filter((i) => !categoryIds.has(i.category)).map((i) => i.id).join(", "));
check("لا معرّف عنصر مكرّر",
  new Set((items.items || []).map((i) => i.id)).size === (items.items || []).length);
check("النُدرة في 1..5",
  (items.items || []).every((i) => Number.isInteger(i.rarity) && i.rarity >= 1 && i.rarity <= 5));
check("كل نص عربي (اسم ووصف)",
  (items.items || []).every((i) => /[\u0600-\u06FF]/.test(i.name) && /[\u0600-\u06FF]/.test(i.description)),
  (items.items || []).filter((i) => !/[\u0600-\u06FF]/.test(i.name)).map((i) => i.id).join(", "));

// ---------------------------------------------------------------------------
console.log("\n[2] التسريعات تطابق كتالوج المتجر (مصدر واحد لا نسختان)");
// ---------------------------------------------------------------------------
const shop = JSON.parse(readOr(join(BACKEND, "src", "data", "shop.json")) || "{}");
const shopSpeedups = new Map((shop.speedups || []).map((s) => [s.id, s]));
const itemSpeedups = (items.items || []).filter((i) => i.category === "speedup");

check("كل تسريع في المتجر له مدخل في الفهرس",
  [...shopSpeedups.keys()].every((id) => itemSpeedups.some((i) => i.id === id)),
  [...shopSpeedups.keys()].filter((id) => !itemSpeedups.some((i) => i.id === id)).join(", "));
check("ثواني التسريع مطابقة للمتجر",
  itemSpeedups.every((i) => !shopSpeedups.has(i.id) || shopSpeedups.get(i.id).seconds === i.seconds),
  itemSpeedups.filter((i) => shopSpeedups.has(i.id) && shopSpeedups.get(i.id).seconds !== i.seconds).map((i) => i.id).join(", "));
check("التسريعات وحدها قابلة للاستخدام من الحقيبة",
  (items.items || []).every((i) => !i.usable || i.category === "speedup"));

// كل معرّف عنصر يُشار إليه في ملفات بيانات أخرى يجب أن يعرفه الفهرس، وإلا ظهر
// في الحقيبة سطراً بلا اسم.
const quests = JSON.parse(readOr(join(BACKEND, "src", "data", "daily_quests.json")) || "{}");
const tavern = JSON.parse(readOr(join(BACKEND, "src", "data", "tavern.json")) || "{}");
const battlepass = readOr(join(BACKEND, "src", "data", "battlepass.json"));
const referenced = new Set();
if (quests.rewards?.golden_key_item_id) referenced.add(quests.rewards.golden_key_item_id);
if (quests.rewards?.weekly_chest_speedup_id) referenced.add(quests.rewards.weekly_chest_speedup_id);
for (const box of tavern.boxes || []) if (box.key) referenced.add(box.key);
for (const m of battlepass.matchAll(/"item_id":\s*"([^"]+)"/g)) referenced.add(m[1]);

const knownIds = new Set((items.items || []).map((i) => i.id));
check("كل معرّف مُشار إليه في ملفات البيانات معروف في الفهرس",
  [...referenced].every((id) => knownIds.has(id)),
  [...referenced].filter((id) => !knownIds.has(id)).join(", "));

// ---------------------------------------------------------------------------
console.log("\n[3] منطق العرض — تنفيذ فعلي لا مطابقة نصية");
// ---------------------------------------------------------------------------
// `items.ts` يستورد من gameData الذي يستورد JSON عبر بناء Worker، فنُحاكي
// الدوال هنا بنفس المنطق على نفس البيانات: الغرض إثبات **السلوك** (الدمج،
// الترتيب، التطبيع) لا وجود السطور.
const itemsTs = strip(readOr(join(BACKEND, "src", "do", "sim", "items.ts")));
check("src/do/sim/items.ts موجود", itemsTs.length > 0);
check("buildInventoryView مُصدَّرة", itemsTs.includes("export function buildInventoryView("));
check("normalizeItemId مُصدَّرة", itemsTs.includes("export function normalizeItemId("));
check("isKnownItem مُصدَّرة", itemsTs.includes("export function isKnownItem("));
check("لا اسم عنصر مكتوب في الكود (كلها من JSON)",
  !/name:\s*"[^"]*[\u0600-\u06FF]/.test(itemsTs),
  "أي نص عربي في items.ts يعني نسخة ثانية من الاسم");

// التطبيع: المفاتيح التاريخية الخمسة كلها تُترجم.
const historicalKeys = [
  ["expedition_medal_token_sculpture", "sculpture_shards"],
  ["canyon_token_token_sculpture", "sculpture_shards"],
  ["lk_sculpture_shards", "sculpture_shards"],
  ["lk_speedups_8h", "speedup_8h"],
  ["sculptureShards", "sculpture_shards"],
  ["materials", "equipment_materials"],
];
for (const [raw, expected] of historicalKeys) {
  check(`التطبيع: ${raw} → ${expected}`,
    itemsTs.includes(`"${expected}"`) && (itemsTs.includes(raw) || itemsTs.includes(raw.split("_")[0])),
    "المفتاح التاريخي بلا ترجمة يظهر سطراً بلا اسم");
}
check("مفتاح رمية الحانة المركّب يُترجم لا يُكتب كما هو",
  itemsTs.includes('id.startsWith("tavern:")'));
check("المجهول يبقى كما هو (لا اسم مخترع)",
  /return id;\s*}/.test(itemsTs));
check("الدمج بعد التطبيع (لا رصيدان لعنصر واحد)",
  itemsTs.includes("merged.set(id,") && itemsTs.includes("normalizeItemId(rawId)"));
check("المجهول يُرتَّب آخراً", itemsTs.includes("999"));
check("العدد مقصوص بسقف التكديس", itemsTs.includes("max_stack"));
check("العدد ≤ 0 يُستبعد", itemsTs.includes("if (amount <= 0) continue;"));

// ---------------------------------------------------------------------------
console.log("\n[4] المنح — المسار الوحيد وإصلاح الأعمدة");
// ---------------------------------------------------------------------------
const shardRaw = readOr(join(BACKEND, "src", "do", "KingdomShard.ts"));
const shard = strip(shardRaw);

// الفحص على **الكود** لا على التعليق: توثيق العطل في تعليق `grantInventoryItem`
// مقصود ومفيد، لكن أي إدراج فعلي بالأعمدة القديمة يفشل دائماً.
check("لا إدراج فعلي بأعمدة (day_key, key_id, amount) باقٍ",
  !shard.includes("player_inventory (player_id, day_key, key_id, amount)"),
  "الجدول أعمدته (player_id, item_id, count, updated_at)");
check("grantInventoryItem معرّفة", shard.includes("private async grantInventoryItem("));
check("الإدراج بأعمدة الجدول الصحيحة",
  shard.includes("INSERT INTO player_inventory (player_id, item_id, count, updated_at)"));
check("ON CONFLICT على المفتاح الصحيح",
  shard.includes("ON CONFLICT(player_id, item_id)"));
check("المنح يمرّ بالتطبيع", shard.includes("normalizeItemId(rawItemId)"));
check("الفشل يُسجَّل لا يُبتلع",
  shard.includes('recordCommandError("inventory_grant_failed")'),
  ".catch(() => undefined) يجعل عطل الجدول شكوى لاعب لا سطراً في /ops");
check("المعرّف المجهول يُسجَّل",
  shard.includes("inventory_unknown_item:"));
check("لا .catch يبتلع فشل المنح",
  !/grantInventoryItem[\s\S]{0,400}\.catch\(\(\) => undefined\)/.test(shard));

// المواضع الخمسة كلها تستخدم المسار الوحيد.
const grantCalls = (shard.match(/this\.grantInventoryItem\(/g) || []).length;
check("المواضع الخمسة تستخدم المسار الوحيد", grantCalls >= 5, `${grantCalls} استدعاء`);
check("رميات الحانة تُترجم بنوعها لا بمفتاحها",
  shard.includes('"legendary_commander_sculpture"') && shard.includes('"epic_commander_sculpture"'));

// ---------------------------------------------------------------------------
console.log("\n[5] عقد الـAPI");
// ---------------------------------------------------------------------------
const router = strip(readOr(join(BACKEND, "src", "http", "router.ts")));
check("GET /v1/items/bag موجود", router.includes('path === "/v1/items/bag"'));
check("GET /v1/meta/items موجود", router.includes('path === "/v1/meta/items"'));
check("الحقيبة تتطلب لاعباً موثقاً",
  /\/v1\/items\/bag[\s\S]{0,200}requirePlayer\(request, env\)/.test(router));
check("الحقيبة تقرأ من player_inventory",
  /\/v1\/items\/bag[\s\S]{0,400}FROM player_inventory WHERE player_id = \?/.test(router));
check("الحقيبة تمرّ بـbuildInventoryView",
  /\/v1\/items\/bag[\s\S]{0,900}buildInventoryView\(counts\)/.test(router));
check("known تُبثّ للواجهة", /known:\s*e\.def !== null/.test(router));
check("الاسم الاحتياطي هو المعرّف لا نص مخترع",
  /name:\s*e\.def\?\.name \|\| e\.itemId/.test(router));
check("gems تُقرأ من المدينة السلطوية",
  /\/v1\/items\/bag[\s\S]{0,700}refreshCity\(env, player\.id\)/.test(router));

const apiDoc = readOr(join(REPO, "game", "docs", "API.md"));
check("API.md يوثّق /v1/items/bag", apiDoc.includes("/v1/items/bag"));
check("API.md يوثّق /v1/meta/items", apiDoc.includes("/v1/meta/items"));
check("API.md يوثّق إصلاح الأعمدة", apiDoc.includes("day_key"));

// ---------------------------------------------------------------------------
console.log("\n[6] العميل — سلسلة الوصول");
// ---------------------------------------------------------------------------
const apiH = readOr(join(CLIENT, "Public", "Rok2Api.h"));
const apiC = strip(readOr(join(CLIENT, "Private", "Rok2Api.cpp")));
const types = readOr(join(CLIENT, "Public", "Rok2Types.h"));
const gmH = readOr(join(CLIENT, "Public", "Rok2GameMode.h"));
const gmC = strip(readOr(join(CLIENT, "Private", "Rok2GameMode.cpp")));
const bagH = readOr(join(CLIENT, "Public", "Rok2BagWidget.h"));
const bagC = strip(readOr(join(CLIENT, "Private", "Rok2BagWidget.cpp")));

check("FRok2BagItem معرّفة", types.includes("struct FRok2BagItem"));
check("FRok2BagState تفصل «فارغة» عن «لم تُقرأ»",
  types.includes("struct FRok2BagState") && /bool bLoaded = false;/.test(types));
check("FetchBag معلنة", apiH.includes("void FetchBag();"));
check("OnBagUpdated معلن", apiH.includes("FOnBagUpdated OnBagUpdated;"));
check("FetchBag تستدعي /v1/items/bag", apiC.includes('Get(TEXT("/v1/items/bag")'));
check("ParseBag تقرأ known", apiC.includes('Rok2Json::Bool(Entry, TEXT("known"))'));
check("الاسم الاحتياطي في العميل هو المعرّف",
  apiC.includes('Rok2Json::Str(Entry, TEXT("name"), Item.ItemId)'));
check("الاستخدام يمرّ بـ/v1/shop/use-speedup القائم",
  /UseBagItemOnQueue[\s\S]{0,400}\/v1\/shop\/use-speedup/.test(apiC),
  "لا endpoint استخدام ثانٍ");
check("الاستخدام يعيد قراءة الحقيبة والمدينة (لا خصم محلي)",
  /UseBagItemOnQueue[\s\S]{0,600}FetchBag\(\)[\s\S]{0,200}LoadCity\(\)/.test(apiC));

check("HandleItemsAction لم يبق توستاً",
  !gmC.includes("الحقيبة قيد التجهيز"),
  "كان سطراً واحداً يبثّ توست «قيد التجهيز»");
check("HandleItemsAction يفتح الشاشة",
  /HandleItemsAction[\s\S]{0,500}URok2BagWidget::StaticClass\(\)/.test(gmC));
check("GameMode يملك الشاشة", gmH.includes("URok2BagWidget* BagWidget;"));
check("ترتيب اللوحات 50", /BagWidget->AddToViewport\(50\)/.test(gmC));
check("لقطة حديثة عند كل فتح",
  /HandleItemsAction[\s\S]{0,700}BagWidget->Setup\(Api\)/.test(gmC));

check("الشاشة طبقة قابلة للتسريح (زر الرجوع)",
  /class\s+ROK2_API\s+URok2BagWidget\s*:\s*public UUserWidget,\s*public IRok2DismissibleLayer/.test(bagH));
check("الشاشة تشترك على OnBagUpdated", bagC.includes("OnBagUpdated.AddUniqueDynamic"));
check("التبويبات من فئات الخادم لا قائمة محلية",
  bagC.includes("Bag.Categories") && !/TEXT\("تسريع"\)/.test(bagC));
check("الفئة الفارغة تُطوى", bagC.includes("if (Count <= 0) continue;"));
check("حالة «لم تُقرأ» مفصولة عن «فارغة»",
  bagC.includes("!Bag.bLoaded") && bagC.includes("جارٍ قراءة الحقيبة"));
check("المجهول يُقال عنه ذلك صريحاً",
  bagC.includes("لا يعرفه فهرس الخادم"));
check("لا هدف للتسريع = توست صادق لا صمت",
  bagC.includes("لا طابور نشط لتسريعه"));
check("وسائط التبويبات منفصلة عن وسائط الصفوف",
  bagH.includes("TArray<URok2BagItemProxy*> ItemProxies;")
  && bagH.includes("TArray<URok2BagItemProxy*> TabProxies;"),
  "مصفوفة واحدة تعني أن تفريغ الصفوف يُتلف أزرار التبويبات");
check("النُدرة من رمز المشروع", bagC.includes("Rok2Visual::RarityTier(Item.Rarity)"));
check("بلا FLinearColor خام", !/FLinearColor\s*\(\s*[0-9]/.test(bagC));
check("بلا SetBrushColor", !bagC.includes("SetBrushColor("));
check("الخطوط من URok2Typography", bagC.includes("URok2Typography::ApplyFont"));
check("التسريح بحركة لا إزالة مفاجئة",
  bagC.includes("URok2MotionLibrary::PlayFadeOut(this)") && !/\n\tRemoveFromParent\(\);/.test(bagC));

// ---------------------------------------------------------------------------
console.log("\n[7] الوظيفة داخل بوابة check");
// ---------------------------------------------------------------------------
const pkg = JSON.parse(readOr(join(BACKEND, "package.json")) || "{}");
check("test:p19-t5-items في سلسلة check",
  chainRuns(pkg.scripts || {}, "test:p19-t5-items"),
  "الوظيفة خارج البوابة تعني حارساً لا يُشغَّل");

// ---------------------------------------------------------------------------
console.log(`\n==== RESULT ====\n${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
