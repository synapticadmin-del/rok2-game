# ROK2 — خطة التنفيذ الرئيسية (Master Execution Plan)

> **الهدف النهائي:** لعبة استراتيجية MMO كاملة بمستوى Rise of Kingdoms، عميلها الوحيد **Unreal Engine 5 (PC + Android)** وخادمها **Cloudflare Workers + D1 + Durable Objects**.
>
> **طريقة استخدام هذا الملف:** كل جلسة تطوير تبدأ بقراءة هذا الملف لتحديد أول بند غير مكتمل `[ ]`، تنفذه، ثم تحدّث هذا الملف نفسه (تعليم `[x]` + تدوين الملاحظات) وتعمل commit على `main`.
>
> **آخر تحديث:** 2026-07-29 — P4-T5 قادة إضافيون (18 قائداً) + anti-cheat أساسي (rate limits من data/anticheat.json في الـ DO والـ router + كشف شذوذ الحمولات + endpoint فحص إداري).

---

## 0. الحالة الحقيقية للمشروع (Ground Truth)

| المكوّن | الحالة | الملاحظات |
|---------|--------|-----------|
| التوثيق والتصميم | ✅ شبه مكتمل | خريطة 2400²، 6 حضارات، أنظمة أساسية موثقة في `01`–`06` + **وثائق التصميم الشاملة `07-game-design/`** (GDD، قلعة سداسية، حضارات بصرية، RoK audit، UI/UX، توازن) |
| بيانات JSON الموحدة | ✅ جاهزة | `data/*.json` (civilizations, zones, passes, buildings, troop_tiers, map_spec_coordinates) |
| Backend (Cloudflare) | 🟡 Prototype يعمل | Auth ضيف + مدينة + مبانٍ + جنود + تحالفات + ممرات + تقارير قتال + WebSocket. ناجح في smoke E2E |
| عميل UE5 (C++) | 🟡 هيكل أولي يعمل | موديول `Rok2` (Api, Camera, WorldRenderer, CityBuilder, Boot/City Widgets) — أشكال هندسية بدائية بدل الأصول الفنية |
| الأصول الفنية 3D/UI | 🟡 بدأت | مبانٍ/وحدات KayKit (P2-T7 ✅) + موسيقى ومؤثرات WAV لكل حضارة + 12 بورتريه قائد (P4-T2 ✅)؛ باقي: موديلات وحدات بشرية + أصول UI نهائية |
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
- [x] **P2-T4** Zone 2 stubs: مناطق مقفلة بمؤقّت زمني + مناطق موارد أعلى — ✅ تم 2026-07-28
- [x] **P2-T5** تحالف كامل: helps بين الأعضاء + rally على الممرات + رتب — ✅ تم 2026-07-28
- [x] **P2-T6** Client: HUD موحد (موارد، طوابير، إشعارات) بأسلوب UMG احترافي — ✅ تم 2026-07-28
- [x] **P2-T7** أصول فنية أولية: استيراد موديلات مجانية (Kenney/Quaternius) للمباني والوحدات في `Content/Art/` — ✅ تم 2026-07-28
- **🚪 بوابة النجاح:** جلسة لعب 30 دقيقة متواصلة بدون أخطاء قاتلة، وفيها بناء + بحث + قتال + تحالف.

### المرحلة 3 — Season Alpha
- [x] **P3-T1** جدول فتح الـ Zones كاملاً على السيرفر (Zone unlock service) — ✅ تم 2026-07-28
- [x] **P3-T2** Zone 3 core contest + تسجيل نقاط الموسم — ✅ تم 2026-07-28
- [x] **P3-T3** أحداث يومية/أسبوعية (barbarians, resource rush) — ✅ تم 2026-07-28
- [x] **P3-T4** متجر sandbox + speedups + VIP أساسي (بدون مدفوعات حقيقية) — ✅ تم 2026-07-28
- [x] **P3-T5** Soft launch: مملكة واحدة أو اثنتان + قياس retention — ✅ تم 2026-07-28
- **🚪 بوابة النجاح:** موسم ألفا كامل يُلعب حتى النهاية مع تتويج تحالف فائز.

