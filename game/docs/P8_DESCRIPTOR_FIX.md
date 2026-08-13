# P8-Descriptor: إصلاح خطأ `Failed to open descriptor file ../../Rok2/Rok2.uproject` على Android

**التاريخ:** 2026-08-13 | **الحالة:** مطبق ومُتحقق منه (حارس `test:p8-descriptor-fix` في `npm run check`)

## وصف المشكلة

بعد بناء التطبيق للاندرويد، يظهر عند بدء التشغيل على الجهاز مربع رسالة:

> Failed to open descriptor file ../../Rok2/Rok2.uproject

وعند الضغط على OK يعلق التطبيق أو يغلق، فلا يتجاوز شعار Unreal Engine.

## السبب الجذري

عند بدء التشغيل على Android، يبحث UE 5.4 عن ملف وصف المشروع (`.uproject` — يسمى descriptor) على الجهاز نفسه، عبر **Android File Server (AFS)**، باستخدام مسار نسبي من مجلد تنفيذ الـ APK. المسار `../../Rok2/Rok2.uproject` يعني أن UE صعد من مجلد بيانات التطبيق (`Android/data/com.rok2.thrones/files/UE4Game/.../Cooked`) بحثًا عن ملف غير موجود أصلًا — لأن بيانات اللعبة مغلّفة داخل APK (`bPackageDataInsideApk=True`) ولا يُحمَّل أي descriptor مع packaged build. فشل هذا البحث يعرض الرسالة ويعلّق التطبيق.

هذه المشكلة موثقة على نطاق واسع في مجتمع UE لـ UE 5.x + SDK 34:

- [Failed to open descriptor file (android) — Epic Forums](https://forums.unrealengine.com/t/failed-to-open-descriptor-file-android/467113)
- [UE5 Android game Failed to open descriptor file — Epic Forums](https://forums.unrealengine.com/t/unreal-engine-5-android-game-failed-to-open-descriptor-file/582207)
- [[Resolved] UE5.1 Long Android deployment — GameDev.tv](https://community.gamedev.tv/t/resolved-ue5-1-long-android-deployment-and-failed-to-open-descriptor-file/222767)

في مشروعنا كان **Android File Server مفعّلًا** (`bEnablePlugin=True` في `Config/DefaultEngine.ini` داخل قسم `[AndroidFileServerEditor.AndroidFileServerRuntimeSettings]`) — وهذا هو المسار الأكثر توثيقًا لظهور الخطأ (توصية مطور Epic staff ryanjon2040: تعطيل AFS).

## الإصلاح المطبق (دفاع متعدد الطبقات)

| # | الإعداد | الملف | القيمة | الدور |
|---|---------|-------|--------|-------|
| 1 | Android File Server | `Config/DefaultEngine.ini` | `bEnablePlugin=False` | تعطيل السبب الجذري — AFS كان مفعّلًا بحثًا عن descriptor |
| 2 | `bDetectIfAppShouldRun` | `Config/DefaultEngine.ini` + `Config/Android/AndroidEngine.ini` | `False` | إيقاف فحص وجود descriptor على الجهاز نهائيًا |
| 3 | `bPackageDataInsideApk` | نفس القسمين | `True` | البيانات داخل APK فلا حاجة لـ descriptor على الجهاز أصلًا |
| 4 | `AdditionalPluginDirectories` | `Rok2.uproject` | غير موجود | سبب توثيقي آخر للخطأ في UE 5.x (خلايا فارغة) |

نُفذ الحارس `scripts/verify_p8_descriptor_fix.mjs` (6 عقود): يتحقق من (1) `bDetectIfAppShouldRun=False` و(2) `bPackageDataInsideApk=True` في القسمين، (3) `bEnablePlugin=False` في AFS، (4) تطابق `Config/Android/AndroidEngine.ini`، (5) عدم وجود `AdditionalPluginDirectories` بخلايا فارغة. أُدرج في `npm run check` باسم `test:p8-descriptor-fix` قبل بوابة قبول PIE.

## خطوات المستخدم المطلوبة على الجهاز (ضرورية)

إصلاح ini وحده لا يكفي إذا بقيت نسخة APK قديمة على الجهاز: بقايا **Install Location** القديمة تحفظ المسار الخاطئ:

1. **احذف APK القديم من الجهاز مع بياناته**: إعدادات الهاتف → التطبيقات → Rok2 → إلغاء التثبيت (وفي بعض الأجهزة: مسح البيانات أولًا).
2. احذف مجلدات `Binaries/` و`Intermediate/` و`Saved/` من `game/client-unreal/` (اختياري لكن موصى به لحزم نظيف).
3. أعِد البناء: `pwsh scripts/Build-Rok2.ps1 -Target Development -Platform Android -Package`
4. ثبّت الـ APK الجديد وشغّله — يجب أن يعبر الشاشة البيضاء直接进入 الواجهة الرئيسية (Splash → Rok2Main).

## ملاحظات

- إذا ظهرت المشكلة مرة أخرى بعد هذا الإصلاح رغم إلغاء التثبيت، فسبب ثانوي محتمل هو أذونات التخزين على Android 11+ (`MANAGE_EXTERNAL_STORAGE`) — وهو سبب منفصل عن خطأ descriptor، وواجهته السابقة مغطاة بحارس P7-T11.
- التحقق الفعلي على UE 5.4.4 Windows + جهاز Android يظل شرط قبول نهائي خارج بيئة التطوير الحالية (Linux) — موثق كتحفظ قبول في PLAN.md.
