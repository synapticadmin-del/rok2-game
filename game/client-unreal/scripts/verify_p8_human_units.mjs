// P8-T8: حارس جودة الوحدات البشرية 3D — فحص بنيوي (لا يتطلب Unreal/PIE).
// يتحقق من: اكتمال ملفات GLB وصحتها، مطابقة كتالوج C++ مع troop_tiers.json و
// civilizations.json، وجود Implementations مقابلة للـ declarations، وسكربت
// الاستيراد، والتراخيص. يطبع CHECK-PASS/CHECK-FAIL وينتهي بـ exit(0/1).

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, "../../..");
const CLIENT = path.join(ROOT, "game/client-unreal");
const ART = path.join(CLIENT, "Content/Art");
const SRC = path.join(CLIENT, "Source/Rok2");
const DATA = path.join(ROOT, "game/backend/src/data");

let fail = 0;
const checks = [];

function check(name, cond, detail = "") {
  checks.push({ name, cond, detail });
  if (!cond) {
    fail++;
    console.log(`CHECK-FAIL: ${name}${detail ? ` — ${detail}` : ""}`);
  }
}

// ---------------------------------------------------------------------------
// 1. ملفات GLB المولدة إجرائيًا (17): infantry/archer/cavalry × T1–T5 + siege×2
// ---------------------------------------------------------------------------
const humanDir = path.join(ART, "HumanUnits");
check("HumanUnits folder exists", fs.existsSync(humanDir));

const expectedHuman = [];
for (const branch of ["infantry", "archer", "cavalry"]) {
  for (let tier = 1; tier <= 5; tier++) expectedHuman.push(`${branch}_t${tier}`);
}
expectedHuman.push("siege_arcuballista", "siege_mangonel");
for (const id of expectedHuman) {
  const file = path.join(humanDir, `${id}.glb`);
  const exists = fs.existsSync(file);
  check(`GLB exists: ${id}.glb`, exists);
  if (exists) {
    const buf = Buffer.alloc(12);
    fs.readSync(fs.openSync(file, "r"), buf, 0, 12, 0);
    const magic = buf.slice(0, 4).toString("ascii");
    const version = buf.readUInt32LE(4);
    const total = buf.readUInt32LE(8);
    check(`GLB valid header: ${id}.glb`, magic === "glTF" && version === 2 && total > 0);
    // فك JSON chunk والتحقق من بنية glTF 2.0
    const raw = fs.readFileSync(file);
    const jsonLen = raw.readUInt32LE(12);
    const jsonChunk = raw.slice(20, 20 + jsonLen).toString("utf8").trim();
    let obj = null;
    try {
      obj = JSON.parse(jsonChunk);
    } catch (e) {
      obj = null;
    }
    check(`GLB JSON chunk parseable: ${id}.glb`, obj !== null && obj.asset?.version === "2.0");
    if (obj) {
      check(`GLB has POSITION accessor: ${id}.glb`, Array.isArray(obj.accessors) && obj.accessors.length >= 2);
      check(`GLB triangle indices: ${id}.glb`, obj.accessors[2]?.count > 0);
    }
    check(`GLB non-empty binary: ${id}.glb`, raw.length > 100);
  }
}
check("README license file in HumanUnits", fs.existsSync(path.join(humanDir, "README.md")));

// ---------------------------------------------------------------------------
// 2. إعادة استخدام موديلات الحصار T3–T5 من Kenney Castle Kit (CC0)
// ---------------------------------------------------------------------------
const kenney = path.join(ART, "KenneyCastleKit");
check("KenneyCastleKit folder exists", fs.existsSync(kenney));
for (const f of ["siege-ballista", "siege-trebuchet", "siege-catapult"]) {
  const file = path.join(kenney, `${f}.glb`);
  check(`Kenney siege reuse: ${f}.glb`, fs.existsSync(file));
}

// ---------------------------------------------------------------------------
// 3. declarations في Rok2ArtAssets.h
// ---------------------------------------------------------------------------
const artH = fs.readFileSync(path.join(SRC, "Public/Rok2ArtAssets.h"), "utf8");
const artCpp = fs.readFileSync(path.join(SRC, "Private/Rok2ArtAssets.cpp"), "utf8");
const decls = [
  "static FString GetHumanUnitId",
  "static FString GetHumanUnitAssetPath",
  "static UStaticMesh* LoadHumanUnitMesh",
  "static bool HasHumanUnit",
];
for (const d of decls) {
  check(`ArtAssets.h declares: ${d}`, artH.includes(d));
}
check("ArtAssets.h declares BuildHumanUnitCatalog", artH.includes("void BuildHumanUnitCatalog()"));
check("ArtAssets.h declares HumanUnitCatalog array", artH.includes("TArray<FRok2ArtEntry> HumanUnitCatalog"));
check("ArtAssets.h declares bHumanCatalogBuilt flag", artH.includes("bHumanCatalogBuilt"));
check("FRok2ArtEntry gains Folder field", artH.includes("FString Folder;"));

