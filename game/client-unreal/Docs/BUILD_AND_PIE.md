# بناء واختبار ROK2 على Unreal Engine 5

> **الغرض:** هذا الدليل هو مسار التشغيل المعتمد لبناء عميل ROK2 على Windows، والتحقق من إقلاعه، ثم تنفيذ اختبار **Play-In-Editor (PIE)** الذي يثبت تكاملات واجهة المرحلة P7-T1. اختبار الدخان المستقل **ليس بديلاً عن Play-In-Editor (PIE)**، لأن عناصر UMG والصوت والتفاعل مع عالم المحرر يجب تأكيدها بصرياً وسمعياً.

| بند | القيمة |
|---|---|
| ملف المشروع | `game/client-unreal/Rok2.uproject` |
| هدف المحرر | `Rok2Editor Win64 Development` |
| هدف العميل | `Rok2 Win64 Development` أو `Shipping` |
| خريطة البدء | `/Game/Maps/Rok2Main` |
| المحرك المعتمد | **Unreal Engine 5.4.4**؛ `EngineAssociation` مضبوط على `5.4` ليطابق مسار Epic Launcher `UE_5.4` |
| مسار العرض الافتراضي | DX11 / Shader Model 5 لتوافق أفضل مع العتاد الضعيف |
| بوابة العقود البرمجية | `cd game/backend && npm run check` |

## 1. المتطلبات المحلية

ثبّت **Unreal Engine 5.4.4** من Epic Games Launcher. استخدم **Visual Studio 2022 17.8 أو أحدث** مع workload **Game development with C++** وMSVC `14.38+` وWindows SDK `10.0.22621.0+`. هذه توافقات UE 5.4 الموصى بها من Epic [1]. إذا كان المحرك في مسار غير افتراضي، عيّن المتغير التالي في PowerShell قبل كل جلسة:

```powershell
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.4'
```

يمكن تمرير المسار صراحةً إلى كل سكربت عبر `-EngineRoot`. لا تحفظ مساراتك الشخصية أو مفاتيح الوصول في ملفات المشروع.

## 1.5 فحص الجاهزية الموحد (Preflight)

قبل أي بناء، نفّذ فحص الجاهزية الذي يجمع كل المتطلبات في تقرير واحد بصيغة PASS/FAIL/WARN دون بناء أو تشغيل أي شيء. يشمل فحص `Build.version = 5.4.4`، وأدوات البناء (`Build.bat`/`RunUAT.bat`/`UnrealEditor-Cmd.exe`)، وملفات المشروع (`Rok2.uproject`، وحدة `Rok2`، خريطة `Rok2Main.umap`)، و`GameDefaultMap` وحالة SplashScreen (P7-T12)، وVisual Studio 2022 17.8+ وWindows SDK عبر `vswhere`، وJDK 17 (شرط بناء Android — BF-001):

```powershell
.\scripts\Run-Rok2Preflight.ps1 -EngineRoot $env:UE_ROOT
```

يرفض السكربت البناء (exit 1) عند أي FAIL ويشرح الإجراء المطلوب في عمود الملاحظات؛ تحذيرات WARN (مثل غياب JDK 17) لا تمنع بناء Win64 لكنها تحذر من فشل محتمل.

## 2. بناء المحرر قبل PIE

افتح PowerShell من جذر `game/client-unreal/` ونفّذ الأمر التالي. هذا هو أول أمر يجب تشغيله بعد سحب تغييرات C++ جديدة:

```powershell
.\scripts\Build-Rok2.ps1 -Target Editor -EngineRoot $env:UE_ROOT
```

