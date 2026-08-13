// P8-Descriptor: فحص بنيوي لإصلاح خطأ `Failed to open descriptor file ../../Rok2/Rok2.uproject`
// الذي يظهر بعد بناء التطبيق للاندرويد عند بدء التشغيل.
// السبب: UE 5.4 + SDK 34 يبحث عن ملف وصف المشروع (.uproject) على الجهاز بمسار نسبي
// من مجلد التنفيذ عبر Android File Server، وعند عدم وجوده يعرض الرسالة ويعلق التطبيق.
// الحلول المطبقة (defense in depth):
// 1) Android File Server مُعطَّل (bEnablePlugin=False) في DefaultEngine.ini — السبب الجذري الأوثق
//    توثيقًا: https://forums.unrealengine.com/t/failed-to-open-descriptor-file-android/467113
// 2) bDetectIfAppShouldRun=False في AndroidRuntimeSettings (DefaultEngine.ini و
//    Config/Android/AndroidEngine.ini) — يوقف فحص وجود descriptor نهائيًا لأن البيانات
//    مغلّفة داخل APK (bPackageDataInsideApk=True).
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

const iniPath = join(client, 'Config', 'DefaultEngine.ini');
const ini = readFileSync(iniPath, 'utf8');
const iniSection = ini.split('[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]')[1]?.split('[')[0] ?? '';

check('detect-if-app-should-run-false', /bDetectIfAppShouldRun\s*=\s*False/i.test(iniSection),
  'bDetectIfAppShouldRun=False في AndroidRuntimeSettings — يتجاوز فحص descriptor على الجهاز');
check('package-data-inside-apk-true', /bPackageDataInsideApk\s*=\s*True/i.test(iniSection),
  'البيانات مغلّفة داخل APK فلا حاجة لـ descriptor على الجهاز');

const afsSection = ini.split('[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]')[1]?.split('[')[0] ?? '';
check('android-file-server-disabled', /bEnablePlugin\s*=\s*False/i.test(afsSection),
  'Android File Server مُعطَّل — السبب الجذري لخطأ descriptor حسب threads مجتمع UE');

const androidIniPath = join(client, 'Config', 'Android', 'AndroidEngine.ini');
if (existsSync(androidIniPath)) {
  const androidIni = readFileSync(androidIniPath, 'utf8');
  const androidSection = androidIni.split('[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]')[1]?.split('[')[0] ?? '';
  check('android-ini-detect-if-app-should-run-false', /bDetectIfAppShouldRun\s*=\s*False/i.test(androidSection),
    'bDetectIfAppShouldRun=False مكرر في Config/Android/AndroidEngine.ini (أولوية أعلى)');
  check('android-ini-package-data-inside-apk-true', /bPackageDataInsideApk\s*=\s*True/i.test(androidSection),
    'Config/Android/AndroidEngine.ini يطابق bPackageDataInsideApk=True');
} else {
  fail++; console.log('CHECK-FAIL android-engine-ini-exists — Config/Android/AndroidEngine.ini مفقود');
}

// Rok2.uproject سليم: لا AdditionalPluginDirectories (سبب آخر للخطأ في UE 5.x)
const up = JSON.parse(readFileSync(join(client, 'Rok2.uproject'), 'utf8'));
check('uproject-no-additional-plugin-dirs', !up.AdditionalPluginDirectories || up.AdditionalPluginDirectories.every(v => v),
  'لا يوجد AdditionalPluginDirectories بخلايا فارغة (سبب توثيقي لخطأ descriptor في UE 5.x)');

console.log(`\nP8-Descriptor: ${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
