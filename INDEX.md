# فهرس وثائق مشروع ROK2 (Unreal Engine 5)

## 00 التوثيق الرئيسي
- `README.md` — مدخل المشروع ورؤية Unreal Engine 5
- `PLAN.md` — **خطة التنفيذ الرئيسية: الحالة، المراحل، البنود، وسجل الإنجاز**
- `AGENTS.md` — **دليل المهام وبروتوكول العمل للجلسات القادمة**
- `UNREAL_ENGINE_GUIDE.md` — دليل بناء وتشغيل عميل UE5 (PC & Android)
- `PROJECT_COMPLETION_REPORT.md` — تقرير حالة المشروع (تاريخي — المرجع النهائي هو PLAN.md)

## 01 الخريطة الإحداثية 2400²
- `01-map/00-map-overview.md` — نظرة على الـ Zones
- `01-map/01-zone1-eight-regions.md` — المناطق الـ 8 في Zone 1
- `01-map/02-zone2-expansion.md` — مناطق التوسع في Zone 2
- `01-map/03-zone3-finals.md` — المملكة المفقودة Zone 3
- `01-map/04-passes-mountains.md` — ممرات الجبال
- `01-map/08-map-spec-precise.md` — الإحداثيات الدقيقة لجميع المناطق والممرات

## 02 الحضارات والقادة
- `02-civilizations/civilizations.md` — الحضارات الـ 6 والبونصات
- `02-civilizations/commanders.md` — القادة والمهارات
- `02-civilizations/commanders-reference.md` — مرجع القادة

## 03 الأنظمة والقتال
- `03-systems/core-systems.md` — أنظمة المدينة والجيوش والمباني
- `03-systems/balance-starter.md` — أنظمة التوازن

## 04 تفكيك اللعبة الأصلية (Reverse Engineering Audit)
- `05-tech-from-install/install-analysis.md` — تفكيك ملفات Unity IL2CPP
- `05-tech-from-install/DEEP-REVERSE-AUDIT.md` — التدقيق العميق
- `05-tech-from-install/AUDIT-CLOSED.md` — وثيقة ختم التدقيق

## 05 خطة الطريق
- `06-implementation/roadmap.md` — الرؤية التاريخية للمراحل (المرجع التنفيذي النهائي: `PLAN.md`)

## 06 العميل والخادم
- `game/client-unreal/` — مشروع عميل Unreal Engine 5 (C++ & UMG)
- `game/client-unreal/Docs/README.md` — دليل عميل UE5 التفصيلي
- `game/client-unreal/Docs/BUILD_ANDROID.md` — بناء نسخة الأندرويد APK
- `game/backend/` — الخادم السحابي Authoritative (Cloudflare Workers + D1 + Durable Objects)
- `game/docs/API.md` — مرجع الـ REST/WebSocket API
- `game/docs/RUN.md` — تشغيل ونشر الخادم
- `game/docs/BACKEND.md` — معمارية الخادم
- `data/` — ملفات JSON الموحدة للتوازن (civilizations, zones, passes, buildings, troop_tiers, map_spec_coordinates)
