# ROK2 — خطة التنفيذ الرئيسية (Master Execution Plan)

> **الهدف النهائي:** لعبة استراتيجية MMO كاملة بمستوى Rise of Kingdoms، عميلها الوحيد **Unreal Engine 5 (PC + Android)** وخادمها **Cloudflare Workers + D1 + Durable Objects**.
>
> **طريقة استخدام هذا الملف:** كل جلسة تطوير تبدأ بقراءة هذا الملف لتحديد أول بند غير مكتمل `[ ]`، تنفذه، ثم تحدّث هذا الملف نفسه (تعليم `[x]` + تدوين الملاحظات) وتعمل commit على `main`.
>
> **آخر تحديث:** 2026-07-28 — P2-T3 شجرة البحث (المرحلة 2).

---

## 0. الحالة الحقيقية للمشروع (Ground Truth)

| المكوّن | الحالة | الملاحظات |
|---------|--------|-----------|
| التوثيق والتصميم | ✅ شبه مكتمل | خريطة 2400²، 6 حضارات، أنظمة أساسية موثقة في `01`–`06` |
| بيانات JSON الموحدة | ✅ جاهزة | `data/*.json` (civilizations, zones, passes, buildings, troop_tiers, map_spec_coordinates) |
| Backend (Cloudflare) | 🟡 Prototype يعمل | Auth ضيف + مدينة + مبانٍ + جنود + تحالفات + ممرات + تقارير قتال + WebSocket. ناجح في smoke E2E |
| عميل UE5 (C++) | 🟡 هيكل أولي يعمل | موديول `Rok2` (Api, Camera, WorldRenderer, CityBuilder, Boot/City Widgets) — أشكال هندسية بدائية بدل الأصول الفنية |
| الأصول الفنية 3D/UI | ❌ غير موجودة | حالياً Engine basic shapes + materials ملوّنة runtime |
| الأنظمة المتقدمة | 🟡 بدأت | قادة (P2-T1 ✅) + مستشفى (P2-T2 ✅: جرحى بالسعة + شفاء زمني)؛ بحث جزئي (stubs)؛ لم تُنفّذ بعد: أحداث، مواسم، KvK، متجر، Battle Pass |
| الاختبار الآلي | ❌ محدود | smoke.mjs فقط على الـ backend |

**الخلاصة:** المشروع في نهاية المرحلة 1 (Prototype) وليس "مكتملاً 100%". أي تقرير يقول غير ذلك قديم.

---

## 1. قرارات معمارية مقفلة (لا تُعاد مناقشتها)

1. **العميل:** Unreal Engine 5 C++ فقط — لا عميل ويب. (الخلافات الموثقة حول 5.4/5.8 تُحسم: **الهدف أحدث UE5 مستقر 5.4+، والكود يجب أن يبني على أي منها**)
2. **الخادم:** Cloudflare Workers + D1 + Durable Object `KingdomShard` — Authoritative، tick ذري كل 1 ثانية.
3. **خريطة واحدة موسمية ثابتة بثلاث Zones** — لا KvK متعدد الخرائط قبل اكتمال الموسم الأول.
4. **كل بيانات التوازن تعيش في `data/*.json`** ويقرأها الطرفان (client + server) — لا قيم ثابتة مكررة في الكود.
5. **اللغة في التوثيق:** العربية أساسية مع مصطلحات تقنية إنجليزية.
6. **التسليم:** تعديل مباشر على `main` مع commit واضح + تحديث هذا الملف في نفس الـ commit.

---

## 2. خارطة الطريق التنفيذية (المراحل والبوابات)

### المرحلة 1 — إنهاء الـ Prototype ⬅️ *المرحلة الحالية*
الهدف: لاعبان حقيقيان على نفس السيرفر يتنازعان ممراً عبر عميل UE5 بدون شرح.

