#!/usr/bin/env node
/**
 * tavern_gacha_offline_test.mjs — P19-T4: شاشة الحانة والصناديق.
 *
 * ما كان مكسوراً قبل هذا البند، بالدليل:
 *
 *   1. **24 أصلاً بصرياً بلا مستهلك.** `Content/Art/Tavern` يحمل ثلاثة صناديق
 *      وستة مفاتيح وأربع منحوتات وأربع مواد وستة مخططات، و
 *      `URok2ArtAssets::LoadTavernIcon` معرّفة و`grep` لا يجد لها **أي مستدعٍ**
 *      خارج ملفها.
 *   2. **صوتان بلا نوع أصلاً.** `chest_open.wav` و`wheel_spin.wav` على القرص
 *      و`ERok2AudioType` لا يحمل قيمة تشير إليهما، فلا مسار لتشغيلهما.
 *   3. **`/v1/tavern/daily-key` بلا مستدعٍ في العميل.** الـendpoint موجود منذ
 *      P10-T1، فأول مفتاح يحصل عليه لاعب جديد كان غير قابل للطلب — وحانةٌ بلا
 *      مفتاح لا تُفتح أبداً.
 *   4. **`/v1/tavern/keys` يرسل حقلاً باسم خاطئ.** الراوتر يرسل `key` والشارد
 *      يقرأ `body.keyId` — فالقيمة تصل فارغة **دائماً** ويُكتب رصيد على المفتاح
 *      `""`. والافتراضي كان `"bronze"` وهو ليس مفتاحاً في `tavern.json` أصلاً.
 *   5. **`__lastFreeDay` حقل خارج النوع.** يوم آخر مفتاح مجاني كان يُحفظ في
 *      `(state as any).__lastFreeDay`، و`persistTavern` يكتب `keys_json` و
 *      `history_json` وحدهما — فالراية تضيع مع كل استئناف للشارد ويصير المفتاح
 *      «اليومي» متاحاً كلما أُعيد تحميل الكائن. تجاوز اقتصادي لا إزعاج.
 *   6. **ثلاثة حقول لا تُرسَل.** `ParseTavernState` يقرأ `lastRolls` و
 *      `opensThisHour` و`dailyKeyClaimed`، وحمولة `tavern-state` كانت
 *      `{ keys, historyCount, antiCheat }` — فالشاشة تعرض صفر فتحات ومفتاحاً
 *      مجانياً متاحاً دائماً.
 *
 * Usage: node scripts/tavern_gacha_offline_test.mjs
 */

