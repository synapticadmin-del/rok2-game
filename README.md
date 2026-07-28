# ROK2 — لعبة استراتيجية MMO على Unreal Engine 5

**ROK2** مشروع لعبة استراتيجية ضخمة بمستوى Rise of Kingdoms: عميل رسمي وحيد على **Unreal Engine 5 (PC + Android)**، وخادم سحابي **Authoritative** على Cloudflare Workers.

**الحالة الحالية:** نهاية مرحلة الـ Prototype — الخادم يعمل مباشرة، والعميل بهيكل أولي قابل للتشغيل. التفاصيل والتقدم الدقيق في **[PLAN.md](PLAN.md)**.

---

## 📌 ابدأ من هنا

| الملف | الغرض |
|-------|-------|
| **[PLAN.md](PLAN.md)** | **خطة التنفيذ الرئيسية** — الحالة الحقيقية، المراحل، البنود، وسجل الإنجاز |
| **[AGENTS.md](AGENTS.md)** | **دليل المهام** — بروتوكول العمل لأي جلسة تطوير قادمة |
| [UNREAL_ENGINE_GUIDE.md](UNREAL_ENGINE_GUIDE.md) | تشغيل وبناء عميل UE5 (Editor, PC, Android) |
| [INDEX.md](INDEX.md) | فهرس جميع وثائق المشروع |
| [game/README.md](game/README.md) | الخادم السحابي والـ API المباشر |

---

## 🎮 العميل — `game/client-unreal`

- Unreal Engine 5 (5.4+) — موديول C++ باسم `Rok2` + واجهات UMG.
- يشمل: اتصال REST/WebSocket بالخادم، كاميرا استراتيجية، رسم خريطة العالم 2400²، بناء المدينة، واجهات اختيار الحضارة وإدارة الموارد.
- التشغيل: افتح `Rok2.uproject` في محرر UE5 — التفاصيل في [UNREAL_ENGINE_GUIDE.md](UNREAL_ENGINE_GUIDE.md).

## 🌐 الخادم — `game/backend`

- **الرابط المباشر:** `https://rok2-api.lolelarap.workers.dev`
- Cloudflare Workers + D1 + Durable Objects (tick ذري كل ثانية).
- التشغيل المحلي والنشر: [game/docs/RUN.md](game/docs/RUN.md) — مرجع الـ API: [game/docs/API.md](game/docs/API.md).

## 📐 التصميم والبيانات

- مواصفات الخريطة والمناطق والممرات: `01-map/`
- الحضارات الست والقادة: `02-civilizations/`
- أنظمة الاقتصاد والقتال والتحالف: `03-systems/`
- بيانات التوازن الموحدة (يقرأها العميل والخادم): `data/*.json`

---

## 🔁 دورة التطوير

كل جلسة عمل تبدأ بقراءة `PLAN.md` لتحديد أول بند غير مكتمل، تنفذه، تحدّث الخطة، وتسلّم على `main` — البروتوكول كاملاً في [AGENTS.md](AGENTS.md).
