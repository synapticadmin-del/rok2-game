import { existsSync, readFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
const __dirname = dirname(fileURLToPath(import.meta.url));
const ROOT = resolve(__dirname, "../../..");
const errors = [];
function source(relativePath) {
  const absolutePath = join(ROOT, relativePath);
  if (!existsSync(absolutePath)) {
    errors.push(`ملف الربط مفقود: ${relativePath}`);
    return "";
  }
  return readFileSync(absolutePath, "utf8");
}
function requireEvidence(relativePath, content, pattern, description) {
  if (!pattern.test(content)) {
    errors.push(`دليل P7-T1 onboarding مفقود في ${relativePath}: ${description}`);
  }
}
const e2ePath = "game/backend/scripts/e2e_p7_t1_guest_onboarding.mjs";
const e2e = source(e2ePath);
const docPath = "game/docs/P7_T1_GUEST_ONBOARDING_E2E.md";
const doc = source(docPath);
const router = source("game/backend/src/http/router.ts");
const backendPackage = source("game/backend/package.json");

// السكربت موجود ويوثّق وضعيه التشغيل (معزول + مباشر) ومراحل الفلو السبع.
requireEvidence(e2ePath, e2e, /sandboxedServer|WRANGLER_D1_STATE_PATH/,
  "وضع معزول كامل (قاعدة D1/DO جديدة + هجرات + dev + تنظيف)");
requireEvidence(e2ePath, e2e, /E2E_LIVE/, "وضع مباشر E2E_LIVE=1 BASE_URL=<url>");
requireEvidence(e2ePath, e2e, /\/v1\/auth\/guest/, "مرحلة تسجيل الضيف");
requireEvidence(e2ePath, e2e, /\/v1\/me/, "مرحلة قياس الهوية قبل وبعد التأسيس");
requireEvidence(e2ePath, e2e, /\/v1\/city\/init/, "مرحلة تأسيس المدينة (FTUE)");
requireEvidence(e2ePath, e2e, /\/v1\/commanders/, "مرحلة القادة (مملوكون + روستر 18)");
requireEvidence(e2ePath, e2e, /\/v1\/world\/snapshot/, "مرحلة لقطة العالم");
requireEvidence(e2ePath, e2e, /seasonDay/, "جاهزية seasonStory في اللقطة");
requireEvidence(e2ePath, e2e, /re-authenticates to the same account|إعادة المصادقة/, "استمرارية الجلسة للجهاز العائد");
requireEvidence(e2ePath, e2e, /process\.exit/, "إنهاء صريح برمز نجاح/فشل");

// الوثيقة تشرح الفلو ووسائط التشغيل ودليل قبول PIE وما لا يغطيه الاختبار.
requireEvidence(docPath, doc, /Rok2BootWidget/, "ربط الفلو بمسار الواجهة في العميل");
requireEvidence(docPath, doc, /URok2Api::LoginAsGuest|LoginAsGuest/, "استدعاء تسجيل الضيف في الـ API");
requireEvidence(docPath, doc, /city\/init/, "توثيق تأسيس المدينة");
requireEvidence(docPath, doc, /PIE_TWO_CLIENTS_ANDROID_ACCEPTANCE/, "إحالة معيار قبول PIE النهائي");
requireEvidence(docPath, doc, /18/, "توثيق عدد القادة في الروستر المركزي");

// السكربت مسجل في سلسلة check في game/backend/package.json (job مستقل).
requireEvidence("game/backend/package.json", backendPackage, /test:p7-t1-onboarding.*verify_p7_t1_onboarding/,
  "test job في package.json يشير للسكربت");

// endpoints المختبرة موجودة فعليًا في الخادم (دليل عدم انحراف الفحص عن الواجهة).
requireEvidence("game/backend/src/http/router.ts", router, /\/v1\/auth\/guest/, "POST /v1/auth/guest في الراوتر");
requireEvidence("game/backend/src/http/router.ts", router, /\/v1\/me/, "GET /v1/me في الراوتر");
requireEvidence("game/backend/src/http/router.ts", router, /\/v1\/city\/init/, "POST /v1/city/init في الراوتر");
requireEvidence("game/backend/src/http/router.ts", router, /\/v1\/commanders/, "GET /v1/commanders في الراوتر");
requireEvidence("game/backend/src/http/router.ts", router, /\/v1\/world\/snapshot/, "GET /v1/world/snapshot في الراوتر");

if (errors.length) {
  console.error("فشل تحقق P7-T1 onboarding:");
  errors.forEach((error) => console.error(`- ${error}`));
  process.exit(1);
}
console.log("✓ P7-T1 onboarding: سكربت E2E لتسجيل الضيف وتأسيس المدينة موثق ومسجل في سلسلة الفحص.");
process.exit(0);
