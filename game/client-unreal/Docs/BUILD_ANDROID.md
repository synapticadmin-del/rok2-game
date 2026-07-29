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
| Unreal Engine | 5.8 (مطابق لـ `EngineAssociation` في `Rok2.uproject`) |
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

## 7) إن فشل البناء

| رسالة الخطأ | الحل |
|-------------|------|
| `SDK not found` / `ANDROID_HOME` | أعد تشغيل `SetupAndroid.bat` ثم أعد تشغيل الجهاز |
| `Missing UE_5.8 ... UnrealBuildTool` | تأكد أن `UE_ROOT` في `BuildAndroid.bat` صحيح |
| خطأ compile في وحدة `Rok2` | خطأ برمجي فعلي — السجل يحدد الملف والسطر |
| تعارض `minSdkVersion` | تحقق من تطابق القيمة في `DefaultEngine.ini` و `Android/AndroidEngine.ini` |

السجلات الكاملة في:
`%USERPROFILE%\AppData\Roaming\Unreal Engine\AutomationTool\Logs`
