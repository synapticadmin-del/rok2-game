import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "../../..");
const read = (relative) => fs.readFileSync(path.join(root, relative), "utf8");
const readGame = (relative) => read(path.join("game", relative));
let failures = 0;

function check(condition, message) {
  if (condition) console.log(`OK  : ${message}`);
  else {
    console.error(`FAIL: ${message}`);
    failures += 1;
  }
}

const catalog = JSON.parse(readGame("backend/src/data/alliance_structures.json"));
const gameData = readGame("backend/src/lib/gameData.ts");
const zones = JSON.parse(readGame("backend/src/data/zones.json"));
const shard = readGame("backend/src/do/KingdomShard.ts");
const router = readGame("backend/src/http/router.ts");
const types = readGame("client-unreal/Source/Rok2/Public/Rok2Types.h");
const apiHeader = readGame("client-unreal/Source/Rok2/Public/Rok2Api.h");
const api = readGame("client-unreal/Source/Rok2/Private/Rok2Api.cpp");
const renderer = readGame("client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp");
const rendererHeader = readGame("client-unreal/Source/Rok2/Public/Rok2WorldRenderer.h");

check(catalog.version === 1, "كاتالوج المنشآت يحمل إصداراً صريحاً");
check(catalog.placement?.requires_alliance_territory === true, "البناء يتطلب إقليماً للتحالف");
check(Number(catalog.placement?.minimum_spacing) > 0, "الكتالوج يحدد مسافة فصل بين المنشآت");
for (const kind of ["watchtower", "bastion", "catapult_emplacement"]) {
  const structure = catalog.structures.find((entry) => entry.id === kind);
  check(Boolean(structure), `النوع ${kind} معرف في الكتالوج`);
  check(Boolean(structure?.required_rank_permission), `${kind} مربوط بصلاحية رتبة`);
  check(Number.isFinite(Number(structure?.max_per_alliance)) && Number(structure.max_per_alliance) > 0, `${kind} يملك سقفاً لكل تحالف`);
  check(typeof structure?.map_marker === "string" && structure.map_marker.length > 0, `${kind} يملك علامة خريطة`);
}
check(gameData.includes("getAllianceStructures"), "فهرس بيانات الخادم يصدّر كتالوج المنشآت");
for (const rank of ["R3", "R4", "R5"]) {
  check(Array.isArray(zones.alliance?.rank_permissions?.[rank]) && zones.alliance.rank_permissions[rank].includes("structure"), `${rank} يحمل صلاحية بناء المنشآت`);
}
check(shard.includes("CREATE TABLE IF NOT EXISTS alliance_structures"), "الخادم يهاجر جدول منشآت التحالف");
check(shard.includes("allianceStructureCatalog: getAllianceStructures()"), "لقطة العالم تبث كتالوج المنشآت السلطوي");
check(shard.includes("allianceStructures: [...this.allianceStructures.values()]"), "لقطة العالم تبث منشآت التحالف الحية");
check(shard.includes("build-alliance-structure"), "الشارد يعالج طلب بناء منشأة التحالف");
check(shard.includes("structure_requires_alliance_territory"), "الشارد يرفض البناء خارج إقليم التحالف");
check(shard.includes("structure_too_close"), "الشارد يفرض مسافة الفصل بين المنشآت");
check(router.includes('/v1/alliance/structure/build'), "الموجه يعرّض مسار بناء مصادقاً عليه");
check(router.includes("insufficient_rank"), "الموجه يرفض الرتب غير المخولة");
check(router.includes("createdBy: player.id"), "الموجه يربط هوية الباني بالرمز المصادق عليه");
check(types.includes("struct FRok2AllianceStructure"), "عميل Unreal يملك نموذج منشأة التحالف");
check(types.includes("TArray<FRok2AllianceStructure> AllianceStructures"), "لقطة العميل تحمل منشآت التحالف");
check(api.includes('TryGetArrayField(TEXT("allianceStructures")'), "العميل يحلل المنشآت من لقطة الخادم");
check(api.includes('TEXT("protectionRadius")'), "العميل يقرأ نطاق الحماية السلطوي");
check(apiHeader.includes("BuildAllianceStructure"), "واجهة Unreal تتيح بناء منشأة في موقع محدد");
check(api.includes('Post(TEXT("/v1/alliance/structure/build")'), "العميل يرسل مسار بناء المنشأة المصادق عليه");
check(api.includes("RefreshWorld();") && api.includes("تم إنشاء منشأة التحالف"), "العميل يعيد تحميل الخريطة بعد نجاح البناء");
check(rendererHeader.includes("AllianceStructureMesh"), "محرر Unreal يعرض أصل علامة المنشأة القابل للتخصيص");
check(rendererHeader.includes("ProtectionRadiusMesh"), "محرر Unreal يعرض أصل نطاق الحماية القابل للتخصيص");
check(rendererHeader.includes("RequestAllianceStructureAtWorldPoint"), "مصمم الخريطة يتيح اختيار موقع للبناء من الواجهة أو Blueprint");
check(renderer.includes("WorldPoint.X / WorldToUnrealScale") && renderer.includes("WorldPoint.Y / WorldToUnrealScale"), "مصمم الخريطة يحوّل موضع Unreal إلى إحداثيات الخادم");
check(renderer.includes("WorldToUnrealScale <= KINDA_SMALL_NUMBER"), "مصمم الخريطة يرفض مقياساً غير صالح قبل طلب البناء");
check(renderer.includes("for (const FRok2AllianceStructure& S : W.AllianceStructures)"), "مصمم الخريطة يرسم كل منشأة حية");
check(renderer.includes("ProtectionRadiusMesh && S.ProtectionRadius > 0.0"), "مصمم الخريطة يرسم نطاق الحماية فقط عند وجوده");
check(renderer.includes("bFriendly"), "ألوان المنشآت تفرق بين الحليف والعدو");

if (failures) {
  console.error(`فشل ${failures} من فحوص عقد منشآت التحالف.`);
  process.exit(1);
}
console.log("فحص عقد منشآت التحالف اجتاز.");
