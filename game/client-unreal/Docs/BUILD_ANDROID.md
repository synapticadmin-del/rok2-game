# بناء APK لأندرويد — بدون الاعتماد على نافذة المحرر

> **لماذا هذا المسار؟** كرت الشاشة على جهاز التطوير (Intel HD 530) لا يدعم
> Shader Model 6، فتظهر نافذة المحرر كتشويش أبيض/أسود. لكن **التغليف
> (packaging) لا يحتاج عرضاً رسومياً إطلاقاً** — هو كتابة أصول وبناء
> shaders على المعالج. لذلك نبني عبر سطر الأوامر، والهاتف (Adreno/Mali)
> سيرندر بشكل سليم.

---

## 1) المتطلبات (مرة واحدة)

| المتطلب | التفاصيل |
|---------|----------|
| Unreal Engine | 5.4 أو أحدث (مطابق لـ `EngineAssociation` في `Rok2.uproject`) |
| Visual Studio 2022 | مع حِمل *Game development with C++* + *.NET desktop* |
| JDK | 17 |
| Android Studio | لتثبيت SDK Platform 34 و NDK |
| إعداد المسارات | شغّل `Engine\Extras\Android\SetupAndroid.bat` من مجلد المحرك — يضبط `ANDROID_HOME` و `NDKROOT` و `JAVA_HOME` تلقائياً |

بعد `SetupAndroid.bat` **أعد تشغيل الجهاز** (أو على الأقل نافذة الأوامر)
حتى تُقرأ متغيرات البيئة الجديدة.

> ملاحظة: نسخة NDK المطلوبة تتبع نسخة المحرك — لا تثبّت واحدة يدوياً؛
> اترك `SetupAndroid.bat` يختارها.

---

## 2) استيراد الأصول (مرة واحدة، وكلما تغيّرت الأصول)

```
game\client-unreal\ImportAssets.bat
```

يفكّ ترميز الأصول المخزّنة base64 ثم يستوردها إلى `.uasset` عبر
`UnrealEditor-Cmd -run=pythonscript -nullrhi` — بلا نافذة محرر وبلا كرت شاشة.

---

## 3) البناء

```
game\client-unreal\BuildAndroid.bat
```

عدّل `UE_ROOT` بداخله أولاً إن كان المحرك في مسار مختلف. السكربت ينفّذ:

```
RunUAT BuildCookRun -platform=Android -cookflavor=ASTC
                    -build -cook -stage -package -pak -archive -nullrhi
```

- `-nullrhi` يمنع تهيئة كرت الشاشة أثناء الـ cooking — مهم على جهاز بكرت معطوب.
- `-cookflavor=ASTC` هو التنسيق المناسب لمعالجات Adreno / Mali الحديثة.
- أول بناء يستغرق **30–90 دقيقة** (بناء shaders من الصفر). البناء التالي أسرع كثيراً.

المخرجات في: `game\client-unreal\Build\Android\`

---

## 4) التثبيت على الهاتف

فعّل *خيارات المطوّر* → *تصحيح أخطاء USB*، ثم:

```
adb install -r "Build\Android\Rok2-arm64.apk"
```

أو انسخ الـ APK للهاتف وثبّته يدوياً (اسمح بالتثبيت من مصادر غير معروفة).

لمتابعة سجل اللعبة أثناء التشغيل:

```
adb logcat -s UE:V LogRok2:V
```

---

## 5) الإعدادات الحالية

| الإعداد | القيمة | ملاحظة |
|---------|--------|--------|
| Package Name | `com.rok2.thrones` | |
| Min SDK | 26 | UE5 لا يدعم أقل من ذلك |
| Target SDK | 34 | |
| الاتجاه | Landscape | |
| Vulkan | مفعّل | |
| OpenGL ES3.1 | **معطّل** | إن لم تعمل اللعبة على هاتفك، فعّل `bBuildForES31=True` في `Config/Android/AndroidEngine.ini` كخطة بديلة |
| بيانات داخل APK | نعم | ملف واحد، بلا OBB |

---

## 6) ما الذي ستراه في هذا البناء؟

توقّعات واقعية — هذا **prototype** وليس بناءً نهائياً:

- شاشة إقلاع ← تسجيل دخول ضيف تلقائي على الخادم الحي
  `https://rok2-api.lolelarap.workers.dev` (تم التحقق أنه يعمل ويردّ برمز دخول).
- ثم اختيار الحضارة ← تأسيس المدينة ← واجهة HUD وكاميرا إيزومترية.
- المجسّمات والأصوات وصور القادة **حقيقية** — بشرط تشغيل `ImportAssets.bat`
  أولاً. إن تخطّيته سيعمل البناء لكن بأشكال هندسية بدائية (fallback مقصود
  في الكود).

---

## 6 ب) صلاحيات التخزين — لماذا لا تطلب اللعبة صلاحية التخزين عند الفتح؟

**المشكلة:** UE5 يضيف `WRITE_EXTERNAL_STORAGE` و `READ_EXTERNAL_STORAGE`
إلى الـ manifest المُولَّد تلقائيًا (للتحقق من ملفات الـ OBB/الملفات الخارجية عند
أول تشغيل)، فيظهر على الهاتف تنبيه *"Permission Required: Storage"* عند فتح
اللعبة رغم أن الإعدادات لا تطلب هذه الصلاحيات صراحة (ticket UE-170079).

**الحل المطبق في هذا المشروع (3 طبقات):**

