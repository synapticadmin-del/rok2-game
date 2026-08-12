// P7-T12: فحص بنيوي لإصلاح تعليق التطبيق على شعار Unreal Engine عند فتح اللعبة على Android.
// يتحقق من:
// 1) OpenGLES3 أساسي على Android: rhi.AndroidDefaultGraphicsRHI=DefaultGraphicsRHI_OpenGLES3
// 2) bSupportsOpenGL3=True و bSupportsVulkan=True (Vulkan متاح كخيار لا إجبار)
// 3) bForceVulkan=False و bSplashScreen=False و bDisableOBBPakUI=True
// 4) PythonScriptPlugin و EditorScriptingUtilities معطلان في Rok2.uproject
//    (DisableEnginePluginsByDefault + DisabledPlugins) — plugin محرر/ويندوز لا يعمل على Android
// 5) WebSocketNetworking لا يزال مفعّلًا (اللعبة MMO تحتاجه)
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

check('gles3-default-rhi', /rhi\.AndroidDefaultGraphicsRHI\s*=\s*DefaultGraphicsRHI_OpenGLES3/i.test(iniSection),
  'يجب أن يكون RHI الافتراضي على Android هو OpenGLES3');
check('supports-opengl3-true', /bSupportsOpenGL3=True/i.test(iniSection), 'GLES3 يجب أن يكون مدعومًا');
check('supports-vulkan-true', /bSupportsVulkan=True/i.test(iniSection), 'Vulkan متاح اختياريًا');
check('force-vulkan-false', /bForceVulkan=False/i.test(iniSection), 'لا إجبار Vulkan');
check('splash-screen-off', /bSplashScreen=False/i.test(iniSection), 'شاشة البداية معطلة');
check('disable-obb-pak-ui', /bDisableOBBPakUI=True/i.test(iniSection), 'واجهة OBB الافتراضية معطلة (SDK 34)');

const uprojectPath = join(client, 'Rok2.uproject');
const up = JSON.parse(readFileSync(uprojectPath, 'utf8'));

check('uproject-disable-engine-plugins', up.DisableEnginePluginsByDefault === true,
  'DisableEnginePluginsByDefault يجب أن يكون true');
const disabled = (up.DisabledPlugins || []).map(p => p.Name);
check('python-script-disabled', disabled.includes('PythonScriptPlugin'),
  'PythonScriptPlugin (plugin محرر/ويندوز) معطل');
check('editor-scripting-disabled', disabled.includes('EditorScriptingUtilities'),
  'EditorScriptingUtilities (plugin محرر) معطل');
check('websocket-enabled',
  (up.Plugins || []).find(p => p.Name === 'WebSocketNetworking')?.Enabled === true,
  'WebSocketNetworking يجب أن يبقى مفعّلًا — اللعبة MMO');
check('target-platforms-android', (up.TargetPlatforms || []).includes('Android'),
  'Android في TargetPlatforms');

console.log(`\nP7-T12: ${pass} passed, ${fail} failed`);
process.exit(fail === 0 ? 0 : 1);
