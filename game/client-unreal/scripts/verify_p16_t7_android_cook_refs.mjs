#!/usr/bin/env node
/**
 * P16-T7 — حارس: لا مادة مشروع تعتمد على محتوى ملحق غير قابل للكوك على أندرويد.
 *
 * العطل الحقيقي الذي يمنعه:
 *   مستورد glTF (Interchange) يربط المادة المستوردة من KayKit بوالد داخل
 *   الملحق نفسه: /Interchange/gltf/MaterialInstances/MI_Default_Opaque.
 *   وInterchange.uplugin معلن SupportedTargetPlatforms = Win64/Linux/Mac فقط،
 *   فمحتواه غير قابل للكوك على أندرويد ويسقط كوك BuildCookRun بـ:
 *     LogCook: Error: Content is missing from cook.
 *       Source package: /Game/Art/kaykit/hexagons_medieval1
 *       Target package: /Interchange/gltf/MaterialInstances/MI_Default_Opaque
 *   النتيجة: لا APK إطلاقاً — 56 دقيقة كوك تنتهي بـ ExitCode=25.
 *
 * الفحوص:
 *   1. لا ملف .uasset تحت Content يذكر /Interchange/ (مرجع صريح داخل الحزمة).
 *   2. مادة المشروع البديلة M_Rok2Gltf موجودة كـ .uasset فعلي.
 *   3. سكربت إعادة الربط موجود ويعيد الربط إلى M_Rok2Gltf ويتحقق بعدها.
 *   4. ImportAssets.bat يشغّل إعادة الربط بعد الاستيراد (وإلا عاد العطل مع
 *      أول إعادة استيراد لأصول glTF).
 *   5. مادة المشروع تحمل نفس أسماء بارامترات مستورد glTF كي لا يُفقد النسيج
 *      ولا تتوقف صبغة الحضارة عبر BaseColorFactor.
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const CLIENT = path.resolve(here, '..');

const checks = [];
function expect(name, condition, detail = '') {
  checks.push({ name, ok: Boolean(condition), detail });
}
function read(rel) {
  return fs.readFileSync(path.join(CLIENT, rel), 'utf8');
}

function walk(dir, out = []) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) walk(full, out);
    else out.push(full);
  }
  return out;
}

// ---------------------------------------------------------------------------
// 1. لا حزمة مشروع تشير إلى محتوى Interchange
// ---------------------------------------------------------------------------
const contentDir = path.join(CLIENT, 'Content');
const offenders = [];
for (const file of walk(contentDir)) {
  if (!file.endsWith('.uasset') && !file.endsWith('.umap')) continue;
  const buffer = fs.readFileSync(file);
  if (buffer.includes('/Interchange/')) {
    offenders.push(path.relative(CLIENT, file));
  }
}
expect(
  'لا حزمة .uasset/.umap تشير إلى /Interchange/ (محتوى ملحق غير قابل للكوك على أندرويد)',
  offenders.length === 0,
  offenders.slice(0, 5).join(', '),
);

// ---------------------------------------------------------------------------
// 2. مادة المشروع البديلة موجودة فعلاً
// ---------------------------------------------------------------------------
expect(
  'مادة المشروع M_Rok2Gltf موجودة كـ .uasset',
  fs.existsSync(path.join(CLIENT, 'Content/Art/Materials/M_Rok2Gltf.uasset')),
);

// ---------------------------------------------------------------------------
// 3. سكربت إعادة الربط
// ---------------------------------------------------------------------------
const reparent = read('scripts/reparent_gltf_materials.py');
expect('سكربت إعادة الربط يبني M_Rok2Gltf', reparent.includes('MASTER_NAME = "M_Rok2Gltf"'));
expect(
  'إعادة الربط تستهدف كل مادة والدها خارج /Game و /Engine',
  reparent.includes('parent_path.startswith("/Game")')
    && reparent.includes('parent_path.startswith("/Engine")')
    && reparent.includes('set_editor_property("parent", master)'),
);
expect(
  'السكربت يتحقق بعد التعديل من خلو المشروع من مراجع غير قابلة للكوك',
  reparent.includes('def verify()') && reparent.includes('static_materials'),
);
expect(
  'مادة الأساس مُعلَّمة used_with_instanced_static_meshes (وإلا تُستبدل على HISM في بناء مُطبَّق)',
  reparent.includes('used_with_instanced_static_meshes'),
);

// ---------------------------------------------------------------------------
// 4. الاستيراد يشغّل إعادة الربط
// ---------------------------------------------------------------------------
const importBat = read('ImportAssets.bat');
const importIndex = importBat.indexOf('Importing raw assets');
const reparentIndex = importBat.indexOf('reparent_gltf_materials.py');
expect('ImportAssets.bat يشغّل reparent_gltf_materials.py', reparentIndex !== -1);
expect(
  'إعادة الربط تأتي بعد الاستيراد لا قبله',
  importIndex !== -1 && reparentIndex > importIndex,
);

// ---------------------------------------------------------------------------
// 5. أسماء البارامترات تطابق ما يضبطه مستورد glTF
// ---------------------------------------------------------------------------
for (const parameter of ['BaseColorTexture', 'BaseColorFactor', 'MetallicFactor', 'RoughnessFactor']) {
  expect(`مادة الأساس تعرّف بارامتر ${parameter} بنفس اسم مستورد glTF`, reparent.includes(parameter));
}
expect(
  'صبغة الحضارة تبقى ممكنة: TintExistingMaterialOn تبحث عن BaseColorFactor',
  read('Source/Rok2/Private/Rok2ProceduralAssets.cpp').includes('BaseColorFactor'),
);

// ---------------------------------------------------------------------------
// 6. سكربت التحزيم يكتب بلوك noCompress صالح لـ Groovy وبلا BOM
// ---------------------------------------------------------------------------
const buildScript = read('scripts/Build-Rok2.ps1');
expect(
  'بلوك noCompress يُكتب على أسطر مستقلة (السطر الواحد يُفسَّر سلسلة نداءات فيفشل بـ null object)',
  /androidResources \{\r?\n\s+noCompress/.test(buildScript),
);
expect(
  'بلوك noCompress يُكتب بلا BOM (Groovy يرفض المحرف الأول)',
  buildScript.includes('UTF8Encoding($false)'),
);

// ---------------------------------------------------------------------------
const failed = checks.filter((c) => !c.ok);
for (const c of checks) {
  console.log(`${c.ok ? 'PASS' : 'FAIL'} — ${c.name}${c.detail ? ` (${c.detail})` : ''}`);
}
console.log(`\nP16-T7: ${checks.length - failed.length} passed, ${failed.length} failed`);
process.exit(failed.length ? 1 : 0);
