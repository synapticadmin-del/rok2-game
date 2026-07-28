# دليل تطوير وبناء لعبة ROK2 على محرك Unreal Engine 5 (UE5 Master Guide)

**المسار المباشر لمشروع المحرك:**  
`C:\Users\kayf\Desktop\rok2\game\client-unreal`

---

## 1. الرؤية الهندسية ومحرك اللعبة (Engine Vision)

بناءً على التوجيه التقني المباشر، تم اعتماد **Unreal Engine 5.8** كعميل وحيد ورئيسي للعبة **ROK2**. يعتمد العميل على لغة **C++** من أجل تحقيق أعلى أداء ورسوميات 3D فاخرة تنافس ألعاب الفئة الأولى (4X Strategy MMO).

### المميزات التقنية المدمجة في C++ (Unreal Engine 5.8):
1. **نظام الاتصال الشبكي (Rok2Api C++ Module):** متصل ومباشر مع سيرفر Cloudflare Worker API والسيرفر الحي (WebSockets) لقراءة الخريطة 2400² وإرسال مسيرات الهجوم.
2. **الكاميرا الاستراتيجية المنظورية (Rok2IsometricCamera):** كاميرا استراتيجية بزاوية (-50 درجة) تدعم السحب التكتيكي والتكبير/التصغير (Pan & Zoom) بسلاسة.
3. **متحكم اللاعب (Rok2PlayerController):** دعم المدخلات التفاعلية للماوس على الكومبيوتر واللمس على هواتف الأندرويد.
4. **رسم عالم اللعبة 3D (Rok2WorldRenderer):** توليد وتحريك مجسمات المدن والممرات والموارد والجيوش على الخريطة.
5. **منشئ المدينة (Rok2CityBuilder):** بناء وتثبيت المباني التفاعلية في رقعة المدينة الخاصة باللاعب.
6. **واجهات UMG الاحترافية (Rok2BootWidget & Rok2CityWidget):** واجهات تسجيل الدخول، اختيار الحضارات الـ 6، ترقية المباني، تدريب القوات، وعرض شارات الموارد والتحالفات.

---

## 2. هيكلية ملفات C++ في Unreal Engine

```
game/client-unreal/Source/Rok2/
├── Rok2.Build.cs              # إدارة التبعيات (HTTP, WebSockets, Json, JsonUtilities, UMG)
├── Rok2.cpp                   # الملف الرئيسي للموديول
├── Public/
│   ├── Rok2Types.h            # الهياكل البرمجية (DTOs: FPlayerState, FCityState, FPassState...)
│   ├── Rok2Api.h              # كلاس الاتصال بالـ REST API والـ WebSockets
│   ├── Rok2GameMode.h         # GameMode الرئيسي لإدارة الدورة البرمجية
│   ├── Rok2PlayerController.h # متحكم المدخلات واللمس
│   ├── Rok2IsometricCamera.h  # كاميرا الاستراتيجية المنظورية
│   ├── Rok2CityBuilder.h      # كلاس بناء رقعة المدينة والمباني 3D
│   ├── Rok2WorldRenderer.h    # كلاس رسم عناصر خريطة العالم 2400²
│   ├── Rok2ProceduralAssets.h # توليد الخامات والألوان ديناميكياً
│   ├── Rok2BootWidget.h       # واجهة البداية وااختيار الحضارات
│   ├── Rok2CityWidget.h       # واجهة إدارة المدينة والجيوش
│   └── Rok2BlueprintLibrary.h # مكتبة دالات Blueprints للـ Nodes والإحداثيات والحضارات
└── Private/                   # التنفيذ البرمجي لجميع الملفات أعلاه
```

---

## 3. كيفية تشغيل المشروع داخل Unreal Engine Editor

1. افتح مجلد المشروع:
   `C:\Users\kayf\Desktop\rok2\game\client-unreal`
2. اضغط مرتين على ملف **`Rok2.uproject`**.
3. إذا طلب المحرك اختيار النسخة، اختر **Unreal Engine 5.8**.
4. سيقوم المحرك بتوليد ملفات الـ Visual Studio وتجميع كود الـ C++ تلقائياً.
5. داخل محرر Unreal Editor، اضغط على زر **Play (PIE)** لتجربة اللعبة مباشرة داخل المحرر.

---

## 4. خطة بناء اللعبة لأجهزة الكومبيوتر (Windows PC)

1. داخل Unreal Editor، اذهب إلى قائمة:  
   `Platforms` → `Windows`.
2. اختر `Binary Configuration` ➔ `Shipping` (أو `Development` للاختبار).
3. اضغط على **`Package Project`**.
4. حدد مجلد الإخراج (مثلاً `Binaries/Win64`).
5. ستقوم العملية بتوليد ملف `Rok2.exe` جاهز للتشغيل والتحزيم.

---

## 5. خطة بناء اللعبة لهواتف الأندرويد (Android APK)

### المتطلبات المسبقة:
- تثبيت **Android Studio** (نسخة معتمدة لـ UE 5.4).
- ضبط **Android SDK & NDK** داخل Unreal Editor:  
  `Edit` → `Project Settings` → `Platforms` → `Android SDK`.

### خطوات البناء:
1. اذهب إلى قائمة:  
   `Platforms` → `Android`.
2. اضغط على **`Package Project`** واختيار `Android (ASTC)`.
3. سيتولّد ملف **`Rok2-arm64.apk`** في المسار:
   `game/client-unreal/Binaries/Android/`
4. انقل ملف الـ APK إلى هاتفك وثبته مباشرة للاستمتاع باللعبة على الجوال!

---

## 6. ربط السيرفر السحابي المباشر (Production API)

مشروع Unreal Engine موصول ومعد تلقائياً للاتصال بالسيرفر السحابي المباشر:
- **REST API Base URL:** `https://rok2-api.lolelarap.workers.dev`
- **WebSocket Streaming URL:** `wss://rok2-api.lolelarap.workers.dev/v1/world/ws`

يمكنك تعديل المسار في أي وقت عبر متغير `ApiBaseUrl` في كلاس `URok2Api`.

---

**مشروع Unreal Engine 5 جاهز ومكتمل ومعد للتطوير والتجميع المباشر.**
