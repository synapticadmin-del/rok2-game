# سجل أعطال البناء والـ PIE (Build & PIE Blocker Log)

> **الغرض:** هذا السجل يوثّق كل عطل blocker واجهته عملية بناء ROK2 أو اختبارها على Unreal Engine 5.4.4، مع سببه الجذري وإصلاحه الموثّق في الكود أو الأدلة. الهدف ليس إخفاء الأعطال، بل تحويل كل عطل متكرر إلى ضمان منع انحدار (سكربت تحقق، إصلاح في الكود، أو قيد موثّق في `BUILD_AND_PIE.md`).

> **الاتفاقية:** يُصنّف العطل **blocker** إذا منع بناء المحرر، أو تحزيم Win64/Android، أو تشغيل دخان Standalone، أو دورة PIE كاملة. الأعطال التي حُلّت داخل هذه الجلسة تُوثق هنا مع رقم الـ commit الذي أدخل الإصلاح.

## جدول السجل

| المعرّف | التاريخ | المكوّن | العطل | السبب الجذري | الإصلاح | Commit |
|---|---|---|---|---|---|---|
| BF-001 | 2026-08-12 | Android / Gradle | البناء يفشل فور بدء تحزيم Android: `Unsupported class file major version 65` | UE 5.4 يحزم Gradle 7.6.3 الذي لا يفهم class files من Java 21، وAndroid Studio JBR يُصدّر Java 21 عبر `JAVA_HOME` | `Build-Rok2.ps1` يوجّه `JAVA_HOME` إلى Microsoft JDK 17 (`C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot`) قبل استدعاء `Build.bat` على منصة Android فقط | `f731dc9` |
| BF-002 | 2026-08-12 | Android / Manifest | طلب تثبيت Android يطلب صلاحية تخزين ولا تظهر في الإعدادات | Manifest يتطلب صلاحيات تخزين خارجية قديمة لا يطلبها التطبيق فعلًا | `ManifestRequirementsOverride.txt` يحذف صلاحيات `WRITE_EXTERNAL_STORAGE` الزائدة مع `bUseExternalFilesDir=True` في `DefaultEngine.ini` | `9df9abf` / `b923aad` |
| BF-003 | 2026-08-12 | Android / Splash | اللعبة تتجمد على شاشة Unreal Engine الافتراضية قبل الدخول | Splash Screen الافتراضي مع تعارضات OpenGLES/RHI على عتاد Android قديم | `bSplashScreen=False` + `bDisableOBBPakUI=True` في `DefaultEngine.ini`، ورسمياً `+MobileProvisionRenderer=OpenGLES3` | `d86588f` |
| BF-004 | 2026-08-13 | Android / JavaLibs | فشل Android build بسبب غياب صنف `com.google.vr.sdk.base.PermissionHelper` المطلوب من UPL | UE Android UPL يستدعي PermissionHelper ولا تُبنى GoogleVR أوتوماتيكيًا | Stub محلي `Build\Android\JavaLibs\vrpermissionstub` يُنسخ إلى `Intermediate\Android\arm64\JavaLibs` قبل أي بناء Android | `f731dc9` (ضمن نفس الدفعة) |
| BF-005 | 2026-08-12 | المحرك / الإصدار | سكربتات البناء قد تستهلك UE بنسخة مختلفة فيعطب التحزيم بصمت | افتراض مسار افتراضي واحد وغياب تحقق نسخة | كل سكربت (Build / Import / Smoke / Prepare-Android) يقرأ `Engine\Build\Build.version` ويرفض أي نسخة غير `5.4.4` | `f731dc9` (P7-T2 gate) |
| BF-006 | 2026-08-12 | المحرك / الشاشة السوداء | شاشة سوداء أو تشويش عند فتح المحرر بعد تغيير إعدادات العرض | تهيئة RHI/Shader Cache بعد تغيير Renderer أو إعدادات Graphics | `RunEditor_SafeMode.bat` + `Docs/TROUBLESHOOT_BLACK_SCREEN.md`؛ لا تعتمد `-Clean` كعلاج أول | `ad2490a` |
| BF-007 | 2026-08-13 | Android / Manifest | شاشة "Permission Required — Storage" معلقة بعد البناء ولا يظهر بند الصلاحية في الإعدادات أصلًا | UE 5.4 يتجاهل `ManifestRequirementsOverride.txt` بصمت في بعض مسارات التوليد، فيضيف `READ/WRITE_EXTERNAL_STORAGE` تلقائيًا عند فحص OBB؛ `bUseExternalFilesDir=True` وحده لا يكفي | `bEnableManifestRequirements=True` في `[AndroidRuntimeSettings]` بالملفين `DefaultEngine.ini` و`Config/Android/AndroidEngine.ini` يُلزم UE بقائمة Override حرفيًا (مصدر: UE forums 386108 + r/vrdev Solved 2025)؛ ترقية حارس P7-T11 إلى 23 فحصًا | قيد الإغلاق مع commit |

