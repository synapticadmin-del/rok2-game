---
name: rok2-visual-ui-parity
description: أسلوب عمل P25 لبناء واجهات ROK2 على شكل Rise of Kingdoms — مرجع بصري أولاً، ثم بناء، ثم التقاط الشاشة والنظر إليها فعلاً، ثم مقارنة وتكرار، والحارس البنيوي أخيراً. استخدمها لأي بند يمسّ واجهة في `game/client-unreal` (شاشة جديدة، إصلاح شكل، شكوى «الواجهة لا تشبه RoK» أو «صفحات جرداء» أو «عناصر غير ظاهرة» أو تراكب/قصّ/حجم خاطئ)، ولأي بند P24/P25، وقبل تعليم أي بند واجهة `[x]`. Use for any ROK2 Unreal UMG widget work that must visually match Rise of Kingdoms: reading the RoK reference from ROK_Wiki_Assets, capturing the running client window, visual comparison, and only then writing structural guards.
---

# ROK2 — تماثل الواجهة البصري مع Rise of Kingdoms (أسلوب P25)

## المشكلة التي وُجدت هذه المهارة لأجلها

جلسات سابقة كتبت حرّاساً بنيوية (تطابق نصّي في `.cpp`) وأبلغت **«45 فحصاً، 0 فشل»** على شاشة كانت أزرارها تركب فوق نصّها وصورة قائدها شريحة مسحوقة على الحافة. الحارس يثبت أن `SetWidthOverride` مكتوب؛ **لا يرى** أن الناتج مكسور.

القاعدة الناتجة: **الحارس يمنع الانحدار، ولا ينوب عن النظر.** ولا يُكتب حارس قبل رؤية اللقطة.

---

## الخطوات الخمس (بهذا الترتيب، بلا تخطٍّ)

### 1. اقرأ مرجع RoK المقابل — قبل أي كود

المصادر بالترتيب:

| المصدر | ما فيه |
|--------|--------|
| `C:\Users\kayf\Desktop\ROK_Wiki_Assets` | **1,487 أصلاً حقيقياً** من RoK في 12 فئة (43MB) + `assets_manifest.json` بأبعاد كل ملف |
| `07-game-design/assets/*.jpg` | 11 لقطة مرجعية — **مرمَّزة base64 لا JPEG**، انظر §«فكّ الترميز» |
| `07-game-design/ui-ux-design-system.md` | مواصفة الشاشات والقواعد الصارمة (§8) |

الفئات الأكثر نفعاً: `09_UI_Frames_HUD_and_Icons` (77 إطاراً وأيقونة)، `11_Artwork_Wallpapers_and_Screenshots` (لقطات لعب فعلية)، `02_Buildings_and_City_Structures` (63 مبنى)، `07_Items_Chests_and_Resources` (149 عنصراً).

اقرأ لوحة تجميعية لا ملفاً ملفاً:

```bash
python .agents/skills/rok2-visual-ui-parity/scripts/build_contact_sheet.py 09_UI_Frames_HUD_and_Icons
```

ثم `Read` على المسار الذي يطبعه. **ونمط RoK البصري في `references/rok_visual_language.md` — اقرأه قبل أول بند واجهة في الجلسة.**

### 2. ابنِ الشاشة

القيود الملزمة (من `AGENTS.md` و`ui-ux-design-system.md`):

- الأسطح من `Rok2Surface`، الألوان من `Rok2Visual`، الخطوط من `URok2Typography` — لا `FLinearColor` خام ولا `SetBrushColor` ولا حجم خط يدوي
- الخادم هو السلطة: لا حساب ولا قيمة توازن في العميل
- **أخطاء UMG المتكرّرة في هذا المشروع: `references/umg_layout_traps.md`** — اقرأه عند أي تراكب أو قصّ أو حجم خاطئ. أهمها أن `SetDesiredSizeOverride` **لا يُخزَّن في `UImage`**.

### 3. التقط الشاشة وانظر إليها

```bash
# شغّل العميل (خلفية)، انتظر ~75s حتى تُبنى الشاشة
game/client-unreal/scripts/Launch-Rok2Client.ps1

# التقط
powershell -NoProfile -ExecutionPolicy Bypass \
  -File game/client-unreal/scripts/Capture-Rok2Window.ps1 -Name before_01
```

ثم `Read` على `%TEMP%\rokshots\before_01.png`.

