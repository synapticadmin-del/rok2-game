# قبول PIE بعميلين وتجهيز Android Development

> **الغرض:** يحدد هذا الدليل الدليل المطلوب لإغلاق **P7-T3** وقبول **P7-T6**. لا تُعلَّم البنود مكتملة بناءً على حارس بنيوي أو تشغيل Worker/D1 محلي فقط؛ يلزم بناء وتشغيل فعليان على Windows مع UE 5.4.4، وقياس فعلي على جهاز Android متصل.

| المسار | شرط القبول | حالة بيئة التنفيذ الحالية |
|---|---|---|
| P7-T3 | عميلان مستقلان داخل PIE أو Standalone، بحسابين مختلفين، يكملان التحالف والرالي والممر ثم تستعيد الواجهة التقرير بعد عودة الاتصال | غير متاح هنا: لا يوجد Windows أو UE 5.4.4 أو بيئة اختبار منشورة/مصرح بها |
| P7-T6 | حزمة Android Development مُثبّتة على جهاز حقيقي مع تسجيل CPU/GPU/RHI والذاكرة من مشهد الخريطة القياسي | غير متاح هنا: لا توجد Android SDK/NDK/JDK أو `adb` أو جهاز متصل |

## 1. متطلبات Windows وAndroid

يتطلب هذا المشروع **Unreal Engine 5.4.4**. بالنسبة إلى Android، تعتمد UE 5.4.4 Android Studio Flamingo 2022.2.1 Patch 2، وSDK 34، وNDK r25b، وBuild-tools 34.0.0، وOpenJDK 17.0.6. ويجب أن يكون الجهاز Android 8 أو أحدث، بمعالج Arm 64-bit ورسوميات متوافقة. [1]

ابدأ في Windows PowerShell بعد تثبيت UE 5.4.4 من Epic Games Launcher. يفحص السكربت الإعدادات أولاً؛ لا يبدأ تنزيل SDK إلا مع `-InstallSdk` صراحةً:

```powershell
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.4'
$env:ROK2_ROOT = 'C:\src\rok2-game'
Set-Location $env:ROK2_ROOT\game\client-unreal
.\scripts\Prepare-AndroidDevelopment.ps1 -EngineRoot $env:UE_ROOT -InstallSdk
# بعد إكمال Android Studio وتسجيل الخروج/الدخول:
.\scripts\Prepare-AndroidDevelopment.ps1 -EngineRoot $env:UE_ROOT
```

يعرض Turnkey تثبيت Android Studio. اترك مسار Android Studio الافتراضي، ثم افتح **More Actions > SDK Manager > SDK Tools** وفعل **Android SDK Command-Line Tools (latest)**، وأغلق Android Studio كي يكمل Turnkey تثبيت المكونات. بعد النجاح، سجل الخروج من Windows ثم سجل الدخول مجدداً، وافتح المحرر وتأكد من أن **Platforms > Android** لا يعرض Install أو Repair. [2]

لا تحفظ متغيرات SDK المحلية أو سجلات الأجهزة أو رموز الوصول في Git.

## 2. قبول P7-T3 بعميلين مستقلين

يجب أن يشير العميلان إلى نفس بيئة الاختبار المصرح بها وأن يستخدما حسابين مختلفين. لا تستخدم بيئة إنتاج ولا حسابات حقيقية. قبل البدء، حدّث المستودع، شغّل بوابة العقود، وابنِ المحرر:

```powershell
Set-Location $env:ROK2_ROOT\game\backend
npm run check

Set-Location $env:ROK2_ROOT\game\client-unreal
.\scripts\Build-Rok2.ps1 -Target Editor -EngineRoot $env:UE_ROOT
```

افتح `Rok2.uproject` مرتين بالمحرك نفسه. شغّل كل نسخة في **New Editor Window (PIE)** أو شغّل إحداهما في PIE والثانية كنسخة Development مستقلة. لا يُقبل وضع نافذتين لحساب اللاعب نفسه.

| المعرّف | الإجراء | معيار النجاح والدليل المطلوب |
|---|---|---|
| T3-PIE-01 | تسجيل دخول/تأسيس لاعب A وB بحسابين مختلفين | مدينتان ومعرّفان مختلفان في اللقطة والسجل |
| T3-PIE-02 | ينضم B إلى تحالف A | تعرض الواجهتان عضوية التحالف نفسها بعد تحديث سلطوي |
| T3-PIE-03 | ينشئ A رالي للممر وينضم B ثم يُطلقه | يظهر الرالي والجيشان قبل الإطلاق، ثم تتحرك المسيرات نحو الهدف |
| T3-PIE-04 | انتظار تسوية القتال والعودة | يحتل الهدف أو يسجل قتالاً حاسماً، وتعود القوات بلا خطأ قاعدة بيانات |
| T3-PIE-05 | فتح التقارير لدى A وB | يرى المشاركان تقرير الرالي الخاص المرتبط بـ `rallyId` نفسه ولا يراه لاعب غير مشارك |
| T3-PIE-06 | قطع الشبكة عن B ثم إعادتها | بعد الاتصال يعيد العميل المدينة والعالم والقادة والتقارير والراليات؛ لا تتضاعف الطلبات ولا تبقى الواجهة في حالة انتظار |