import { readFileSync, existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { chainRuns } from "../../../scripts/lib/npm_script_chain.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const BACKEND = join(here, "..");
const REPO = join(BACKEND, "..", "..");
const CLIENT_ROOT = join(REPO, "game", "client-unreal");
const CLIENT = join(CLIENT_ROOT, "Source", "Rok2");

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
/**
 * يزيل تعليقات C/TS بلا أن يقصّ الروابط.
 *
 * `//` بعد نقطتين هو مخطّط URL لا تعليق: `"https://do/tavern-add-keys"` كان
 * يُقصّ بالكامل مع النمط الساذج `\/\/[^\n]*` — فيصير الفحص يبحث في فراغ **ويمرّ
 * أو يفشل بلا علاقة بالكود**. هذا العطل بعينه موثّق في `verify_delegate_bind`
 * بصيغة أخرى (ترتيب قصّ الكتل قبل السطور)، وهو صنف واحد من الأخطاء: حارسٌ
 * يفحص نصاً محرّفاً.
 */
const strip = (s) =>
  s.replace(/(^|[^:])\/\/[^\n]*/g, "$1").replace(/\/\*[\s\S]*?\*\//g, "");

function fnBody(src, signature) {
  const at = src.indexOf(signature);
  if (at < 0) return "";
  const open = src.indexOf("{", at);
  if (open < 0) return "";
  let depth = 0;
  for (let i = open; i < src.length; i++) {
    if (src[i] === "{") depth++;
    else if (src[i] === "}") {
      depth--;
      if (depth === 0) return src.slice(open, i + 1);
    }
  }
  return "";
}

const tavernJson = JSON.parse(readOr(join(BACKEND, "src", "data", "tavern.json")) || "{}");
const tavernSim = strip(readOr(join(BACKEND, "src", "do", "sim", "tavern.ts")));
const shard = strip(readOr(join(BACKEND, "src", "do", "KingdomShard.ts")));
const router = strip(readOr(join(BACKEND, "src", "http", "router.ts")));
const apiH = readOr(join(CLIENT, "Public", "Rok2Api.h"));
const apiC = strip(readOr(join(CLIENT, "Private", "Rok2Api.cpp")));
const audioH = readOr(join(CLIENT, "Public", "Rok2AudioManager.h"));
const audioC = strip(readOr(join(CLIENT, "Private", "Rok2AudioManager.cpp")));
const gmH = readOr(join(CLIENT, "Public", "Rok2GameMode.h"));
const gmC = strip(readOr(join(CLIENT, "Private", "Rok2GameMode.cpp")));
const tavH = readOr(join(CLIENT, "Public", "Rok2TavernWidget.h"));
const tavC = strip(readOr(join(CLIENT, "Private", "Rok2TavernWidget.cpp")));

// ---------------------------------------------------------------------------
console.log("\n[1] عقد keyId — الحقل كان باسم خاطئ فتصل القيمة فارغة دائماً");
// ---------------------------------------------------------------------------
check("الراوتر يرسل keyId لا key",
  /tavern-add-keys[\s\S]{0,400}keyId:\s*String\(body\.keyId \|\| body\.key/.test(router),
  "الشارد يقرأ body.keyId — إرسال `key` يعني قيمة فارغة دائماً");
check("الافتراضي مفتاح حقيقي لا «bronze»",
  router.includes('|| "silver_key"') && !/\|\| "bronze"/.test(router),
  "«bronze» ليس مفتاحاً في tavern.json (silver_key/gold_key/gear_key)");
check("الشارد يقبل الاسمين (توافق)",
  /body\.keyId \|\| body\.key/.test(shard));
check("addKeys تتحقق من المفتاح قبل الإضافة",
  tavernSim.includes("spec.boxes.some(b => b.key === key)"),
  "أي نص كان يُقبل فيُكتب رصيد على مفتاح لا وجود له");
check("addKeys ترفض العدد ≤ 0", tavernSim.includes("if (!valid || count <= 0) return state;"));
check("الرفض يُبلَّغ لا يبدو نجاحاً",
  shard.includes('error: "unknown_key"') && shard.includes("tavern_unknown_key:"),
  "طلبٌ ينجح بـ200 بلا أثر يُخفي العطل");

// ---------------------------------------------------------------------------
console.log("\n[2] المفتاح اليومي — راية كانت تضيع مع كل استئناف");
// ---------------------------------------------------------------------------
check("lastFreeDay حقل في النوع لا خارجه",
  tavernSim.includes("lastFreeDay?: string;"),
  "(state as any).__lastFreeDay لا يعرفه persistTavern فيضيع");
check("لا __lastFreeDay باقٍ", !shard.includes("__lastFreeDay"));
check("عمود last_free_day في جدول الشارد",
  shard.includes("ALTER TABLE tavern_state ADD COLUMN last_free_day"));
check("هجرة الشارد مرقّمة (21)",
  shard.includes("_sql_schema_migrations (id) VALUES (21)"));
check("persistTavern يكتب العمود",
  /INSERT INTO tavern_state \(player_id, keys_json, history_json, last_free_day\)/.test(shard));
check("loadP10State يقرأ العمود",
  shard.includes("lastFreeDay: String(row.last_free_day"));
check("المنح يقرأ الراية المحفوظة",
  shard.includes("dailyFreeKey(state, dayKey, state.lastFreeDay)"));
check("المنح يحفظ الراية في الحالة",
  /lastFreeDay: dayKey/.test(shard));

// ---------------------------------------------------------------------------
console.log("\n[3] الحمولة — ثلاثة حقول يقرأها العميل ولم تكن تُرسَل");
// ---------------------------------------------------------------------------
const stateHandler = shard.slice(shard.indexOf('path.endsWith("/tavern-state")'));
check("tavern-state يرسل lastRolls", /lastRolls:/.test(stateHandler.slice(0, 1200)));
check("tavern-state يرسل opensThisHour", /opensThisHour,/.test(stateHandler.slice(0, 1200)));
check("tavern-state يرسل dailyKeyClaimed", /dailyKeyClaimed:/.test(stateHandler.slice(0, 1200)));
check("dailyKeyClaimed محسوبة من يوم اليوم",
  /dailyKeyClaimed:\s*\(state\.lastFreeDay \|\| ""\) === dayString\(nowMs\(\)\)/.test(shard));

const openHandler = shard.slice(shard.indexOf('path.endsWith("/tavern-open")'));
check("tavern-open يرسل keys بعد الخصم",
  /keys:\s*state\.keys,/.test(openHandler.slice(0, 2500)),
  "رصيد المفاتيح على الشاشة كان يبقى كما كان بعد الفتح");
check("tavern-open يرسل lastRolls بكمياتها",
  /lastRolls:\s*rollResult\.rolls\.map/.test(openHandler.slice(0, 2500)));

// العميل يقرأ ما يُرسَل فعلاً.
for (const field of ["lastRolls", "opensThisHour", "dailyKeyClaimed"]) {
  check(`ParseTavernState يقرأ ${field}`, apiC.includes(`TEXT("${field}")`));
}

// ---------------------------------------------------------------------------
console.log("\n[4] المفتاح المجاني صار قابلاً للطلب");
// ---------------------------------------------------------------------------
check("ClaimTavernDailyKey معلنة", apiH.includes("void ClaimTavernDailyKey();"));
check("تستدعي /v1/tavern/daily-key", apiC.includes('Post(TEXT("v1/tavern/daily-key")'));
const claimBody = fnBody(apiC, "void URok2Api::ClaimTavernDailyKey");
check("الحالتان تُبلَّغان بصدق",
  claimBody.includes('Rok2Json::Bool(Obj, TEXT("granted"))')
  && claimBody.includes("مُستلَم بالفعل"),
  "توست نجاح موحّد يكذب على لاعب استلمه");
check("إعادة قراءة كاملة بعده",
  claimBody.includes("FetchTavernState()"),
  "حمولة daily-key تعيد keys وحدها بلا سقف الساعة ولا الراية");
check("GetTavernState متاحة للواجهة",
  apiH.includes("const FRok2TavernState& GetTavernState() const"));

// ---------------------------------------------------------------------------
console.log("\n[5] الصوتان صار لهما نوع ومستدعٍ");
// ---------------------------------------------------------------------------
check("chest_open.wav على القرص",
  existsSync(join(CLIENT_ROOT, "Content", "Audio", "sfx", "chest_open.wav")));
check("wheel_spin.wav على القرص",
  existsSync(join(CLIENT_ROOT, "Content", "Audio", "sfx", "wheel_spin.wav")));
check("ChestOpen نوع في ERok2AudioType", audioH.includes("ChestOpen,"));
check("WheelSpin نوع في ERok2AudioType", audioH.includes("WheelSpin"));
check("المسارات مسجّلة في BuildAudioPaths",
  audioC.includes("Audio/sfx/chest_open") && audioC.includes("Audio/sfx/wheel_spin"));
check("صوت الدوران عند طلب الفتح (رد فعل فوري)",
  fnBody(tavC, "void URok2TavernWidget::RequestOpenBox").includes("ERok2AudioType::WheelSpin"),
  "الاستجابة تعبر الشبكة — الصوت عند الوصول يتأخر عن حدّ 100ms");
check("صوت الصندوق عند وصول رميات جديدة",
  fnBody(tavC, "void URok2TavernWidget::OnTavernUpdated").includes("ERok2AudioType::ChestOpen"));
check("حارس يمنع تشغيله عند كل تحديث حالة",
  tavH.includes("int32 LastRollCount = 0;")
  && tavC.includes("State.LastRolls.Num() != LastRollCount"),
  "FetchTavernState يبثّ الحدث نفسه عند فتح الشاشة");

// ---------------------------------------------------------------------------
console.log("\n[6] الأصول الـ24 صار لها مستهلك");
// ---------------------------------------------------------------------------
const tavernDir = join(CLIENT_ROOT, "Content", "Art", "Tavern");
for (const asset of ["chest_silver", "chest_gold", "chest_equipment",
  "key_silver", "key_gold", "key_equipment",
  "sculpture_legendary", "sculpture_epic", "sculpture_elite", "sculpture_advanced",
  "material_iron"]) {
  check(`${asset}.png على القرص`, existsSync(join(tavernDir, `${asset}.png`)));
}
check("LoadTavernIcon لها مستدعٍ",
  tavC.includes("URok2ArtAssets::LoadTavernIcon("),
  "كانت معرّفة بلا أي مستدعٍ خارج ملفها");
check("صور الصناديق الثلاثة تُحمّل",
  tavC.includes('TEXT("chest_silver")') && tavC.includes('TEXT("chest_gold")')
  && tavC.includes('TEXT("chest_equipment")'));
check("صور المفاتيح تُحمّل",
  tavC.includes('TEXT("key_silver")') && tavC.includes('TEXT("key_gold")'));
check("صور المنحوتات للرميات",
  tavC.includes('TEXT("sculpture_legendary")') && tavC.includes('TEXT("sculpture_epic")'));
check("احتياط أيقونة إجرائية عند غياب الأصل",
  tavC.includes("URok2ArtAssets::GetIconBrush("),
  "غياب الحزمة لا يُعطّل الشاشة");

// ---------------------------------------------------------------------------
console.log("\n[7] الشاشة — بيانات الخادم لا أرقام محلية");
// ---------------------------------------------------------------------------
check("Rok2TavernWidget.h موجود", tavH.length > 0);
check("طبقة قابلة للتسريح (زر الرجوع)",
  /class\s+ROK2_API\s+URok2TavernWidget\s*:\s*public UUserWidget,\s*public IRok2DismissibleLayer/.test(tavH));
check("تشترك على OnTavernUpdated", tavC.includes("OnTavernUpdated.AddUniqueDynamic"));
check("تجلب الحالة عند الفتح", tavC.includes("Api->FetchTavernState()"));
check("زر الفتح معطّل بصرياً عند غياب المفتاح",
  tavC.includes("Open->SetIsEnabled(KeysHeld > 0)"),
  "زر يبدو جاهزاً ثم يفشل يُعلّم اللاعب ألّا يثق بالواجهة");
check("لا احتمال ولا وزن رمية في العميل",
  !/\b(590|199|150|61|300|80|30|600|250|100|50)\b/.test(tavC.replace(/Rok2Space::\w+/g, "")),
  "الأوزان في tavern.json وحده — العميل يعرض النتيجة لا يحسبها");
check("سقف الساعة يُعرض بلا اختراع مقام",
  tavC.includes("فتحات هذه الساعة") && !/\/\s*60/.test(tavC),
  "maxOpensPerHour لا يصل العميل — عرض مقام مخترع كذب");
check("أسماء الصناديق تطابق tavern.json",
  (tavernJson.boxes || []).every((b) => tavC.includes(`TEXT("${b.id}")`) && tavC.includes(b.name)),
  (tavernJson.boxes || []).filter((b) => !tavC.includes(b.name)).map((b) => b.id).join(", "));
check("مفاتيح الصناديق تطابق tavern.json",
  (tavernJson.boxes || []).every((b) => tavC.includes(`TEXT("${b.key}")`)));
check("النُدرة من رمز المشروع", tavC.includes("Rok2Visual::RarityTier("));
check("وميض ذهبي للرمية الأسطورية",
  tavC.includes("PlayGoldFlash("),
  "§1 «كل تأكيد له وميض ذهبي» — وهذا أجدر موضع به");
check("بلا FLinearColor خام", !/FLinearColor\s*\(\s*[0-9]/.test(tavC));
check("بلا SetBrushColor", !tavC.includes("SetBrushColor("));
check("الخطوط من URok2Typography", tavC.includes("URok2Typography::ApplyFont"));
check("التسريح بحركة", tavC.includes("PlayFadeOut(this)") && !/\n\tRemoveFromParent\(\);/.test(tavC));

// ---------------------------------------------------------------------------
console.log("\n[8] سلسلة الوصول: بطاقة الحانة → GameMode → الشاشة");
// ---------------------------------------------------------------------------
check("chests لم يبق توستاً",
  !gmC.includes("الحانة تُفتح من شاشة الأحداث"),
  "كان توستاً صادقاً انتظاراً لهذه الشاشة");
check("chests يفتح الشاشة",
  /ActionKind == TEXT\("chests"\)[\s\S]{0,200}OpenTavernScreen\(\)/.test(gmC));
check("OpenTavernScreen معلنة", gmH.includes("void OpenTavernScreen();"));
check("GameMode يملك الشاشة", gmH.includes("URok2TavernWidget* TavernWidget;"));
check("ترتيب اللوحات 50", /TavernWidget->AddToViewport\(50\)/.test(gmC));
check("لقطة حديثة عند كل فتح",
  /OpenTavernScreen[\s\S]{0,700}TavernWidget->Setup\(Api\)/.test(gmC));

// ---------------------------------------------------------------------------
console.log("\n[9] الوظيفة داخل بوابة check");
// ---------------------------------------------------------------------------
const pkg = JSON.parse(readOr(join(BACKEND, "package.json")) || "{}");
check("test:p19-t4-tavern في سلسلة check",
  chainRuns(pkg.scripts || {}, "test:p19-t4-tavern"),
  "حارس خارج البوابة لا يُشغَّل");

console.log(`\n==== RESULT ====\n${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);
