#!/usr/bin/env node
/**
 * P7-T2 — Build and PIE pipeline contract.
 * Verifies that the documented local Windows pipeline continues to target the
 * real Unreal project, its editor/game targets, its startup map, and P7-T1
 * runtime evidence. It intentionally does not claim that an Unreal build ran.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const CLIENT = path.resolve(here, '..');

function read(relativePath) {
  return fs.readFileSync(path.join(CLIENT, relativePath), 'utf8');
}

const checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}
function includesAll(text, snippets) {
  return snippets.every((snippet) => text.includes(snippet));
}

const build = read('scripts/Build-Rok2.ps1');
const civImport = read('scripts/Import-CivVisuals.ps1');
const cityMapUiImport = read('scripts/Import-CityMapUIAssets.ps1');
const smoke = read('scripts/Run-Rok2RuntimeSmoke.ps1');
const preflight = read('scripts/Run-Rok2Preflight.ps1');
const guide = read('Docs/BUILD_AND_PIE.md');
const failuresLog = read('Docs/BUILD_FAILURES.md');
const pieQuick = read('Docs/PIE_QUICK_SCENARIO.md');
const defaultEngine = read('Config/DefaultEngine.ini');
const project = JSON.parse(read('Rok2.uproject'));

expect('ملف المشروع يعرّف وحدة Rok2', project.Modules?.some((module) => module.Name === 'Rok2'));
expect('EngineAssociation مثبت على سلسلة UE 5.4 الخاصة بـ 5.4.4', project.EngineAssociation === '5.4', project.EngineAssociation);
expect('خريطة البدء Rok2Main محددة في الإعدادات', defaultEngine.includes('GameDefaultMap=/Game/Maps/Rok2Main.Rok2Main'));

expect(
  'سكربت البناء يستهدف Rok2Editor الحقيقي',
  includesAll(build, ["'Rok2Editor'", "'Rok2'", 'Build.bat', 'Rok2.uproject'])
);
expect(
  'سكربت البناء يدعم تحزيم Win64 عبر UAT',
  includesAll(build, ['RunUAT.bat', "'BuildCookRun'", "'-platform=Win64'", "'-archive'"])
);
expect(
  'سكربت البناء يطلب UE_ROOT أو EngineRoot ولا يفترض مساراً واحداً',
  includesAll(build, ['$env:UE_ROOT', '-EngineRoot', 'Resolve-UnrealEngineRoot', "'5.4.4'", 'Build.version'])
);
expect(
  'سكربت البناء ينشئ سجلات قابلة للمراجعة',
  includesAll(build, ['Saved\\BuildLogs', 'build-$Target-$Timestamp.log', 'ImportCityMapUiAssets'])
);

expect(
  'اختبار الدخان يشغّل Standalone لا يدّعي PIE على UE 5.4.4',
  includesAll(smoke, ["'-game'", 'Standalone runtime smoke', '-dx11', '-sm5', "'5.4.4'", 'Build.version'])
);
expect(
  'سكربتا الاستيراد يرفضان محركاً غير UE 5.4.4',
  includesAll(civImport, ["'5.4.4'", 'Build.version'])
    && includesAll(cityMapUiImport, ["'5.4.4'", 'Build.version'])
);
expect(
  'اختبار الدخان يتحقق من Rok2Main ومن أخطاء قاتلة',
  includesAll(smoke, ['Rok2Main', '$FatalSignatures', 'Fatal error:', 'Unhandled Exception:'])
);
expect(
  'اختبار الدخان يملك حد وقت وإيقافاً تلقائياً',
  includesAll(smoke, ['$TimeoutSeconds', 'Stop-Process', '$KeepOpen'])
);

expect(
  'الدليل يشرح بناء المحرر ثم دخان التشغيل',
  includesAll(guide, ['Build-Rok2.ps1 -Target Editor', 'Run-Rok2RuntimeSmoke.ps1', 'Saved\\BuildLogs\\', 'UE 5.4.4', 'Visual Studio 2022 17.8'])
);
expect(
  'الدليل يصرح بأن الدخان ليس بديلاً عن PIE',
  guide.includes('ليس بديلاً عن Play-In-Editor')
);
expect(
  'الدليل يحتوي قائمة أدلة P7-T1 كاملة',
  includesAll(guide, ['PIE-01', 'PIE-02', 'PIE-03', 'PIE-04', 'PIE-05', 'PIE-06', 'PIE-07', 'PIE-08'])
);
expect(
  'الدليل يغطي الصوت والأيقونات وحكاية المملكة',
  includesAll(guide, ['UiButtonClick', 'حكاية المملكة', 'URok2WorldIconography', 'season_story_event'])
);
expect(
  'سكربت الجاهزية Preflight يتحقق من نسخة المحرك وأدوات البناء والمشروع',
  includesAll(preflight, ['Build.version', 'Build.bat', 'RunUAT.bat', 'UnrealEditor-Cmd.exe', 'Rok2.uproject', 'Rok2Main.umap', 'GameDefaultMap=/Game/Maps/Rok2Main', 'vswhere', '$env:UE_ROOT'])
);
expect(
  'سكربت الجاهزية يفحص VS 2022 وWindows SDK وJDK 17 ويقرر بexit code',
  includesAll(preflight, ['Microsoft.Component.MSBuild', '10.0.22621', 'exit 1', 'JDK 17', 'bSplashScreen=False', 'Saved\\BuildLogs'])
);
expect(
  'سجل الأعطال يوثّق BF-001 حتى BF-006 بقواعد تعامل وصفوف موثقة',
  includesAll(failuresLog, ['BF-001', 'BF-002', 'BF-003', 'BF-004', 'BF-005', 'BF-006', 'Blocker Log', 'major version 65', 'bSplashScreen=False', 'PermissionHelper', 'Run-Rok2Preflight.ps1'])
);
expect(
  'سجل الأعطال يحظر حذف الصفوف ويطلب فحص منع انحدار',
  includesAll(failuresLog, ['لا تحذف الصفوف', 'verify_p7_t2_build_pipeline.mjs'])
);
expect(
  'سيناريو PIE القصير يغطي ست خطوات وجدول تسجيل ومعرفات الأدلة',
  includesAll(pieQuick, ['New Editor Window (PIE)', 'UiButtonClick', 'UiPanelOpen', 'season_story_event', 'RestoreAuthoritativeState', 'P7-T2-PIE-SMOKE', 'PIE-06', 'PIE-11'])
);
expect(
  'الدليل يشير إلى Preflight وسجل الأعطال وسيناريو PIE القصير',
  includesAll(guide, ['Run-Rok2Preflight.ps1', 'BUILD_FAILURES.md', 'PIE_QUICK_SCENARIO.md', 'BF-001'])
);

for (const check of checks) {
  console.log(`${check.ok ? 'PASS' : 'FAIL'} — ${check.name}${check.detail ? ` (${check.detail})` : ''}`);
}

const failed = checks.filter((check) => !check.ok);
if (failed.length > 0) {
  console.error(`\nP7-T2 failed: ${failed.length}/${checks.length} build-pipeline contracts are missing.`);
  process.exitCode = 1;
} else {
  console.log(`\nP7-T2 passed: ${checks.length} build-pipeline contracts verified.`);
}
