# ROK2 — توثيق وتصميم لعبة استراتيجية ضخمة (Unreal Engine 5 Core)

**المسار الرئيسي للمشروع:** `c:\Users\kayf\Desktop\rok2`  
**عميل اللعبة الرسمي والوحيد:** **Unreal Engine 5.8 (PC & Android)**  
**المسار المباشر لمشروع UE5:** `c:\Users\kayf\Desktop\rok2\game\client-unreal`  

---

## 📌 ابدأ من هنا

1. `UNREAL_ENGINE_GUIDE.md` — **الدليل الشامل لتطوير، بناء، وتشغيل اللعبة على Unreal Engine 5**
2. `PROJECT_COMPLETION_REPORT.md` — **تقرير المراجعة والتسليم النهائي للمشروع**
3. `01-map/08-map-spec-precise.md` — مواصفات الخريطة الإحداثية 2400²
4. `02-civilizations/civilizations.md` — مصفوفة الحضارات والقادة
5. `05-tech-from-install/AUDIT-CLOSED.md` — نتائج تفكيك لعبة Rise of Kingdoms الأصلية
6. `game/README.md` — دليل السيرفر السحابي والـ API المباشر

---

## 🎮 عميل Unreal Engine 5 (`game/client-unreal`)

- **المحرك المستهدف:** Unreal Engine 5.4+
- **اللغات والموديولات:** C++ (`Rok2` Module) + UMG Widgets
- **المنصات المدعومة:** Windows PC (.exe) + Android (.apk)
- **المحتويات المضمنة:**
  - `URok2Api`: ربط الـ HTTP REST API والـ WebSockets الحية بالسيرفر.
  - `ARok2IsometricCamera`: كاميرا الاستراتيجية المنظورية 50- درجة.
  - `ARok2WorldRenderer`: رسم رقعة العالم 2400² والكائنات 3D.
  - `ARok2CityBuilder`: منشئ ورقعة المدينة التفاعلية.
  - `URok2BootWidget` & `URok2CityWidget`: واجهات UMG لاختيار الحضارات وإدارة المدينة والجيوش.

لتشغيل المشروع، افتح `game/client-unreal/Rok2.uproject` في محرر Unreal Engine 5.

---

## 🌐 السيرفر والخادم السحابي (`game/backend`)

- **الرابط المباشر (Live API):** `https://rok2-api.lolelarap.workers.dev`
- **التقنيات:** Cloudflare Workers + D1 Serverless SQLite + Durable Objects (1s Atomic Tick Engine).