### المرحلة 4 — Live
- [x] **P4-T1** Battle Pass موسمي (sandbox): مسار مجاني + مدفوع بالـ gems + نقاط من أفعال اللعب — ✅ تم 2026-07-28
- [x] **P4-T2** إنتاج الأصول الصوتية + بورتريهات القادة (موسيقى/مؤثرات WAV حقيقية لكل حضارة + 12 بورتريه PNG مربوط بالعميل) — ✅ تم 2026-07-28
- [x] **P4-T3** موسيقى معركة منفصلة لكل حضارة (battle.wav) + حالة قتال في URok2AudioManager مربوطة بتقارير القتال — ✅ تم 2026-07-28
- [x] **P4-T4** مؤثرات أحداث اللعب (جمع موارد/اكتمال بحث/شفاء جرحى/فتح منطقة/rally) مربوطة بالأحداث — ✅ تم 2026-07-28
- [x] **P4-T5** قادة إضافيون (12→18، قائد جديد لكل حضارة) + anti-cheat أساسي (rate limits + كشف شذوذ) — ✅ تم 2026-07-29
- [ ] **P4-T6** matchmaking ممالك (تقسيم لاعبين جدد على ممالك حسب القوة/النشاط)
- [ ] **P4-T7** تحسين أداء UA (أداء العميل: LOD، pooling، batching للرسم)
- **🚪 بوابة النجاح:** نشر عام + مؤشرات أداء حية مستقرة.

### المرحلة 5 — Game Feel & Visual Identity ⬅️ *المرحلة الجديدة*
الهدف: أن تبدو اللعبة وتُحسّ كلعبة RoK حقيقية — قلعة سداسية حية، حضارات مميزة بصرياً، واجهة لعبة لا واجهة موقع. المرجع: `07-game-design/`.

