import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  themeHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2VisualTheme.h'),
  themeSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2VisualTheme.cpp'),
  buildMenu: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2BuildMenuWidget.cpp'),
  buildingDetail: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2BuildingDetailWidget.cpp'),
  worldRenderer: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp'),
  visualDesign: path.join(repoRoot, 'design/01-visual/VISUAL_IDENTITY.md'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

// P17: صار السطح (خلفية/بطاقة/ورقة) يأتي من `Rok2Surface`، وهي مبنية على
// `Rok2Visual` — فالعقد نفسه محفوظ عبر طبقة واحدة إضافية. لذلك تقبل الفحوص
// أدناه أيّ المسارين: نداء الرمز مباشرة، أو مصنع السطح الذي يستهلكه.
// ما يبقى ممنوعاً هو اللوحة المحلية والقيمة الخام — وهذا ما تفحصه `forbidden`
// هنا و`verify_p17_design_system.mjs` بتوسّع.
const required = [
  ['themeHeader', /namespace Rok2Visual/, 'مساحة أسماء رموز الواجهة'],
  ['themeHeader', /FLinearColor\s*&\s*Gold\s*\(/, 'رمز الذهب'],
  ['themeHeader', /FLinearColor\s*&\s*Success\s*\(/, 'رمز النجاح'],
  ['themeHeader', /FLinearColor\s+CivilizationAccent\s*\(/, 'رمز لهجة الحضارة'],
  ['themeSource', /CivilizationAccent/, 'تنفيذ لهجة الحضارة'],
  ['buildMenu', /#include "Rok2VisualTheme\.h"/, 'ربط قائمة البناء بالهوية'],
  ['buildMenu', /Rok2Visual::(Panel|Scrim)\(\)|Rok2Surface::Sheet\(\)/, 'خلفية قائمة البناء الموحدة'],
  ['buildMenu', /Rok2Visual::Gold\(\)/, 'لهجة قائمة البناء الموحدة'],
  ['buildMenu', /Rok2Visual::Card\(\)|Rok2Surface::Card\(\)/, 'بطاقات البناء الموحدة'],
  ['buildingDetail', /#include "Rok2VisualTheme\.h"/, 'ربط تفاصيل المبنى بالهوية'],
  ['buildingDetail', /Rok2Visual::PrimaryAction\(\)/, 'زر الترقية الموحد'],
  ['buildingDetail', /Rok2Visual::Success\(\)/, 'حالة الموارد الموحدة'],
  ['worldRenderer', /#include "Rok2VisualTheme\.h"/, 'ربط خريطة العالم بالهوية'],
  ['worldRenderer', /Rok2Visual::Information\(\)/, 'تمييز منشأة الحليف على الخريطة'],
  ['worldRenderer', /Rok2Visual::Danger\(\)/, 'تمييز منشأة الخصم على الخريطة'],
  ['visualDesign', /Rok2VisualTheme/, 'توثيق نقطة الحقيقة البصرية'],
];

const forbidden = [
  ['buildMenu', /namespace Rok2BuildStyle/, 'لوحة قائمة بناء محلية مكررة'],
  ['buildingDetail', /namespace Rok2CardStyle/, 'لوحة تفاصيل مبنى محلية مكررة'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}
for (const [fileKey, pattern, label] of forbidden) {
  if (pattern.test(content[fileKey])) failures.push(`ممنوع: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد الهوية البصرية:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد الهوية البصرية اجتاز: قائمة البناء وتفاصيله تستعملان رموز Rok2Visual المشتركة.');