- [x] Backend: auth ضيف + مدينة + مبانٍ + تدريب + تحالف + ممرات + تقارير قتال
- [x] Backend: smoke E2E (`scripts/smoke.mjs`) ضد البيئة الحية
- [x] Client: اتصال REST + WebSocket بكلاس `URok2Api`
- [x] Client: كاميرا استراتيجية + رسم خريطة العالم + بناء مدينة أساسي
- [x] **P1-T1** توحيد نسخة المحرك والتوثيق (تنظيف تضارب 5.4/5.8 في كل الملفات) — ✅ تم 2026-07-28
- [x] **P1-T2** Client: شاشة تحميل + إعادة اتصال تلقائي + معالجة أخطاء الشبكة في `URok2Api` — ✅ تم 2026-07-28
- [x] **P1-T3** Client: مسيرات الجيوش مرئية على الخريطة (أيقونة متحركة من المدينة للهدف حسب زمن الـ march) — ✅ تم 2026-07-28
- [x] **P1-T4** Client: واجهة تقرير القتال (`URok2BattleReportWidget`) تعرض الخسائر من الـ API — ✅ تم 2026-07-28
- [x] **P1-T5** Client: شريط موارد حي يتحدث عبر WebSocket بدون polling يدوي — ✅ تم 2026-07-28
- [x] **P1-T6** Backend: endpoint لسحب بيانات `data/*.json` (buildings/civs/tiers) بدل القيم الثابتة — ✅ تم 2026-07-28
- [x] **P1-T7** اختبار لاعبين: سيناريو موثق في `game/docs/E2E_TWO_PLAYERS.md` + نجاحه فعلياً — ✅ تم 2026-07-28 (E2E TWO PLAYERS PASSED ضد الإنتاج)
- **🚪 بوابة النجاح:** ✅ **محققة 2026-07-28** — لاعبان يتنافسان على ممر ويستلمان تقارير قتال صحيحة (مثبت آلياً بـ e2e_two_players.mjs ضد الإنتاج).

### المرحلة 2 — Vertical Slice
- [x] **P2-T1** نظام القادة: بيانات `data/commanders.json` + عمولات/مهارات أساسية (attack/defense/passive واحدة لكل قائد) — ✅ تم 2026-07-28
- [x] **P2-T2** المستشفى: استقبال الجرحى حسب السعة + شفاء زمني عبر الـ tick — ✅ تم 2026-07-28
- [x] **P2-T3** شجرة البحث: Economic + Military tiers من `data/research.json` (إنشاء الملف) — ✅ تم 2026-07-28
- [ ] **P2-T4** Zone 2 stubs: مناطق مقفلة بمؤقّت زمني + مناطق موارد أعلى
- [ ] **P2-T5** تحالف كامل: helps بين الأعضاء + rally على الممرات + رتب
- [ ] **P2-T6** Client: HUD موحد (موارد، طوابير، إشعارات) بأسلوب UMG احترافي
- [ ] **P2-T7** أصول فنية أولية: استيراد موديلات مجانية (Kenney/Quaternius) للمباني والوحدات في `Content/Art/`
- **🚪 بوابة النجاح:** جلسة لعب 30 دقيقة متواصلة بدون أخطاء قاتلة، وفيها بناء + بحث + قتال + تحالف.

### المرحلة 3 — Season Alpha
- [ ] **P3-T1** جدول فتح الـ Zones كاملاً على السيرفر (Zone unlock service)
- [ ] **P3-T2** Zone 3 core contest + تسجيل نقاط الموسم
- [ ] **P3-T3** أحداث يومية/أسبوعية (barbarians, resource rush)
- [ ] **P3-T4** متجر sandbox + speedups + VIP أساسي (بدون مدفوعات حقيقية)
- [ ] **P3-T5** Soft launch: مملكة واحدة أو اثنتان + قياس retention
- **🚪 بوابة النجاح:** موسم ألفا كامل يُلعب حتى النهاية مع تتويج تحالف فائز.

### المرحلة 4 — Live
- [ ] Battle Pass، قادة إضافيون، anti-cheat، matchmaking ممالك، تحسين أداء UA
- **🚪 بوابة النجاح:** نشر عام + مؤشرات أداء حية مستقرة.

---

## 3. ترتيب بناء الخريطة تقنياً (قاعدة ثابتة)

1. Tilemap + mountain collision
2. Region polygons + membership test
3. Pass entities + traverse rules
4. Pathfinding region-aware
5. AOI replication
6. Holy sites timers
7. Season unlock service
8. Zone3 scoring

---

## 4. سجل الإنجاز (Session Log)

> كل جلسة تضيف سطراً هنا عند الانتهاء: التاريخ، البند المنفّذ، رقم الـ commit، وأي ملاحظات.

