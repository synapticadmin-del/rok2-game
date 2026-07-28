# دليل تطوير وبناء لعبة ROK2 على محرك Unreal Engine 5 (UE5 Guide)

**مسار مشروع المحرك داخل الريبو:** `game/client-unreal/`
**نسخة المحرك:** Unreal Engine 5.4 أو أحدث (أي نسخة UE5 مستقرة ≥ 5.4 — الكود لا يعتمد على ميزات نسخة بعينها).

---

## 1. الرؤية الهندسية

العميل الرسمي والوحيد للعبة **ROK2** مبني على **Unreal Engine 5** بلغة **C++** لتحقيق أعلى أداء ورسوميات 3D تنافس ألعاب الـ 4X Strategy MMO.

### المكوّنات البرمجية (C++):

1. **`URok2Api`** — الاتصال بالخادم: HTTP REST + WebSocket حي (قراءة الخريطة 2400²، إرسال المسيرات).
2. **`ARok2IsometricCamera`** — كاميرا استراتيجية بزاوية -50 درجة مع Pan & Zoom سلس.
3. **`ARok2PlayerController`** — مدخلات الماوس (PC) واللمس (Android).
4. **`ARok2WorldRenderer`** — رسم كائنات العالم: المدن، الممرات، الموارد، الجيوش.
5. **`ARok2CityBuilder`** — بناء وتثبيت المباني في رقعة المدينة.
6. **`URok2BootWidget` / `URok2CityWidget`** — واجهات UMG: تسجيل الدخول، اختيار الحضارات الـ 6، الموارد، التدريب، التحالفات.

---

## 2. هيكلية ملفات C++

```
game/client-unreal/Source/Rok2/
├── Rok2.Build.cs              # تبعيات الموديول (HTTP, WebSockets, Json, UMG)
├── Rok2.cpp                   # تنفيذ الموديول
├── Public/
│   ├── Rok2Types.h            # DTOs (FPlayerState, FCityState, FPassState...)
│   ├── Rok2Api.h              # REST API + WebSockets
│   ├── Rok2GameMode.h         # إدارة دورة اللعبة
│   ├── Rok2PlayerController.h # المدخلات واللمس
│   ├── Rok2IsometricCamera.h  # الكاميرا الاستراتيجية
│   ├── Rok2CityBuilder.h      # رقعة المدينة والمباني
│   ├── Rok2WorldRenderer.h    # رسم خريطة العالم 2400²
│   ├── Rok2ProceduralAssets.h # خامات وألوان ديناميكية
│   ├── Rok2BootWidget.h       # واجهة البداية
│   ├── Rok2CityWidget.h       # واجهة المدينة والجيوش
│   └── Rok2BlueprintLibrary.h # دالات Blueprints
└── Private/                   # تنفيذ جميع الملفات أعلاه
```

---

## 3. التشغيل داخل Unreal Editor

1. افتح `game/client-unreal/Rok2.uproject` بنقرة مزدوجة.
2. إذا سأل المحرك عن النسخة، اختر أحدث UE5 مثبت لديك (≥ 5.4).
3. سيتولى المحرك توليد ملفات Visual Studio وتجميع كود C++ تلقائياً.
4. اضغط **Play (PIE)** للتجربة داخل المحرر.

---

## 4. بناء نسخة الكومبيوتر (Windows PC)

1. في المحرر: `Platforms` → `Windows`.
2. `Binary Configuration` → `Shipping` (أو `Development` للاختبار).
3. `Package Project` → حدد مجلد الإخراج (مثلاً `Binaries/Win64`).
4. الناتج: `Rok2.exe` جاهز للتحزيم.

---

## 5. بناء نسخة الأندرويد (Android APK)

التفاصيل الكاملة في [game/client-unreal/Docs/BUILD_ANDROID.md](game/client-unreal/Docs/BUILD_ANDROID.md). باختصار:

1. ثبّت **Android Studio** واضبط SDK/NDK من: `Edit` → `Project Settings` → `Platforms` → `Android SDK`.
2. `Platforms` → `Android` → `Package Project` → `Android (ASTC)`.
3. الناتج: `Rok2-arm64.apk` في `game/client-unreal/Binaries/Android/` — انقله للهاتف وثبّته.

---

## 6. الربط بالخادم السحابي

- **REST API:** `https://rok2-api.lolelarap.workers.dev`
- **WebSocket:** `wss://rok2-api.lolelarap.workers.dev/v1/world/ws`
- المسار قابل للتعديل عبر `ApiBaseUrl` في كلاس `URok2Api`.

---

*للمهام التنفيذية القادمة على العميل، راجع بنود المرحلة الحالية في [PLAN.md](PLAN.md).*
