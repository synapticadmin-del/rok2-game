# P16-T7 — كوك أندرويد يفشل: مواد glTF مربوطة بمحتوى ملحق للمحرر فقط

> **الطلب:** «ابني تطبيق اللعبة وأعطني مسار الـ APK».
> ما حدث أن البناء فشل بعد 56 دقيقة كوك، ثم فشل مرة ثانية في Gradle.
> هذه الوثيقة تسجّل العطلين وسببهما الجذري وما تغيّر.

الحارس الآلي: `npm run test:p16-t7-android-cook-refs` (15 فحصاً) داخل
`game/backend`، مضاف إلى سلسلة `npm run check`.

---

## 1. العطل الأول — الكوك يفشل بـ ExitCode=25

### الأثر

```
LogCook: Error: Content is missing from cook. Source package referenced an object
in target package but the target package was marked NeverCook or is not cookable
for the target platform.
	Source package: /Game/Art/kaykit/hexagons_medieval1
	Target package: /Interchange/gltf/MaterialInstances/MI_Default_Opaque
	Referenced object: /Interchange/gltf/MaterialInstances/MI_Default_Opaque
Took 3,236.31s to run UnrealEditor-Cmd.exe, ExitCode=1
Cook failed.
AutomationTool exiting with ExitCode=25 (Error_UnknownCookFailure)
```

### السبب الجذري

`hexagons_medieval1` هو `MaterialInstanceConstant` أنتجه مستورد glTF لأطلس نسيج
KayKit، وهو المادة التي تستخدمها **كل** مجسمات KayKit (18 مبنى وعلماً وجبلاً).
والمستورد يربطه بوالد داخل ملحق Interchange نفسه:

```
parent: /Interchange/gltf/MaterialInstances/MI_Default_Opaque.MI_Default_Opaque
```

و`Interchange.uplugin` في UE 5.4.4 معلن:

```json
"SupportedTargetPlatforms": [ "Win64", "Linux", "Mac" ]
```

أي أن محتواه غير قابل للكوك على أندرويد. الوالد لا يُحزَّم، فالمادة بلا والد،
فالكوك يتوقف. هذا يفسّر لماذا كان البناء يمرّ على Win64 ويفشل على Android فقط.

ملاحظة سياقية: تفعيل Interchange للمحرر وحده (`TargetAllowList: ["Editor"]`) في
P16-T5 كان صحيحاً ولازماً — بدونه يسقط المحرك عند استيراد `.glb`. لكنه لا يحل
هذه المشكلة لأنها في **محتوى محزوم يشير إلى محتوى الملحق**، لا في تحميل وحدة.

### ما تغيّر

مادة مشروع `/Game/Art/Materials/M_Rok2Gltf` تكرّر بارامترات مستورد glTF **بنفس
الأسماء**:

| بارامتر | نوعه | لماذا بهذا الاسم بالذات |
|---|---|---|
| `BaseColorTexture` | `TextureSampleParameter2D` | المستورد يضبط النسيج بهذا الاسم؛ أي اسم آخر يفقد أطلس KayKit |
| `BaseColorFactor` | `VectorParameter` | `URok2ProceduralAssets::TintExistingMaterialOn` تصبغ الحضارة عبره |
| `MetallicFactor` | `ScalarParameter` | المستورد يضبطها `0.0` |
| `RoughnessFactor` | `ScalarParameter` | المستورد يضبطها `0.5` |

`BaseColor = BaseColorTexture.RGB × BaseColorFactor`، وهو نفس ما تفعله
`M_Default` داخل الملحق. المادة مُعلَّمة `used_with_instanced_static_meshes`
لأن مجسمات الخريطة والمدينة كلها HISM، وبدون العلم يستبدلها المحرك
بـ`DefaultMaterial` في بناء مُطبَّق.

ثم `scripts/reparent_gltf_materials.py` يعيد ربط كل `MaterialInstanceConstant`
تحت `/Game` والدها خارج `/Game` و`/Engine` إلى هذه المادة، ويتحقق بعد التعديل
من خلو المشروع من أي مرجع غير قابل للكوك (مواد وفتحات مجسمات على حد سواء).
لأن مطابقة البارامترات بالاسم، تبقى النسج والقيم كما استوردها glTF:

```
[ROK2] MI /Game/Art/kaykit/hexagons_medieval1.hexagons_medieval1
[ROK2]   parent: /Game/Art/Materials/M_Rok2Gltf.M_Rok2Gltf
[ROK2]   texture BaseColorTexture = /Game/Art/kaykit/hexagons_medieval.hexagons_medieval
[ROK2]   scalar MetallicFactor = 0.0
[ROK2]   scalar RoughnessFactor = 0.5
```

`ImportAssets.bat` يشغّل السكربت **بعد** الاستيراد. هذا ليس تفصيلاً: أي إعادة
استيراد لأصل glTF تعيد ربط المادة بالملحق، فيعود العطل. الحارس يتحقق من الترتيب.

---

## 2. العطل الثاني — Gradle: `Cannot invoke method aaptOptions() on null object`

بعد نجاح الكوك، فشل `assembleDebug` على السطر الذي يحقنه `Build-Rok2.ps1` في
`Intermediate/Android/arm64/gradle/app/buildAdditions.gradle`:

```groovy
android { androidResources { noCompress ... } aaptOptions { noCompress ... } }
```

Groovy يقرأ `androidResources { … } aaptOptions { … }` على سطر واحد كسلسلة
نداءات — أي `androidResources(closure).aaptOptions(closure)` — و`androidResources`
تعيد `void`، فالنداء التالي على `null`. كل بلوك صار على سطره، و`aaptOptions`
حُذفت لأنها مهجورة في AGP 7.4.2 و`androidResources` تكفي.

عطل تابع: `Set-Content -Encoding UTF8` في PowerShell 5.1 يكتب BOM، فيفشل Groovy
بـ `Unexpected character: '?' @ line 1, column 1`. الكتابة الآن عبر
`System.IO.File]::WriteAllText` مع `UTF8Encoding($false)`.

---

## 3. حالة الـ APK الناتج

| الحقل | القيمة |
|---|---|
| المسار | `game/client-unreal/Artifacts/Android-Development/Rok2-arm64.apk` |
| الحجم | 538,086,729 بايت (~513 MiB) |
| الحزمة | `com.rok2.thrones` — versionName 0.1.0، versionCode 1 |
| minSdk / targetSdk | 26 / 34 |
| ABI | arm64-v8a (`libUnreal.so` 258 MB) |
| البيانات | `assets/main.obb.png` — 229,723,449 بايت، **stored** (غير مضغوط، قابل للـ mmap) |
| الأصول المطبوخة | `M_Rok2Base` + `M_Rok2Unlit` + `M_Rok2Gltf` + 18 مجسم KayKit، `Rok2-Android_ASTC.pak` 86 MB |
| التوقيع | debug keystore |

## 4. ما لم يُتحقق منه

لا جهاز أندرويد ولا محاكٍ متصل في هذه البيئة (`adb devices` فارغة). أي أن
**تثبيت الـ APK وتشغيله لم يُجرَّبا**، ويبقى قياس `stat unit` و`stat memory`
و`stat RHI` على جهاز مستهدف شرطاً قائماً كما في
`Docs/PIE_TWO_CLIENTS_ANDROID_ACCEPTANCE.md`. ما يثبته هذا العمل هو أن الكوك
والتحزيم يكتملان وأن الأصول التي كانت تسقط صارت داخل الحزمة.