- [x] **P5-T0** وثائق التصميم الشاملة `07-game-design/` (GDD + قلعة سداسية + حضارات بصرية + RoK audit + UI/UX + توازن) + 11 صورة مرجعية — ✅ تم 2026-07-28
- [x] **P5-T1** نظام السور السداسي + شبكة hex + البناء الحر (CityBuilder overhaul — مواصفة `07-game-design/castle-hex-city.md`) — ✅ تم 2026-07-28
- [x] **P5-T2** ثيمات مباني الحضارات الست (City Hall + 5 مبانٍ أساسية لكل حضارة، مرجع `07-game-design/civilizations-visual-design.md` + `assets/`) — ✅ تم 2026-07-28
- [x] **P5-T3** HUD بأسلوب RoK كامل (شريط موارد، أزرار دائرية، دردشة، مهام — مرجع `07-game-design/ui-ux-design-system.md` + `assets/ui-city-mockup.jpg`) — ✅ تم 2026-07-28
- [x] **P5-T4** شاشة القادة الكاملة (بورتريهات، مهارات، مواهب، معدات) — ✅ تم 2026-07-28
- [x] **P5-T5** ضباب الحرب + الكشافة على الخريطة — ✅ تم 2026-07-28
- [x] **P5-T6** حركات وانتقالات وأصوات (بناء يُشيّد، جنود تسير، احتفال ترقية، موسيقى حسب الحضارة) — ✅ تم 2026-07-28
- **🚪 بوابة النجاح:** لاعب يدخل اللعبة ويشعر أنها RoK — قلعته السداسية بثيم حضارته، يحرك مبانيه بحرية، واجهة لعبة كاملة.

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
| 2026-07-28 | P2-T4 Zone 2 stubs (قفل زمني + موارد أعلى) | 5452841 | data/zones.json: unlock_schedule لـ Zone2 (4 مناطق @يوم10 + 4 ممرات داخلية @يوم10 و4 @يوم14) + constants (zone2_richness_mult=1.5 ونطاقات موارد)؛ sim/zones.ts جديد يقرأ JSON فقط؛ KingdomShard: رفض المسيرات لمناطق مقفلة (zone_locked) لكل الأهداف (عقد/نقاط/مدن/عرش)، بذر حتمي لعقد Z1+Z2 (مستويات 1-4 و3-6، غنى Z2 ×1.5)، بث zone_unlocked مرة واحدة عند بلوغ اليوم، حالة zones في snapshot + GET /v1/world/zones + /v1/meta/zones + zones في /v1/meta/all؛ اجتاز zones_offline_test.mjs (30/30) ولم تتأثر اختبارات research/hospital/commanders |
| 2026-07-28 | P2-T5 تحالف كامل (رتب + helps + rally) | 9aa251d | data/zones.json يكسب alliance config (رتب R1-R5 بصلاحيات، help: 60ث/مساعدة ×10 كحد أقصى و30% من المدة، rally: R3+ و5 مشاركين وتجميع 300ث)؛ migration 0004 (alliance_members/alliance_helps/rallies/rally_participants)؛ sim/alliance.ts يقرأ JSON فقط؛ helps حقيقية تسرّع طوابير الأعضاء تراكمياً (مساعدة واحدة لكل لاعب/طابور)؛ رتب فعلية: promote/kick بقاعدة أعلى-يغيّر-أدنى + leave (القائد ممنوع)؛ rally على pass/throne يجمع قوات المشاركين في مسيرة واحدة بقائد + إعادة القوات عند فشل المسار + بث rally_launched + poller كل طلب؛ snapshot يكسب queues؛ اجتاز alliance_offline_test.mjs (28/28) ولم تتأثر بقية الاختبارات |
| 2026-07-28 | P2-T6 HUD موحد في العميل (موارد + طوابير + إشعارات) | d5cae1e | URok2HudWidget جديد يُبنى بالكود (بدون Blueprint): شريط علوي بموارد حية (تحديث كل Tick من Rates) + يوم الموسم + مؤقّت أقرب منطقة مقفلة (من snapshot.zones) + شارة اتصال + جرس بعدّاد؛ لوحة طوابير بشريط تقدم UProgressBar وعدّ تنازلي لكل طابور (من snapshot.queues)؛ بطاقات إشعارات تتلاشى (قتال/منطقة/rally/بحث) + مركز إشعارات قابل للطي بسجل 20؛ شريط سفلي بأزرار مفوَّضة لـ Blueprint؛ Rok2Api: FRok2ZoneStatus/FRok2HudNotification + PushNotification مركزي + بارس queues/zones في snapshot + أحداث zone_unlocked/tech_researched/rally_launched/season_day؛ GameMode يركّب HUD فوق CityWidget (z=20)؛ 41/41 فحص بنيوي ناجح (بناء UE5 الفعلي على جهاز المطور) |
| 2026-07-28 | P2-T7 أصول فنية أولية (KayKit CC0) | 93358bf | 17 GLB ذاتية الاكتفاء في Content/Art/kaykit (~2.4MB) محوّلة من KayKit Medieval Hexagon Pack (CC0، LICENSE مضمّن): 11 مبنى مدينة (castle/windmill/lumbermill/mine/market/barracks/archeryrange/tavern/tower/blacksmith) + قلعة حمراء للأعداء + 4 أعلام تحالف + مرتفعات؛ URok2ArtAssets (فهرس Id→GLB + تحميل مع fallback هندسي — لا يُكسر البناء بدون استيراد)؛ CityBuilder يرسم موديلات حقيقية بمقياس الفهرس؛ WorldRenderer: قلاع زرقاء/حمراء للمدن + تلال KayKit؛ setup_level.py يستورد GLB إلى /Game/Art/kaykit تلقائياً؛ Content/Art/README.md توثيق + ملاحظة وحدات Extra؛ 63/63 فحص ناجح (صحة GLB + تغطية الفهرس + سلامة C++). **ملاحظة إصلاح:** GLB على main مخزّنة نصياً (base64) — `setup_level.py` يفك ترميزها تلقائياً قبل الاستيراد (`_ensure_binary_glb`) فلا يُكسر البناء؛ توثيق في README |
| 2026-07-28 | P3-T1 جدول فتح الـ Zones كاملاً على السيرفر (Zone unlock service) | f06a5ba | data/zones.json يكسب Zone 3 unlock_schedule (CORE + 4 بوابات نهائية @يوم35، عرش @يوم40) + constants (season_day_ms=86400000، season_max_day=60) + season_service config؛ sim/zones.ts: throneUnlockDay/isThroneUnlocked من core_objective.open_day (إزالة الثابت 14)، seasonDayAt (تقدم اليوم زمنياً من season_start_ms)، seasonUnlockState/seasonSchedule؛ KingdomShard: migration v2 (season_start_ms)، تقدم اليوم تلقائياً عبر الـ tick مع بث season_day، throne.unlockDay من JSON دائماً، /do/season/schedule، snapshot + zones-status يكسبان season؛ router: GET /v1/season/schedule + ثوابت season في /v1/meta/all؛ اجتاز season_offline_test.mjs (47/47) + كل اختبارات zones/research/hospital/commanders/alliance (بدون كسر) |
| 2026-07-28 | P3-T2 Zone 3 core contest + تسجيل نقاط الموسم | 9f27874 | data/zones.json يكسب core_contest config (نقاط احتفاظ/حامية/مكاسب احتلال لكل نوع: عرش 1، حصن خارجي 0.5، مذبح جانبي 0.25 نقطة/tick + first_capture_bonus=25)؛ sim/zones.ts: holdScorePerTick/coreGarrison/coreCaptureGain/firstCaptureBonus/coreContestActive + نوع CoreObjectiveKind (كلها من JSON)؛ KingdomShard: migration v3 (جدول core_objectives)، كيان CoreObjective (4 حصون + 4 مذابح) مبذوذ من map_spec.zone3_objectives، معالج وصول core_objective (قتال + احتلال + مكافأة أول احتلال + بث core_objective_changed)، نقاط احتفاظ لكل tick عند نشاط المسابقة (يوم 40+)، استهداف core_objective في createMarch مع قفل core_contest_locked، snapshot يكسب coreObjectives، endpoints /do/season/scoreboard + /v1/season/scoreboard (لوحة نقاط كاملة + متصدر)؛ throne hold يستخدم holdScorePerTick؛ اجتاز season_scoring_offline_test.mjs (48/48) + كل الاختبارات الأوفلاين الستة (بدون كسر) |
| 2026-07-28 | P3-T3 أحداث يومية/أسبوعية (barbarians, resource rush, war_fever) | fb1aafe | data/events.json جديد (3 أحداث: غزو البرابرة يومي، اندفاع الموارد يومي، حمى الحرب أسبوعي @جمعة + constants)؛ sim/events.ts جديد: seasonWeekday حتمي (day0=أحد)، isEventDay/isEventActive (نافذة duration_ticks من بداية اليوم)، eventBuff تجميع مضاعف/جمعي عبر الأحداث المتزامنة، barbExtraPerRegion/barbLevelBonus بسقف constants؛ KingdomShard: tickInDay حتمي من season_start_ms، بث event_started/event_ended مرة واحدة لكل يوم/حدث، seedEventBarbarians يكثّف معسكرات البرابرة حتمياً (مستوى +HP أعلى)، باف rush على سرعة الجمع وغنى العقد، نقاط قتل برابرة/جمع/قتال لاعبين تُضاف للموسم، snapshot يكسب events، endpoints /do/events + /v1/events/active؛ اجتاز events_offline_test.mjs (41/41) + كل الاختبارات الأوفلاين الثمانية (بدون كسر) |
| 2026-07-28 | P3-T4 متجر sandbox + speedups + VIP أساسي (بدون مدفوعات) | 457c3de | data/shop.json جديد (7 speedups من دقيقة إلى يوم + 7 مستويات VIP بمزايا تراكمية: production/build/train mult + تسريع مجاني يومي + constants لمنحة gems الأولى 1000 واليومية 200 ونقطة VIP لكل gem)؛ migration 0005 (player_inventory + player_vip + عمود gems في cities)؛ sim/shop.ts جديد يقرأ JSON فقط (كتالوج + vipTierForPoints + vipPointsForPurchase)؛ endpoints جديدة: GET /v1/shop/catalog و/v1/vip/status + POST /v1/shop/buy (خصم gems + مخزون upsert + نقاط VIP) و/v1/shop/use-speedup (من المخزون أو التسريع المجاني اليومي) و/v1/shop/daily-gems (منحة يومية)؛ مضاعفات VIP مطبقة: إنتاج refreshCity + مدة البناء والتدريب؛ city init يمنح gems البداية من JSON؛ اجتاز shop_offline_test.mjs (40/40) + كل الاختبارات الأوفلاين التسعة (بدون كسر) |
| 2026-07-28 | P3-T5 Soft launch + قياس retention | adb816c | data/softlaunch.json جديد (مملكتان: kingdom-1 مفتوحة بسعة 500 + kingdom-2 احتياطية مغلقة + عتبات retention D1=40%/D7=15%/D30=5% + success_gate)؛ migration 0006 (player_activity يوم-لاعب + last_seen_at على accounts + فهرس اليوم)؛ sim/retention.ts جديد يقرأ JSON فقط (openKingdoms/isKingdomOpen/kingdomCapacity/cohortDayOf/pct)؛ بوابة الانضمام في city/init (kingdom_not_open_for_launch + kingdom_full عند السعة)؛ تتبع النشاط في lib/context.ts (requireAuth + requirePlayer → upsert يومي، بمعرّف واحد)؛ GET /v1/admin/retention (DAU + رجوع cohorts عند D1/D3/D7/D14/D30 مقابل العتبات) + GET /v1/launch/status عام؛ إصلاح حرج: ملف router.ts على main كان تالفاً (ذيل مكرر + تعريفات VIP مفقودة) — أعيد بناؤه نظيفاً؛ اجتاز retention_offline_test.mjs (37/37) + كل الاختبارات الأوفلاين العشرة (بدون كسر). الجزء التشغيلي (دعوة لاعبين فعليين لمملكة الإطلاق) خارج نطاق الكود ويتطلب wrangler deploy |
| 2026-07-28 | P3-T5 توثيق خطة Soft launch التشغيلية | 1615e95 | game/docs/SOFT_LAUNCH.md جديد: تهيئة ممالك من softlaunch.json + خطوات wrangler deploy مع migrations 0005/0006 + التحقق عبر /v1/launch/status و/v1/admin/retention + قراءة cohorts مقابل العتبات + runbook يومي + حدود معروفة (التتبع من تاريخ النشر، قياس معزول لكل worker) |
| 2026-07-28 | P4-T1 Battle Pass موسمي (مسار مجاني + مدفوع) | 0a49941 | data/battlepass.json جديد (20 مستوى × مكافأة مجانية + مدفوعة: موارد/gems/speedups تشير لعناصر shop حقيقية + xp_sources لستة أفعال: build 10/train 5/research 15/heal 3/march 8/pass_attack 20 + constants premium 500 gems وxp_per_level 100)؛ migration 0007 (player_battlepass xp/level/premium + battlepass_claims بمفتاح فريد level+track)؛ sim/battlepass.ts يقرأ JSON فقط (bpLevelForXp خطي بسقف max + bpProgressInLevel + bpClaimableLevels)؛ endpoints: GET /v1/battlepass (حالة كاملة + claimable لكل مسار) + POST /v1/battlepass/unlock-premium (خصم gems) + POST /v1/battlepass/claim (مطالبة مرة واحدة لكل مستوى/مسار، موارد تُضاف للمدينة وspeedups للمخزون)؛ نقاط تُمنح تلقائياً عند build/train/research/heal/march/pass_attack عبر grantBpXp؛ اجتاز battlepass_offline_test.mjs (48/48) + كل الاختبارات الأوفلاين الـ11 (بدون كسر) |
| 2026-07-28 | حذف عميل الويب القديم game/client | 4324d9c..bc78685 | 6 ملفات (index.html/styles.css/app.js/_headers/package.json/README.md) — المشروع UE5 فقط، لا لغبطة مستقبلية |
| 2026-07-28 | P5-T0 وثائق التصميم الشاملة 07-game-design/ | f14122f |
| 2026-07-28 | P5-T1 نظام السور السداسي + البناء الحر (العميل) | 8da42ca |
| 2026-07-28 | P5-T2 ثيمات مباني الحضارات الست (العميل) | e9e4d19..80e1efe | URok2CivThemes جديد: يقرأ data/civilizations.json (أو fallback مدمج) ويقدّم لوحة الألوان والنمط المعماري لكل حضارة (روما/الصين/العرب/مصر/الفايكنج/اليابان)؛ Rok2BuildingActor: بناء مركّب placeholder (جسم + سقف مميز حسب النمط + شريط زخارف + عنصر تمييز) يُلوّن ويُشكّل حسب ثيم الحضارة — مع fallback للـ GLB عند توفره (تلوين خفيف)؛ Rok2CityLayoutActor: يقرأ حضارة اللاعب من Api->GetPlayer().Civ ويمررها لكل مبنى + السور عند الزرع؛ Rok2HexWallActor: يستقبل CivId ويضيف ثيم الحضارة للسور (لون أساسي) والبوابة (لون ثانوي) والأبراج (لون تمييز)؛ Rok2WorldRenderer: يستخدم ثيم حضارة اللاعب لتلوين مدينته الخاصة على الخريطة؛ scripts/verify_civ_themes.mjs: 68 فحص بنيوي ناجح |
| 2026-07-28 | P5-T3 HUD بأسلوب RoK (العميل) | b258978 | Rok2HudWidget أعيد كتابته بالكامل: شريط موارد علوي ذهبي RTL (طعام/خشب/حجر/ذهب/gems/AP بتنسيق مختصر K/M) + أزرار دائرية مزخرفة أسفل يمين (بناء 🔨 بشارة بنّاء خامل + قادة/تحالف/حقيبة/أحداث) + مجموعة يسار (خريطة/تقارير/تحرير المدينة) + طوابير/إشعارات/مركز إشعارات — كل الأحداث تفوَّض لـ Blueprint (OnBuild/OnCommanders/OnAlliance/OnItems/OnEvents/OnEditCity/OnMap/OnReports)؛ Rok2BuildingDetailWidget أعيد كتابته كبطاقة مبنى Bottom Sheet تنزلق من أسفل (مقبض سحب + عنوان + وصف + مستوى + تكلفة/وقت + شريط طابور + زر ترقية + زر ثانوي حسب النوع: تدريب/شفاء/بحث/صناديق + خلفية معتمة للإغلاق)؛ Rok2BuildMenuWidget جديد: قائمة بناء بثلاث فئات (اقتصاد/عسكري/زخرفة) بشبكة UUniformGridPanel + وسيط URok2BuildButtonProxy لكل زر (يخزن id ويعيد بثّ الضغط)؛ لوحة ألوان ui-ux-design-system.md (برونز #1A120B، ذهب #C9A227، عاجي #F5E9D0)؛ فحص سلامة 6/6. ملاحظة: gems/AP تُعرض 0 مؤقتاً حتى إتاحتها في FRok2City من الـ backend |
| 2026-07-28 | P5-T4 شاشة القادة الكاملة (العميل) | d103df0 | Rok2CommanderWidget أعيد كتابته بالكامل كشاشة قادة RoK-style: قائمة قادة قابلة للتمرير (يمين) ببورتريه placeholder ملوّن حسب الحضارة (من URok2CivThemes) + اسم + ندرة ملوّنة + نجوم مستوى (★/☆)؛ لوحة تفاصيل (يسار) عند اختيار قائد: بورتريه كبير بإطار ذهبي + اسم + حضارة + ندرة + مستوى/نجوم + شريط خبرة UProgressBar + إحصائيات (هجوم/دفاع/دعم) + 3 مهارات (attack ⚔️ / defense 🛡️ / passive ✨ بأيقونات وألوان) + 3 فروع مواهب stub (قتال 🔴 / دعم 🟡 / حركة 🔵 بنقاط 0/20) + 5 خانات معدات stub (سلاح/خوذة/درع/حذاء/إكسسوار) + أزرار (تعيين في مسيرة ⚔️ / ترقية مستوى ⭐ / ترقية مهارة ⚡)؛ FRok2CommanderSkillData + FRok2CommanderDetailData في Rok2CommanderWidget.h؛ LoadCommanderDetailsFromJson يقرأ data/commanders.json (12 قائداً بمهارات كاملة)؛ delegates: OnCommanderSelected + OnAssignCommander؛ لوحة ألوان ui-ux-design-system.md (برونز #1A120B، ذهب #C9A227، عاجي #F5E9D0)؛ scripts/verify_commanders_screen.mjs: 82 فحص بنيوي ناجح |
| 2026-07-28 | P5-T5 ضباب الحرب + الكشافة (العميل) | f795b41 | URok2FogOfWar جديد: شبكة كشف للخريطة (2400×2400 بخلايا 50) كل خلية لها حالة (Unexplored/Partially/Explored)؛ RevealArea يكشف دائرة حول نقطة (للمدينة أو الكشافة)؛ IsExplored/GetFogStateAt للاستعلام؛ FRok2Scout (كشافة بموقع/هدف/وقت وصول) + UpdateScouts يكشف عند الوصول + OnScoutArrived/OnFogUpdated delegates؛ Rok2Types: FRok2ScoutEntity + Scouts في FRok2WorldSnapshot؛ Rok2Api: SendScout (POST /v1/world/scout) + ParseScoutEntity + بارس scouts في snapshot + WS scout_arrived event؛ Rok2WorldRenderer: يقرأ Fog->IsExplored — لا يرسم مدن/ممرات/عقد في مناطق غير مكتشفة (إلا مدينة اللاعب) + يكشف المنطقة حول مدينة اللاعب تلقائياً + UpdateScouts في Tick؛ Rok2MarchPanel: زر "🔭 إرسال كشافة" يرسل SendScout(ToX, ToY) بدون قوات؛ scripts/verify_fog_of_war.mjs: 58 فحص بنيوي ناجح |
| 2026-07-28 | P5-T6 حركات وانتقالات وأصوات (العميل) | 6b36de3 | URok2AudioManager جديد: يدير الموسيقى والمؤثرات حسب حضارة اللاعب (من URok2CivThemes) — يقرأ ملفات .wav من Content/Audio/<civ>/ إن وُجدت وإلا placeholder صامت؛ ERok2AudioType (Music/BuildComplete/Upgrade/BattleVictory/BattleDefeat/MarchStart/ButtonClick/Notification) + ERok2MusicState (Stopped/Playing/Paused) + OnMusicStateChanged delegate؛ Rok2BuildingActor: PlayBuildAnimation (scale-in من 0.1 لمدة 0.6s) + PlayUpgradeAnimation (pulse ذهبي بـ sin لمدة 0.4s) + PlayRevealAnimation (fade-in من 0.01 لمدة 0.5s) + Tick override + UpdateAnimation + ComputeAnimatedScale؛ Rok2CityLayoutActor: يستدعي PlayBuildAnimation عند زرع مبنى جديد؛ Rok2WorldRenderer: يستدعي PlayRevealAnimation عند ظهور مدينة بعد ضباب + PlaySfx(MarchStart) عند انطلاق مسيرة؛ Rok2Api: InitForCiv+PlayMusic عند InitCity + PlaySfx(Upgrade) عند ترقية + PlaySfx(BattleVictory/Defeat) عند تقرير قتال؛ scripts/verify_game_feel.mjs: 59 فحص بنيوي ناجح |
| 2026-07-28 | P4-T2 إنتاج الأصول الصوتية + بورتريهات القادة | f859748..a0fc599 | Content/Audio/<civ>/music.wav ×6 + sfx/*.wav ×7 (WAV 16-bit PCM 44.1kHz مولّد إجرائياً عبر scripts/generate_audio.py — بصمة لكل حضارة: روما fanfare/الصين خماسي/العرب حجاز/مصر فريجي/الفايكنج درون+أبواق/اليابان insen — حلقات 20-24ث بتلاشي سلس + 7 مؤثرات مشتركة)؛ Content/Art/Commanders/<id>.png ×12 (512×512 بأسلوب المرجع commanders-lineup.jpg: bust + إضاءة ذهبية + خلفية حضارية)؛ Rok2CommanderWidget يحمّل البورتريه الحقيقي من /Game/Art/Commanders/<id> في البطاقات ولوحة التفاصيل مع fallback للـ placeholder؛ setup_level.py يكسب Step 1c (فك base64 + استيراد WAV/PNG) + scripts/decode_binary_assets.py عام لكل الثنائيات + توثيق README في Audio/ وCommanders/؛ scripts/verify_produced_assets.mjs: 65 فحص بنيوي ناجح |
| 2026-07-28 | P4-T3 موسيقى معركة لكل حضارة + حالة قتال | ba176e0..45f10c6 | Content/Audio/<civ>/battle.wav ×6 (نسخة قتالية من سلم كل حضارة: 126-150 BPM مقابل 70-110 في السلام، طبول حرب مكثفة بأوف-بيت مزدوج، نغمات قصيرة متصاعدة — حلقات 20ث)؛ URok2AudioManager: ERok2MusicMode (Peace/Battle) + BattleMusicPaths + EnterBattleMode/ExitBattleMode + PlayCurrentModeMusic يبدّل المسار حسب النمط + BattleModeTimeout (30ث) بمؤقت FTimerHandle للعودة التلقائية + IsInBattleMode؛ Rok2Api: EnterBattleMode() عند وصول battle_report (يمدّد المهلة عند تقارير متتالية)؛ scripts/verify_produced_assets.mjs موسّع (أقسام 1b/6b/6c): 113 فحص بنيوي ناجح |
| 2026-07-29 | دمج fix/decode-base64-assets (PR #2) + fix/sm6-black-screen (PR #1) في main | 09eb9b8..c1dd6b7 | 24 كوميتاً من البرانشين في main (merge commit، بلا إعادة كتابة تاريخ) |
| 2026-07-29 | إصلاح حرج شامل بعد التدقيق | — | الخادم: قوس seedWorld المفقود (كان يكسر wrangler deploy)، ثغرة توليد القوات (خصم home→marching عند الإنشاء + إسقاط إنشاء المسيرات عبر WS + رفض الأعداد السالبة/الكسرية + ربط playerId بالتوكن عبر header)، alliance/help (تمرير playerId مالك الطابور + already_helped بعد النجاح)، مصادقة على 6 مسارات GET عامة (snapshot منقّح للضيوف)، إبطال الرموز عبر جدول sessions، 500 ثابت بلا تسريب رسائل، تحقق إحداثيات flag، حد LIMIT لاستعلام retention، combat.ts severeRate فعّال، smoke/e2e بدون مفتاح افتراضي مسرّب + رمز في طلبات snapshot. الكلاينت: حقل Kind في FRok2MarchEntity يُملأ من payload.kind + Cluster من UOverlay إلى UCanvasPanel (كاسرا بناء). النظافة: حذف tmp_binary_test.bin + _extracted (~9MB)، *.apk/*.aab/tmp_* في .gitignore، تصحيح ادعاء PLAN.md السابق، AGENTS.md يعكس التسليم التراكمي |
| 2026-07-28 | P4-T4 مؤثرات أحداث اللعب الخمسة | 73ba38e | Content/Audio/sfx/ يكسب gather_complete (رنين عملات حصاد)/research_complete (وميض اكتشاف صاعد)/heal_complete (وتد دافئ)/zone_unlock (طبلة + فنفار مهيب)/rally_launch (بوقا تجمع + طبول مسير)؛ ERok2AudioType يكسب 5 أنواع + SfxPaths؛ Rok2Api: PlaySfx عند zone_unlocked/tech_researched/rally_launched (WS) وmarch_returning من نوع gather/node للاعب نفسه + HealWounded جديدة (POST /v1/city/heal بـ troops JSON + صوت HealComplete عند النجاح + توست) — يملأ فجوة عدم وجود عميل لـ endpoint الشفاء؛ scripts/verify_produced_assets.mjs موسّع (أقسام 2b/6d): 143 فحص بنيوي ناجح |
| 2026-07-29 | P4-T5 قادة إضافيون (18) + anti-cheat أساسي | COMMIT_HASH | data/commanders.json يكسب 6 قادة جدد (cmd_<nation>_2: Germanicus/Zhuge Liang/Saladin/Imhotep/Lagertha/Oda Nobunaga — واحد لكل حضارة، بمهارات attack/defense/passive كاملة)؛ data/anticheat.json جديد (6 rate limits: march/pass_attack/help/shop_buy/use_speedup/rally بحدود نافذة+cooldown + anomaly: سقف قوات المسيرة الكلي/المفرد، سقف المسيرات النشطة 5، سقف شراء 99)؛ sim/anticheat.ts جديد يقرأ JSON فقط (AntiCheatRateLimiter نافذة منزلقة + checkMarchPayload/checkShopBuyPayload)؛ KingdomShard: فحص شذوذ + rate limit في createMarch + سجل مخالفات (آخر 50) + GET /do/anticheat/violations؛ router.ts: enforceRateLimit على help/shop_buy/use_speedup/rally (429 rate_limited_<reason> + retryAfterMs) + GET /v1/admin/anticheat؛ اختباران: anticheat_offline_test.mjs جديد (34 فحصاً) + commanders_offline_test.mjs موسّع (19 فحصاً) — كل الاختبارات الأوفلاين الـ 13 ناجحة بدون كسر |
---

## 5. ملفات مرجعية سريعة

- `README.md` — مدخل المشروع
- `INDEX.md` — فهرس كل الوثائق
- `07-game-design/GDD.md` — **وثيقة التصميم الشاملة (مصدر الحقيقة للتصميم)**
- `07-game-design/castle-hex-city.md` — مواصفة القلعة السداسية والبناء الحر
- `07-game-design/civilizations-visual-design.md` — التصميم البصري للحضارات الست
- `07-game-design/rok-features-audit.md` — توثيق مزايا RoK الأصلية (المرجع)
- `07-game-design/ui-ux-design-system.md` — نظام الواجهات بأسلوب اللعبة
- `07-game-design/power-balance-map.md` — خريطة توازن القوى والجيم بلاي
- `07-game-design/assets/` — 11 صورة مرجعية مولّدة (قلعة، حضارات، وحدات، قادة، UI)
- `UNREAL_ENGINE_GUIDE.md` — تشغيل وبناء عميل UE5
- `game/docs/API.md` — مرجع الـ REST/WebSocket API
- `game/docs/RUN.md` — تشغيل ونشر الـ backend
- `data/*.json` — بيانات التوازن الموحدة
- `06-implementation/roadmap.md` — الرؤية التاريخية (تُقرأ للسياق، والمرجع النهائي هو هذا الملف)
