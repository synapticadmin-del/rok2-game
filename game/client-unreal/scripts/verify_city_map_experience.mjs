import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(scriptDir, '../../..');

const files = {
  citySave: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2CityLayoutSaveGame.h'),
  cityLayoutHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2CityLayoutActor.h'),
  cityLayoutSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2CityLayoutActor.cpp'),
  buildingHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2BuildingActor.h'),
  buildingSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2BuildingActor.cpp'),
  detailHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2BuildingDetailWidget.h'),
  detailSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2BuildingDetailWidget.cpp'),
  cameraHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2IsometricCamera.h'),
  cameraSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2IsometricCamera.cpp'),
  viewHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2ViewManager.h'),
  viewSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2ViewManager.cpp'),
  worldHeader: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Public/Rok2WorldRenderer.h'),
  worldSource: path.join(repoRoot, 'game/client-unreal/Source/Rok2/Private/Rok2WorldRenderer.cpp'),
  contract: path.join(repoRoot, 'design/04-world-map/CITY_AND_MAP_EXPERIENCE_CONTRACT.md'),
};

const content = Object.fromEntries(
  await Promise.all(Object.entries(files).map(async ([name, filename]) => [name, await readFile(filename, 'utf8')]))
);

const required = [
  ['citySave', /URok2CityLayoutSaveGame/, 'حاوية حفظ تخطيط المدينة'],
  ['citySave', /FRok2BuildingPlacement/, 'موضع المبنى وواجهته المحفوظان'],
  ['cityLayoutHeader', /SetBuildingFacade/, 'أمر تغيير واجهة المبنى'],
  ['cityLayoutHeader', /SaveLayoutToServer/, 'نقطة حفظ التخطيط'],
  ['cityLayoutSource', /city_hall/, 'نواة القلعة الثابتة'],
  ['cityLayoutSource', /LoadLocalLayout/, 'استعادة التخطيط المحلي'],
  ['cityLayoutSource', /SaveGameToSlot/, 'حفظ التخطيط المحلي'],
  ['buildingHeader', /ERok2BuildingFacade/, 'تعداد واجهات المباني'],
  ['buildingSource', /city_core/, 'تمييز القلعة المركزية'],
  ['detailHeader', /OnFacadeClicked/, 'حدث زر الواجهة'],
  ['detailSource', /SetBuildingFacade/, 'ربط زر الواجهة بالتخطيط'],
  ['cameraHeader', /SetTargetZoomDistance/, 'هدف تكبير الكاميرا'],
  ['cameraHeader', /CurrentDistance/, 'مسافة تكبير مرئية مستقلة'],
  ['cameraSource', /FInterpTo\(CurrentDistance/, 'تنعيم مسافة التكبير'],
  ['viewHeader', /ERok2ViewTransition/, 'حالة انتقال العرض'],
  ['viewSource', /BeginTransition/, 'بدء انتقال العرض'],
  ['viewSource', /FinishTransition/, 'إكمال انتقال العرض'],
  ['viewSource', /LastMapZoomDistance/, 'استعادة تكبير الخريطة'],
  ['worldHeader', /ERok2WorldZoomLayer/, 'تعداد طبقات الخريطة'],
  ['worldHeader', /TacticalZoomMaxDistance/, 'حد الطبقة التكتيكية'],
  ['worldSource', /UpdateZoomLayer/, 'حساب طبقة الخريطة'],
  ['worldSource', /ApplyZoomLayerVisibility/, 'تطبيق رؤية طبقات الخريطة'],
  ['worldSource', /if \(!IsTacticalLayer\(\)\) continue;/, 'إخفاء العقد عند التصغير'],
  ['worldSource', /if \(!IsRegionalOrCloserLayer\(\)\) continue;/, 'إخفاء الممرات والمسيرات في عرض المملكة'],
  ['contract', /طبقات خريطة العالم حسب التكبير/, 'توثيق قواعد طبقات التكبير'],
  ['contract', /القلعة المركزية/, 'توثيق نواة القلعة'],
];

const failures = [];
for (const [fileKey, pattern, label] of required) {
  if (!pattern.test(content[fileKey])) failures.push(`مفقود: ${label} في ${files[fileKey]}`);
}

if (failures.length) {
  console.error('فشل فحص عقد تجربة المدينة والخريطة:\n- ' + failures.join('\n- '));
  process.exit(1);
}

console.log('✓ فحص عقد تجربة المدينة والخريطة اجتاز: القلعة والتخصيص والانتقال وطبقات التكبير متصلة.');