**لا تستخدم `CopyFromScreen`** — يلتقط ما هو معروض على الشاشة، فأي نافذة تعلو اللعبة تُصوَّر مكانها (حدث فعلاً: عاد مستكشف الملفات بدل اللعبة). السكربت يستخدم `PrintWindow` بعلم `PW_RENDERFULLCONTENT`.

### 4. قارن وكرّر

ضع اللقطة بجوار المرجع واسأل عن **الملاحظات القابلة للقياس** لا الانطباع:

- تراكب: هل يعلو عنصرٌ عنصراً؟ (سببه غالباً ارتفاع ثابت أصغر من المحتوى + `Fill`)
- قصّ: هل يُبتر محتوى عند حدٍّ؟ (لوحٌ ثابت بلا `ScrollBox`)
- نسبة: هل صورةٌ مسحوقة أو منتفخة؟ (`ImageSize` بمقاس الملف)
- إطار: هل كل أيقونة داخل إطار كـRoK، أم رمز مسطّح عائم؟
- قِران: هل كل رقم مقرون بأيقونته؟ (§8.2)

كرّر 2→3→4 حتى تشابه المرجع. **اعرض على المستخدم لقطة «قبل/بعد»** — حكمه على صورة لا على وصف.

### 5. الحارس البنيوي — أخيراً

الآن فقط: حارس في `game/client-unreal/scripts/verify_*.mjs` + job في سلسلة `check` عبر `chainRuns` من `scripts/lib/npm_script_chain.mjs`.

**واجعل الحارس يقيس ما يستطيع قياسه حقاً:**
- خصائص الصور (شفافية، أبعاد، رقعة مطبوعة) تُقاس **من بايتات الملف** — انظر `scripts/measure_png.py` ونمط فكّ PNG في `verify_p24_t9_civ_select.mjs`
- ثوابت التخطيط المسمّاة تُفحص نصياً، وهذا مقبول **لأن اللقطة أثبتت أن القيمة صحيحة**
- استخرج أجسام الدوال بموازنة الأقواس (`fnBody`) لا بنافذة محارف ثابتة — النافذة تكذب عند إدراج سطر

---

## بروتوكول التسليم

بعد الخطوة 5: بناء `Build-Rok2.ps1 -Target Editor` (EXIT=0) → `cd game/backend && npm run check:fast && npm run check:ue-contracts` → تحديث `PLAN.md` (بند + سجل إنجاز) + وثيقة في `game/docs/` → commit على `main`.

**والتحفّظ يُكتب صريحاً:** إن لم يُختبر على جهاز أندرويد أو لم تُجرَّب الدورة على خادم حي، قُل ذلك. «لقطة على ويندوز» ليست «قبول PIE».

---

## أخطاء بيئية موثّقة (تكرّرت فعلاً)

| العطل | السبب والحل |
|-------|-------------|
| سكربت PowerShell يسقط بأخطاء تحليل على نصّ عربي | بلا UTF-8 BOM يقرأ PS 5.1 بترميز الصفحة المحلية. أضف BOM لكل `.ps1` فيه عربية |
| `"...هو $Var: $Other"` خطأ مرجع متغير | النقطتان تجعلانه مرجعاً مؤهَّلاً بمحرك أقراص. استخدم `${Var}` |
| `-run=ImportAssets` يعيد exit 1 مع نجاح كامل | يقرأ مجموعة فارغة إضافية (`Invalid Destination Path ()`). البوابة = **عدّ `.uasset` الناتجة** لا شفرة الخروج |
| `Error Code 32` أثناء الاستيراد | المحرر يمسك الملف. **أغلق المحرر قبل الاستيراد**، وتحقق من تواريخ الأصول بعده |
| `Rok2.uproject` يتغيّر بعد الاستيراد | الـcommandlet يكتب فيه فيُفشل `verify_p7_t12_splash_hang`. `git checkout --` عليه وأعد الفحص |
| ملف غير أصل داخل `Content/` | المحرر يحاول استيراده (فتح حوار DataTable على `alpha_report.json`). التقارير في `game/docs/` |
| `Rok2.exe` المبنيّ يسقط: `Failed to initialize ShaderCodeLibrary` | المشروع غير مطبوخ. شغّل عبر `UnrealEditor.exe <uproject> -game` |
| نصّ base64 في `07-game-design/assets/*.jpg` | ليست JPEG. `scripts/decode_reference_images.py` يفكّها |