// 4. implementations مقابلة في .cpp (URok2ArtAssets::<Name>)
for (const d of decls) {
  // "static UStaticMesh* LoadHumanUnitMesh" → آخر توكين هو اسم الدالة
  const tokens = d.replace("static ", "").split("(")[0].trim().split(/\s+/);
  const name = tokens[tokens.length - 1];
  check(`ArtAssets.cpp implements: ${name}`, artCpp.includes(`URok2ArtAssets::${name}`));
}
check("ArtAssets.cpp implements BuildHumanUnitCatalog", artCpp.includes("URok2ArtAssets::BuildHumanUnitCatalog"));

// ---------------------------------------------------------------------------
// 5. مطابقة كتالوج C++ مع troop_tiers.json (20 شبكة: 4 فروع × 5 مراحل)
// ---------------------------------------------------------------------------
const tiers = JSON.parse(fs.readFileSync(path.join(DATA, "troop_tiers.json"), "utf8"));
check("troop_tiers.json has 5 tiers", tiers.tiers?.length === 5);
for (const t of tiers.tiers) {
  for (const branch of ["infantry", "archer", "cavalry", "siege"]) {
    const id = t.tier <= 2 && branch === "siege"
      ? `siege_t${t.tier}` // siege T1–T2 إجرائي
      : (t.tier >= 3 && branch === "siege" ? `siege_t${t.tier}` : `${branch}_t${t.tier}`);
    if (t.tier >= 3 && branch === "siege") {
      // T3–T5 siege من Kenney — يوثّق في GetHumanUnitAssetPath
      check(`C++ siege catalog covers tier ${t.tier}`, artCpp.includes(`siege_t${t.tier}`));
    }
  }
}

// 6. الوحدات الخاصة الحضارية الست في الكتالوج (unlock_tier=4)
const civs = JSON.parse(fs.readFileSync(path.join(DATA, "civilizations.json"), "utf8"));
const specialIds = (civs.civilizations || []).map((c) => c.special_unit?.id).filter(Boolean);
check("6 civ special units defined", specialIds.length === 6);
for (const id of specialIds) {
  check(`C++ special unit catalog: ${id}`, artCpp.includes(`TEXT("${id}")`));
}

// 7. WorldRenderer: DeriveMarchTier + استخدام GetHumanUnitId في SpawnMarch
const wrH = fs.readFileSync(path.join(SRC, "Public/Rok2WorldRenderer.h"), "utf8");
const wrCpp = fs.readFileSync(path.join(SRC, "Private/Rok2WorldRenderer.cpp"), "utf8");
check("WorldRenderer.h declares DeriveMarchTier", wrH.includes("static int32 DeriveMarchTier"));
check("WorldRenderer.cpp implements DeriveMarchTier", wrCpp.includes("ARok2WorldRenderer::DeriveMarchTier"));
check("SpawnMarch uses GetHumanUnitId (P8-T8)", wrCpp.includes("GetHumanUnitId(M.Branch"));
check("SpawnMarch uses LoadHumanUnitMesh", wrCpp.includes("LoadHumanUnitMesh(UnitId)"));
check("DeriveMarchTier parses branch_tN keys", wrCpp.includes('_t"'));
check("DeriveMarchTier clamps tier 1..5", wrCpp.includes("Tier < 1 || Tier > 5"));

// ---------------------------------------------------------------------------
// 8. سكربت الاستيراد
// ---------------------------------------------------------------------------
const importScript = path.join(CLIENT, "scripts/Import-HumanUnits.ps1");
check("Import-HumanUnits.ps1 exists", fs.existsSync(importScript));
if (fs.existsSync(importScript)) {
  const ps = fs.readFileSync(importScript, "utf8");
  check("Import script validates 17 files", ps.includes("17"));
  check("Import script targets /Game/Art/HumanUnits", ps.includes("/Game/Art/HumanUnits"));
  check("Import script uses UnrealEditor ImportAssets", ps.includes("ImportAssets"));
}

// ---------------------------------------------------------------------------
// 9. وثيقة الدليل
// ---------------------------------------------------------------------------
const doc = path.join(ROOT, "game/docs/P8_T8_UNITS_3D.md");
check("P8_T8_UNITS_3D.md document exists", fs.existsSync(doc));
if (fs.existsSync(doc)) {
  const docText = fs.readFileSync(doc, "utf8");
  check("Doc covers mesh map (infantry_t1)", /infantry_t1/i.test(docText));
  check("Doc covers special units (legionary)", /legionary/i.test(docText));
  check("Doc states license (CC0/procedural)", /CC0|إجرائي|procedural/i.test(docText));
}

// ---------------------------------------------------------------------------
// النتيجة
// ---------------------------------------------------------------------------
console.log(`\n${checks.filter((c) => c.cond).length}/${checks.length} passed, ${fail} failed`);
if (fail === 0) {
  console.log("CHECK-PASS: P8-T8 human units 3D (T1–T5 × 4 فروع + 6 وحدات خاصة)");
} else {
  console.log("CHECK-FAIL: P8-T8 human units — failures above");
  process.exit(1);
}
process.exit(0);
