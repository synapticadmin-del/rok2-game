/**
 * City / map / UI asset contract guard.
 *
 * Keeps the CC0 external mesh pack, original UI PNGs, import workflow, and
 * runtime-safe fallback contracts available without requiring Unreal Editor.
 */
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';
import { fileURLToPath } from 'node:url';

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const clientRoot = path.resolve(scriptDir, '..');
const pngMagic = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
// قائمة UIIcons الكاملة = الحارس السابق + المعرفات التي يتوقعها URok2ArtAssets::ImportedIds
const icons = [
  'alliance', 'bag', 'bell', 'build', 'food', 'gems', 'gold', 'heal',
  'hospital', 'mail', 'map', 'reports', 'research', 'settings', 'speed',
  'speedup', 'stone', 'train', 'upgrade', 'wood',
];
const buttons = ['danger_red', 'primary_gold', 'secondary_blue', 'success_green'];
const cityBuildings = [
  'castle', 'barracks', 'archery_range', 'smithy', 'lumbermill',
  'quarry', 'farm', 'market', 'tavern', 'academy',
];
const requiredMeshes = [
  'gate', 'metal-gate', 'tower-square', 'tower-hexagon-base', 'wall',
  'wall-doorway', 'wall-narrow-gate', 'bridge-straight', 'rocks-large',
  'tree-large', 'siege-ballista', 'siege-catapult', 'siege-ram',
  'siege-tower', 'siege-trebuchet',
];
let failures = 0;

function requireText(relativePath, needles) {
  const absolutePath = path.join(clientRoot, relativePath);
  if (!fs.existsSync(absolutePath)) {
    console.error(`FAIL missing text file: ${relativePath}`);
    failures += 1;
    return;
  }
  const text = fs.readFileSync(absolutePath, 'utf8');
  for (const needle of needles) {
    if (!text.includes(needle)) {
      console.error(`FAIL ${relativePath}: missing ${needle}`);
      failures += 1;
    }
  }
}

function requirePng(relativePath) {
  const absolutePath = path.join(clientRoot, relativePath);
  if (!fs.existsSync(absolutePath)) {
    console.error(`FAIL missing asset: ${relativePath}`);
    failures += 1;
    return;
  }
  const bytes = fs.readFileSync(absolutePath);
  if (bytes.length < pngMagic.length || !bytes.subarray(0, pngMagic.length).equals(pngMagic)) {
    console.error(`FAIL non-PNG binary asset: ${relativePath}`);
    failures += 1;
  }
}

for (const icon of icons) requirePng(`Content/Art/UIIcons/icon_${icon}.png`);
for (const button of buttons) requirePng(`Content/Art/UIButtons/button_${button}.png`);
for (const building of cityBuildings) requirePng(`Content/Art/CityBuildingIcons/building_${building}.png`);

for (const mesh of requiredMeshes) {
  const relativePath = `Content/Art/KenneyCastleKit/${mesh}.glb`;
  const absolutePath = path.join(clientRoot, relativePath);
  if (!fs.existsSync(absolutePath) || fs.statSync(absolutePath).size < 32) {
    console.error(`FAIL missing or invalid GLB asset: ${relativePath}`);
    failures += 1;
  }
}

requireText('Content/Art/KenneyCastleKit/LICENSE_KENNEY_CASTLE_KIT.txt', [
  'Kenney', 'Creative Commons Zero, CC0', 'commercial purposes',
]);
requireText('scripts/Import-CityMapUIAssets.ps1', [
  'UIIcons', 'UIButtons', 'CityBuildingIcons', 'CityBuildingPortraits', 'TextureFactory',
  'KenneyCastleKit', 'GLTFImportFactory', '-ImportMeshes', 'UnrealEditor-Cmd.exe',
]);
requireText('Source/Rok2/Public/Rok2ArtAssets.h', [
  'GetImportedUiIconAssetPath', 'GetIconBrush',
]);
requireText('Source/Rok2/Private/Rok2ArtAssets.cpp', [
  'GetImportedUiIconAssetPath', 'ImportedIds', '/Game/Art/UIIcons/icon_%s.icon_%s',
  'URok2IconLibrary::BrushFromArtAssets',
]);
requireText('Source/Rok2/Private/Rok2CityWidget.cpp', [
  'ApplyButtonSkin', '/Game/Art/UIButtons/%s.%s', 'button_primary_gold',
  // P17-T4: جلود الأزرار صارت عبر Rok2Surface::TexturedSkinButton التي تسقط
  // إلى SecondaryButton عند غياب الجلد — لا SetStyle(Style) اليدوية القديمة.
  'button_secondary_blue', 'TexturedSkinButton', 'ResolveCityBuildingArtId',
  '/Game/Art/CityBuildingIcons/building_%s.building_%s', 'LoadObject<UTexture2D>',
]);
requireText('../docs/CITY_MAP_UI_ASSET_BRIEF.md', [
  'Kenney Castle Kit', 'CC0', 'المدينة الداخلية', 'الخريطة', 'أزرار',
]);

if (failures > 0) {
  console.error(`\nCity/map/UI asset verification failed (${failures} issue(s)).`);
  process.exit(1);
}
console.log(`City/map/UI asset verification passed: ${icons.length} original PNG icons, ${buttons.length} button skins, ${cityBuildings.length} city building portraits, ${requiredMeshes.length} required CC0 GLB meshes, and runtime-safe import contracts found.`);
