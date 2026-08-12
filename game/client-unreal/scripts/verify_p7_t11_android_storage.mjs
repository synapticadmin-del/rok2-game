// P7-T11: فحص بنيوي لإصلاح طلب صلاحية التخزين على Android.
// يتحقق من:
// 1) وجود ManifestRequirementsOverride.txt وعدم احتوائه أي storage permissions.
// 2) وجود ManifestApplicationAdditions.txt مع requestLegacyExternalStorage="false".
// 3) bUseExternalFilesDir=True في DefaultEngine.ini.
// 4) bPackageDataInsideApk=True (البيانات داخل APK — لا تأثير OBB).
// 5) عدم وجود WRITE_EXTERNAL_STORAGE أو READ_EXTERNAL_STORAGE في أي ملف manifest/ini/Build.cs.
// EXIT=0 يعني نجاح الفحص؛ أي فحص يفشل يطبع CHECK-FAIL ويخرج برمز غير صفر.

import { readFileSync, existsSync } from 'node:fs';
import { join } from 'node:path';

const root = join(import.meta.dirname ?? process.cwd(), '..', '..', '..');
const client = join(root, 'game', 'client-unreal');

let pass = 0, fail = 0;
function check(name, ok, detail = '') {
  if (ok) { pass++; console.log(`CHECK-PASS ${name}`); }
  else { fail++; console.log(`CHECK-FAIL ${name}${detail ? ' — ' + detail : ''}`); }
}

const ini = readFileSync(join(client, 'Config', 'DefaultEngine.ini'), 'utf8');
const overridePath = join(client, 'Build', 'Android', 'ManifestRequirementsOverride.txt');
const additionsPath = join(client, 'Build', 'Android', 'ManifestApplicationAdditions.txt');

// 1) ملف Override موجود
check('override-manifest-exists', existsSync(overridePath));
// 2) لا storage permissions في Override
if (existsSync(overridePath)) {
  const ov = readFileSync(overridePath, 'utf8');
  check('override-no-write-storage', !/WRITE_EXTERNAL_STORAGE/.test(ov));
  check('override-no-read-storage', !/READ_EXTERNAL_STORAGE/.test(ov));
  check('override-no-manage-storage', !/MANAGE_EXTERNAL_STORAGE/.test(ov));
  // الصلاحيات المسموحة فقط
  check('override-has-internet', /android\.permission\.INTERNET/.test(ov));
  check('override-has-network-state', /ACCESS_NETWORK_STATE/.test(ov));
  check('override-has-wifi-state', /ACCESS_WIFI_STATE/.test(ov));
  check('override-targetsdk-34', /targetSdkVersion="34"/.test(ov));
  check('override-minsdk-26', /minSdkVersion="26"/.test(ov));
} else {
  for (const n of ['override-no-write-storage', 'override-no-read-storage',
    'override-no-manage-storage', 'override-has-internet', 'override-has-network-state',
    'override-has-wifi-state', 'override-targetsdk-34', 'override-minsdk-26']) { fail++; }
}

// 3) ملف Additions + requestLegacyExternalStorage=false
check('additions-manifest-exists', existsSync(additionsPath));
if (existsSync(additionsPath)) {
  const ad = readFileSync(additionsPath, 'utf8');
  check('additions-legacy-false', /requestLegacyExternalStorage="false"/.test(ad));
} else { fail++; }

// 4) إعدادات DefaultEngine.ini
check('use-external-files-dir-true', /bUseExternalFilesDir=True/i.test(ini) && !/bUseExternalFilesDir=False/i.test(ini));
check('package-data-inside-apk-true', /bPackageDataInsideApk=True/i.test(ini));
check('allow-patch-obb-false', /bAllowPatchOBBFile=False/i.test(ini));
check('targetsdk-34', /TargetSDKVersion=34/i.test(ini));
check('minsdk-26', /MinSDKVersion=26/i.test(ini));

// 5) لا يوجد أي storage permission في ملفات الإعدادات والملفات البنيوية
const iniFiles = ['Config/DefaultEngine.ini', 'Config/Android/AndroidEngine.ini']
  .map(p => join(client, p)).filter(p => existsSync(p));
let iniClean = true;
for (const f of iniFiles) {
  const t = readFileSync(f, 'utf8');
  if (/WRITE_EXTERNAL_STORAGE|READ_EXTERNAL_STORAGE|MANAGE_EXTERNAL_STORAGE/i.test(t)) { iniClean = false; break; }
}
check('no-storage-permissions-in-ini-files', iniClean, iniFiles.join(', '));

// 6) Build.cs لا يضيف صلاحيات تخزين
const buildCsPath = join(client, 'Source', 'Rok2', 'Rok2.Build.cs');
check('build-cs-exists', existsSync(buildCsPath));
if (existsSync(buildCsPath)) {
  const cs = readFileSync(buildCsPath, 'utf8');
  check('build-cs-no-storage-permissions', !/WRITE_EXTERNAL_STORAGE|READ_EXTERNAL_STORAGE/.test(cs));
} else { fail++; }

console.log(`\nP7-T11: ${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