## قواعد التعامل مع عطل جديد

1. **أوقِف**: لا تدفع دفعة جديدة قبل توثيق العطل هنا، حتى لو كان الفحص البنيوي عابرًا.
2. **عزل**: نفّذ `Run-Rok2Preflight.ps1` أولًا — كثير من الأعطال الظاهرة في الدخان أو PIE أصلها بيئة (نسخة محرك، أدوات بناء، إعدادات) وليس كودًا؛ إن فشل العزل، شغّل `Run-Rok2RuntimeSmoke.ps1` — فإن ظهر العطل في دخان Standalone فهو blocker للمحرر أيضًا.
3. **سجّل**: افتح صفًا جديدًا في الجدول أعلاه بمعرف `BF-00N` متسلسل وخصّص له ملف السجل من `Saved\BuildLogs\` أو Output Log.
4. **أصلح من جذره**: إصلاح العطل يتضمن على الأقل واحدًا من: (أ) تعديل سكربت يمنع تكراره، (ب) قيد موثّق في `BUILD_AND_PIE.md`، (ج) بند PLAN جديد إن تطلب تغييرًا في العميل.
5. **امنَع الانحدار**: أضف فحصًا إلى `verify_p7_t2_build_pipeline.mjs` إن كان الإصلاح يتعارض مع عقد موثّق (مثل `-package Android` بعد إصلاح Java).
6. **لا تحذف الصفوف**: يظل السجل مرجعًا تاريخيًا؛ أخطأت في التصنيف؟ صحّح بصف جديد ولا تحذف.

## الأعطال المعروفة غير المحسومة (انتظار بيئة UE 5.4.4 على Windows)

- **PIE الكامل P7-T1**: المعايير PIE-01 إلى PIE-11 تتطلب عميلين UE 5.4.4 فعليين؛ لا يوجد في البيئة الحالية محرك أو جهاز Android، فلا يُقبل إغلاق P7-T1 أو P7-T3 قبل أدلة بصرية مرفقة في تقرير المراجعة.
- **شاشات PIE**: أي مكوّن UMG جديد (بطاقة الحضارة، حكاية المملكة) قد يحتاج إعادة تحميل الـ Hot Reload أو إعادة بناء المحرر؛ وثّق ذلك كـ BF جديد إن ثبت.
- **Android APK على الجهاز**: بعد إصلاح BF-001/002/003 يظل قبول البناء على جهاز Android فعلي شرطًا لإغلاق P7-T2 النهائي.

## المراجع الداخلية

- دليل البناء والدخان وPIE: [`BUILD_AND_PIE.md`](BUILD_AND_PIE.md)
- فحص بوابة العقود: `game/client-unreal/scripts/verify_p7_t2_build_pipeline.mjs`
- فحص قبول PIE/Android: `game/client-unreal/scripts/verify_pie_android_acceptance.mjs`