1. **`Build/Android/ManifestRequirementsOverride.txt`** — يستبدل قسم
   `<uses-permission>` في الـ manifest النهائي بالكامل؛ القائمة أعلاه لا تحتوي
   أي صلاحية تخزين، فقط `INTERNET` + `ACCESS_NETWORK_STATE` +
   `ACCESS_WIFI_STATE`. هذا هو التخليص القاطع: حتى لو حاول المحرك أو أي plugin
   إضافة storage permissions لاحقًا فلن تدخل الـ manifest النهائي.
2. **`Build/Android/ManifestApplicationAdditions.txt`** — يضيف
   `android:requestLegacyExternalStorage="false"` لضمان عدم وراثة الوضع
   القديم من إصدارات Android ≤ 9.
3. **`bUseExternalFilesDir=True`** في
   `Config/DefaultEngine.ini` (`[AndroidRuntimeSettings]`) — الحل الرسمي من
   Epic: ملفات اللعبة المحفوظة (مثل `DeviceId` في
   `FPaths::ProjectSavedDir()` بـ `Rok2Api.cpp`) تذهب إلى
   `ExternalFilesDir` الذي **لا يطلب أي runtime permission على API 23+**، بينما
   تظل بيانات اللعبة نفسها داخل الـ APK (`bPackageDataInsideApk=True`, بلا
   OBB) — لا تأثير على حجم التثبيت.

التحقق بعد البناء: استخرج الـ manifest من الـ APK (`apktool d`) وتأكد من
غياب `WRITE_EXTERNAL_STORAGE` و `READ_EXTERNAL_STORAGE`؛ أو ببساطة افتح
اللعبة على Android 11+ ولا يجب أن تظهر أي شاشة "Permission Required".

### 6 ج) اللعبة عالقة على شعار Unreal Engine ولا تفتح (P7-T12)

**المشكلة:** عند فتح APK على الهاتف تظهر شاشة شعار Unreal Engine وتبقى
معلقة بلا نهاية — لا تتخطاها ولا تدخل اللعبة. الشعار يعرضه نشاط المحرك
الأصلي (UAndroidThunkJava) **قبل** تحميل المستوى، وأي فشل في إعداد العرض
(RHI) أو واجهة OBB عند بدء التشغيل يعلق الشاشة صامتًا.

**الحل المطبق في هذا المشروع (4 إعدادات في
`Config/DefaultEngine.ini` قسم `[AndroidRuntimeSettings]`):**

1. **`rhi.AndroidDefaultGraphicsRHI=DefaultGraphicsRHI_OpenGLES3` +
   `bSupportsOpenGL3=True`** — OpenGLES3 أساسي لأن أدريفرات Vulkan على أجهزة
   كثيرة (خاصة GPU قديمة أو Mid-range) تجريبية وتعلق التطبيق على أول إطار؛
   GLES3 مستقر عالميًا على أي GPU. Vulkan يظل متاحًا إن دعمه الجهاز فعليًا
   (`bSupportsVulkan=True`).
2. **`bSplashScreen=False`** — شاشة البداية هي نفسها نقطة التعليق؛ تعطيلها
   يجعل التطبيق يفتح مباشرة على اللودج.
3. **`bDisableOBBPakUI=True`** — مع TargetSDK 34 يظهر المحرك واجهة OBB
   الافتراضية عند بدء التشغيل وتعلق إن فُقدت ملفات الـ APK؛ ولا OBB لدينا
   أصلًا لأن `bPackageDataInsideApk=True`.
4. **`bForceVulkan=False`** — لا نُجبر Vulkan على أي جهاز.

**سببان إضافيان عولجا في نفس الدفعة:**

- **`PythonScriptPlugin` و `EditorScriptingUtilities` عطلّيا من
  `Rok2.uproject`** (`DisableEnginePluginsByDefault` + `DisabledPlugins`) —
  كلاهما plugin محرر/ويندوز لا يعمل على Android، وحملهما في APK سببٌ معروف
  لتعليق/فشل صامت عند أول تشغيل. سكربتات الاستيراد تستخدم
  `ImportAssetsCommandlet` ولا تتأثر.

**المصادر:**

- [Black Screen / Crash — UE 5.4.3 + SDK 34](https://forums.unrealengine.com/t/black-screen-crash-building-android-14-on-5-4-3-sdk-34-target/1946498)
  (الحل الموثق: OpenGLES بدل Vulkan + تعطيل Splash + `Disable OBB at start up`)
- [UE5 Android crash in splash screen](https://forums.unrealengine.com/t/ue5-android-crash-in-splash-screen/526634)
  (الحل الموثق: التحويل إلى OpenGLES فقط)

**التحقق بعد البناء:** افتح APK على الهاتف — يجب أن يتخطى الشعار فورًا
ويدخل اللودج، ولا تظهر أي شاشة معلقة. إذا علقت بعد ذلك فسببٌ آخر في بدء
التشغيل: افحص `adb logcat -s UE4` للحصول على أول Fatal error.

---

## 7) إن فشل البناء

| رسالة الخطأ | الحل |
|-------------|------|
| `SDK not found` / `ANDROID_HOME` | أعد تشغيل `SetupAndroid.bat` ثم أعد تشغيل الجهاز |
| `Missing UE_5.4 ... UnrealBuildTool` | تأكد أن `UE_ROOT` في `BuildAndroid.bat` صحيح |
| خطأ compile في وحدة `Rok2` | خطأ برمجي فعلي — السجل يحدد الملف والسطر |
| تعارض `minSdkVersion` | تحقق من تطابق القيمة في `DefaultEngine.ini` و `Android/AndroidEngine.ini` |

السجلات الكاملة في:
`%USERPROFILE%\AppData\Roaming\Unreal Engine\AutomationTool\Logs`