احفظ لقطة/فيديو قصيراً وسجل Output Log لكل صف في مساحة أدلة خارج Git بصيغة `P7-T3-T3-PIE-XX-YYYYMMDD.*`. بعد نجاح الصفوف الستة فقط، يحق تحديث P7-T3 إلى `[x]`.

## 3. تحضير وقياس P7-T6 على Android

بعد نجاح Turnkey، افتح المشروع في UE 5.4.4 وتحقق من إعدادات SDK في **Platforms > Android**. أنشئ حزمة **Development** فقط، لا Shipping، وثبّتها على الجهاز المتصل. استخدم سكربت البناء الذي يدعم Android بعد هذه الدفعة من الجذر `game/client-unreal`:

```powershell
.\scripts\Build-Rok2.ps1 -Target Development -Platform Android -Package -OutputDirectory 'D:\ROK2-Builds\Android-Development' -EngineRoot $env:UE_ROOT
```

شغّل نفس مشهد الخريطة المحدد في `game/docs/P7_T6_WORLD_PERFORMANCE_BUDGET.md`، وانتظر استقرار التحميل قبل القياس. سجّل طراز الجهاز وإصدار Android والدقة ومزود الرسوميات، ثم اجمع لقطات `stat unit` و`stat RHI` و`stat memory` ولقطة `URok2WorldRenderer::GetPerformanceSnapshot()`. افصل قياس CPU الخاص بالراسم عن قياسات GPU والذاكرة الفعلية ولا تستنتج أحدها من الآخر.

| الدليل | المطلوب |
|---|---|
| صلاحية الحزمة | سجل UAT وانتهاء التثبيت والتشغيل بنجاح |
| تعريف الجهاز | الطراز، Android API، ABI، GPU، الدقة ومعدل التحديث |
| زمن الإطار | لقطة `stat unit` من مشهد القياس بعد الاستقرار |
| GPU/RHI والذاكرة | لقطتا `stat RHI` و`stat memory` من المشهد نفسه |
| عدادات راسم العالم | لقطة للقيم التي يعيدها `GetPerformanceSnapshot()` |
| قرار القبول | مقارنة القياسات بميزانية P7-T6 وذكر Pass/Fail والتراجع التالي عند الفشل |

دوّن لكل جلسة قياس: `P7-T6-ANDROID-YYYYMMDD.md` خارج Git، واربط فيه ملفات الأدلة الستة أعلاه وتكوين البناء ونسخة المحرك. لا يفي نجاح `Prepare-AndroidDevelopment.ps1` أو بناء APK وحده بشرط القياس.

ارفع ملفات الأدلة إلى مساحة المراجعة الخارجية ولا تضف APK/AAB أو السجلات الكبيرة أو ملفات تعريف الجهاز إلى Git. لا يمكن إغلاق P7-T6 قبل قياس جهاز حقيقي واحد على الأقل؛ ويُفضل تكراره على جهازين من فئتين مختلفتين.

## 4. المانع الحالي

فحص بيئة هذه الجلسة في 12 أغسطس 2026 أثبت أنها Linux معزولة، لا تحتوي `UnrealEditor` أو `adb` أو `sdkmanager` أو جسر Windows/سطح مكتب أو جهاز Android متصل. لذلك أُنجزت ملفات الإعداد والتحقق فقط هنا. لتنفيذ الصفوف أعلاه فعلياً، يجب توفير جهاز Windows متصل عليه UE 5.4.4 وVisual Studio 2022، وجهاز Android مفعّل USB debugging، وبيئة اختبار مصرح بها.

## المراجع

[1]: https://dev.epicgames.com/documentation/unreal-engine/android-development-requirements-for-unreal-engine?application_version=5.4 "Epic Games — Android Development Requirements for UE 5.4"
[2]: https://dev.epicgames.com/documentation/unreal-engine/set-up-android-sdk-ndk-and-android-studio-using-turnkey-for-unreal-engine?application_version=5.4 "Epic Games — Setting Up Android SDK and NDK for UE 5.4"