| التاريخ | البند | Commit | ملاحظات |
|---------|-------|--------|---------|
| 2026-07-28 | إنشاء PLAN.md + AGENTS.md + تحديث التوثيق الجذري | 49b07fe | البداية الرسمية للخطة التنفيذية |
| 2026-07-28 | P1-T1 توحيد نسخة المحرك والتوثيق | 49b07fe | حسم التضارب: UE 5.4+ في كل الملفات |
| 2026-07-28 | P1-T2 شاشة تحميل + إعادة اتصال + معالجة أخطاء الشبكة | 8f0df32 | URok2Api: HTTP retry backoff + WS auto-reconnect + OnConnectionState؛ BootWidget: شاشة تحميل؛ CityWidget: شارة اتصال |
| 2026-07-28 | P1-T3 مسيرات الجيوش مرئية على الخريطة | c9b951e | ParseWorld يقرأ marches؛ أحداث WS (march_created/returning/update)؛ WorldRenderer: اتجاه + ألوان تحالف + حالة returning |
| 2026-07-28 | P1-T4 واجهة تقرير القتال | 0bc5eda | Rok2Types: FRok2BattleReport؛ Api: بارس reports + WS battle_report لحظي؛ Widget كامل: قائمة + تفاصيل خسائر (قتلى/خطير/خفيف)؛ زر 📜 في CityWidget |
| 2026-07-28 | P1-T5 شريط موارد حي عبر WebSocket | 6376641 | FRok2City.Rates من المباني؛ LoadCity تلقائي كل 30s عند اتصال WS؛ حدث city_upsert يزامن فورياً؛ عدادات +X/س في CityWidget |
| 2026-07-28 | P1-T6 endpoint بيانات JSON الموحدة | 8bc3b12 | /v1/meta/all في router.ts؛ URok2Api::FetchMeta يسحب عند Init مع fallback؛ معدلات الإنتاج وقائمة وحدات التدريب من الخادم؛ API.md محدث |
| 2026-07-28 | P1-T7 اختبار لاعبين E2E — بوابة المرحلة 1 محققة | 6588337 | scripts/e2e_two_players.mjs + game/docs/E2E_TWO_PLAYERS.md؛ نجح ضد الإنتاج (لاعبان/تحالفان/نزاع ممر/تقارير صحيحة)؛ تحذير: الـ API المُنشر أقدم من main — يحتاج wrangler deploy |
| 2026-07-28 | P2-T1 نظام القادة (بيانات + مهارات + باف قتال) | e9621fc | data/commanders.json (12 قائداً، لكلٍ attack/defense/passive) + migration 0002 (player_commanders/march_commanders)؛ sim/commanders.ts يحسب بافات من JSON ويربطها في resolveCombat (هجوم للطرفين + دفاع للمدافع بسقف 50%)؛ endpoints: GET /v1/commanders، summon/levelup/skill/assign + starter commander مع city/init + خبرة من القتال؛ اجتاز commanders_offline_test.mjs (10/10) + commanders_test.mjs جاهز للإنتاج |
| 2026-07-28 | P2-T2 المستشفى (جرحى بالسعة + شفاء زمني) | d8d78ff | data/buildings.json يكسب hospital config (سعة 200+150/مستوى، شفاء بنصف تكلفة التدريب، 5ث/جندي)؛ sim/hospital.ts؛ KingdomShard يقبَص الجرحى الخطيرين بالسعة عند كل معركة (الفائض يموت) ويخصم الخسائر من marching + ملخص hospital في تقرير المعركة؛ /v1/city/heal بتكلفة موارد + GET /v1/city يعرض wounded + hospital{level,capacity,used,free}؛ اجتاز hospital_offline_test.mjs (11/11) + hospital_test.mjs جاهز للإنتاج |
| 2026-07-28 | P2-T3 شجرة البحث (economy + military) | 8e61f29 | data/research.json جديد (10 تقنيات × 5 مستويات × فرعين مع prerequisites)؛ migration 0003 player_research؛ sim/research.ts يقرأ JSON؛ اكتمال طابور research يكتب المستوى ويبث tech_researched؛ GET /v1/research + /v1/city/research يقرأ المستوى من D1 ويتحقق أكاديمية/prerequisites؛ بافات مطبقة: إنتاج + تدريب + سرعة مسير + هجوم قتال؛ اجتاز research_offline_test.mjs (13/13) + research_test.mjs جاهز للإنتاج |

---

## 5. ملفات مرجعية سريعة

- `README.md` — مدخل المشروع
- `INDEX.md` — فهرس كل الوثائق
- `UNREAL_ENGINE_GUIDE.md` — تشغيل وبناء عميل UE5
- `game/docs/API.md` — مرجع الـ REST/WebSocket API
- `game/docs/RUN.md` — تشغيل ونشر الـ backend
- `data/*.json` — بيانات التوازن الموحدة
- `06-implementation/roadmap.md` — الرؤية التاريخية (تُقرأ للسياق، والمرجع النهائي هو هذا الملف)