يُنشئ الأمر سجل بناء مؤرخاً في `Saved\BuildLogs\`. عند أول تشغيل بعد تغيير إعدادات العرض أو إصدار المحرك، أعد البناء النظيف فقط عند الحاجة:

```powershell
.\scripts\Build-Rok2.ps1 -Target Editor -Clean -EngineRoot $env:UE_ROOT
```

> لا تستخدم `-Clean` كإجراء اعتيادي؛ فهو يحذف `Binaries` و`Intermediate` ويطيل زمن إعادة بناء المشروع.

## 2.5 استيراد أصول الواجهة والمدينة والحضارات (لا تغيير وظيفي)

قبل أول بناء بعد هذه الدفعة، حوّل صور شاشة اختيار الحضارات وحزمة المدينة والخريطة والواجهة إلى `Texture2D` داخل مشروع Unreal. تنشئ السكربتات سجلات استيراد ولا تتطلب حساب Marketplace:

```powershell
.\scripts\Import-CivVisuals.ps1 -EngineRoot $env:UE_ROOT
.\scripts\Import-CityMapUIAssets.ps1 -EngineRoot $env:UE_ROOT
```

أو نفّذ استيراد جميع PNG تلقائياً قبل بناء المحرر:

```powershell
.\scripts\Build-Rok2.ps1 -Target Editor -ImportCivVisuals -ImportCityMapUiAssets -EngineRoot $env:UE_ROOT
```

> بعد تعديل ملف PNG لاحقاً، مرر `-ReplaceExisting` إلى سكربت الاستيراد ثم أعد بناء المحرر. يجب أن تظهر المسارات `/Game/Art/CivBackgrounds` و`/Game/Art/CivIcons` و`/Game/Art/Commanders` و`/Game/Art/UIIcons` و`/Game/Art/UIButtons` و`/Game/Art/CityBuildingIcons` في Content Browser. لاستيراد نماذج Kenney GLB، فعّل GLTF Importer أو Interchange في UE 5.4.4 ثم أضف `-ImportMeshes` إلى سكربت المدينة.

## 3. اختبار دخان تشغيلي مستقل (لا تغيير وظيفي)

بعد نجاح بناء المحرر، شغّل العميل في وضع **Standalone** لمدة 90 ثانية. يتحقق السكربت من إنشاء السجل، وتحميل `Rok2Main`، وعدم ظهور بصمة خطأ قاتل، ثم يغلق العملية تلقائياً:

```powershell
.\scripts\Run-Rok2RuntimeSmoke.ps1 -EngineRoot $env:UE_ROOT
```

إذا أردت إبقاء اللعبة مفتوحة للفحص، استخدم `-KeepOpen`. أما إذا فشل الاختبار، فأرفق ملف `Saved\BuildLogs\runtime-smoke-*.log` عند فتح تقرير خطأ؛ لا ترسل أي ملف مفاتيح أو بيانات حساب.

## 3.5 سيناريو PIE القصير (Smoke قبل PIE الكامل)

بعد نجاح البناء، نفّذ الدورة البصرية السريعة (5–10 دقائق) الموثقة في [`PIE_QUICK_SCENARIO.md`](PIE_QUICK_SCENARIO.md): إقلاع بدون قتل، نقرة موحدة، لوحة تفتح وتُغلق، بطاقة الحضارة، حدث موسم حي، قطع وعودة. سجّل نتائج الدورة في جدولها وأرفق اللقطات؛ الدورة تغطي تغطية أولية صفوف PIE-02/03/04/05/08/09 ولا تُغلق المعايير المتبقية (PIE-06/07/10/11) التي تبقى ضمن دورة PIE الكاملة بعميلين.

## 4. اختبار PIE اليدوي — معيار P7-T1

### 4.1 فتح المشروع

افتح `Rok2.uproject` بالمحرك نفسه الذي بُني به الكود. عند ظهور طلب إعادة بناء، وافق عليه فقط بعد نجاح أمر **Build-Rok2.ps1**. إن واجهت شاشة سوداء أو تشويشاً، استخدم `RunEditor_SafeMode.bat` بعد ضبط متغير `UE_PATH` بداخله، ثم راجع `Docs/TROUBLESHOOT_BLACK_SCREEN.md`.

في شريط المحرر، تحقق من أن الخريطة المفتوحة هي **Rok2Main**، ثم اضغط زر **Play**. استخدم وضع `Selected Viewport` أو `New Editor Window (PIE)`؛ المهم أن تسجل النتيجة بوضوح في جدول الأدلة أدناه.

### 4.2 قائمة تحقق P7-T1

نفّذ الحالات بالترتيب. يجب أن تمر جميع الصفوف قبل تعليم P7-T1 كمكتمل في `PLAN.md`.

| المعرّف | الفعل داخل PIE | النتيجة المطلوبة | دليل النجاح |
|---|---|---|---|
| PIE-01 | تشغيل `Rok2Main` وانتظار اكتمال التحميل | تظهر الكاميرا والخريطة وHUD من دون خطأ قاتل أو شاشة سوداء | لقطة شاشة + آخر 30 سطراً من Output Log |
| PIE-02 | اضغط زراً من أزرار HUD أو افتح لوحة | يُسمع `UiButtonClick` عند الضغط و`UiPanelOpen` عند إظهار اللوحة | فيديو قصير أو تسجيل صوت/شاشة |
| PIE-03 | افتح الدردشة ثم أغلق أي لوحة قابلة للإغلاق | يعمل صوت فتح اللوحة وإغلاقها من دون تكرار مزعج | فيديو قصير أو ملاحظة دقيقة في السجل |
| PIE-04 | افتح **حكاية المملكة** من شريط HUD | تُنشأ الشاشة عند الطلب وتعرض خط الزمن من اللقطة، ويصدر صوت فتح اللوحة | لقطة الشاشة + Output Log |
| PIE-05 | أدخل/حدّث حدث موسم من جلسة خادم ثانية أو من بيئة الاختبار | يصل `season_story_event` الحي ويضاف مرة واحدة إلى الخط الزمني إذا كانت الشاشة موجودة | لقطة قبل/بعد أو فيديو قصير |
| PIE-06 | تحرك في خريطة العالم وانظر إلى الموارد والممرات والعرش | تظهر ألوان ومقاييس ووسوم `URok2WorldIconography` للعقد/الممرات/العرش بدلاً من النمط الثابت | ثلاث لقطات: مورد، ممر، عرش |
| PIE-07 | شغّل جلسة بحضارة ذات همس معرف | يُشغّل همس الحضارة مرة واحدة عند التهيئة من دون رفع مستوى الصوت العام | فيديو قصير أو توثيق سمعي من المختبر |
| PIE-08 | افصل الاتصال أو استخدم بيئة اختبار غير متاحة ثم أعد الاتصال | يصدر صوت الخطأ الموحد وتبقى اللعبة قابلة للاسترداد | Output Log + وصف الخطوة |
| PIE-09 | سجّل دخول ضيفاً بلا مدينة | تظهر بطاقة الحضارة مع الخلفية والشعار والقائد بدلاً من `ComboBox` نصي | لقطة شاشة 16:9 |
| PIE-10 | انتقل عبر الأسهم الستة في بطاقة الحضارة | يتغير الاسم والخلفية والشعار والقائد والنبذة والعداد مع بقاء اختيار API متزامناً | فيديو قصير أو ست لقطات |
| PIE-11 | اضغط `Start Journey` بعد اختيار حضارة غير روما | يستقبل الخادم معرّف الحضارة الظاهر في البطاقة وتُنشأ المدينة الصحيحة | Output Log + لقطة HUD |

يُفضّل تسمية الأدلة بهذه الصيغة: `P7-T1-PIE-01-YYYYMMDD.png` أو `P7-T1-PIE-05-YYYYMMDD.mp4`. لا تضع لقطات البناء أو الفيديوهات الكبيرة في Git؛ ارفعها إلى مساحة مشاركة المشروع أو أرفقها في تقرير المراجعة.

## 4.5 سجل أعطال البناء (Blocker Log)

كل عطل يمنع البناء أو التحزيم أو الدخان أو PIE يُوثق بمعرف `BF-00N` في [`BUILD_FAILURES.md`](BUILD_FAILURES.md) مع سببه الجذري وإصلاحه ورقم الـ commit، وفق القواعد الست في السجل (أوقف، عزل، سجّل، أصلح من جذره، امنع الانحدار، لا تحذف الصفوف). السجل الحالي يوثّق BF-001 إلى BF-006: فشل Java/Gradle على Android، صلاحيات التخزين، تعليق Splash، GoogleVR PermissionHelper stub، اختلاف نسخة المحرك، والشاشة السوداء.

## 5. تحزيم نسخة Windows

لإنشاء نسخة قابلة للتوزيع بعد اجتياز PIE، نفّذ بناء Development أولاً:

```powershell
.\scripts\Build-Rok2.ps1 -Target Development -Package -OutputDirectory 'D:\ROK2-Builds\Development' -EngineRoot $env:UE_ROOT
```

ولاستخدام تهيئة إصدار نهائي بدلاً من ذلك:

```powershell
.\scripts\Build-Rok2.ps1 -Target Shipping -Package -OutputDirectory 'D:\ROK2-Builds\Shipping' -EngineRoot $env:UE_ROOT
```

يفصل السكربت سجلات البناء والتحزيم داخل `Saved\BuildLogs\`. لا تضف مجلد `Artifacts` أو ملفات الحزم الناتجة إلى المستودع.

## 6. استكشاف الأخطاء السريع

| العرض | الإجراء الأول |
|---|---|
| أي عطل بناء أو PIE جديد | نفّذ `Run-Rok2Preflight.ps1` أولًا، ثم سجّل العطل في `BUILD_FAILURES.md` بمعرف جديد قبل أي إصلاح. |
| لا يُعثر على `Build.bat` أو `UnrealEditor.exe` | الضبط `UE_ROOT` أو مرر `-EngineRoot` إلى مجلد UE 5.4.4 الصحيح؛ فشل preflight يكشفه تلقائيًا. |
| رسالة اختلاف نسخة المحرك | تحقق أن `Engine\Build\Build.version` يسجل `5.4.4`؛ السكربتات ترفض أي إصدار آخر لحماية قابلية التكرار. |
| خطأ C++/Visual Studio عند البناء | افتح سجل `Saved\BuildLogs\build-*.log`، أصلح أول خطأ C++ ظاهر فقط، ثم أعد البناء. |
| شاشة سوداء/تشويش | شغّل `RunEditor_SafeMode.bat` واقرأ `TROUBLESHOOT_BLACK_SCREEN.md`. |
| يفشل دخان التشغيل لغياب `Rok2Main` | تحقق من إعداد `GameMapsSettings` ومن سجل `runtime-smoke-*.log`. |
| لا يصل حدث الموسم الحي | تحقق من اتصال WebSocket في Output Log، ثم اختبر الخادم بجلسة ثانية. |
| لا تسمع الصوت | تحقق من مخرج الصوت في Windows ومستوى Master Volume، ثم أعد PIE ولا تعتمد على سجل البناء لإثبات السماع. |
| تظهر البطاقة بلون احتياطي بلا صور | شغّل `Import-CivVisuals.ps1` ثم أعد فتح المحرر؛ تحقق من أن Texture2D موجودة في مسارات `/Game/Art/` المذكورة أعلاه. |

## 7. قرار الإغلاق

بعد تمرير بوابة `npm run check`، وفحص الجاهزية `Run-Rok2Preflight.ps1`، وبناء المحرر، ودخان التشغيل، **وجميع صفوف PIE-01 إلى PIE-11** مع توثيق أي عطل في `BUILD_FAILURES.md`، أضف أدلة الاختبار إلى تقرير المراجعة ثم علّم **P7-T1** `[x]`. إذا فشل صف واحد، اترك P7-T1 مفتوحاً وسجّل المعرّف وملف السجل والخطوة اللازمة للإصلاح؛ لا توصف الدفعة بأنها مكتملة بناءً على اختبار بنيوي وحده.

## المراجع

[1]: https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?lang=en-US "Epic Games — Setting Up Visual Studio"
