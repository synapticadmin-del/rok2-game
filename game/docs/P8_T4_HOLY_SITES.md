# P8-T4: المواقع المقدسة ودورة المعبد المفقود (Lost Temple Cycle)

## الهدف

إضافة مواقع مقدسة قابلة للاحتلال بالتحالفات على خريطة المملكة — Sanctum (6 مواقع، حراس T1 × 10,000)، Altar (4 مواقع، حراس T2 × 15,000)، Shrine (موقعان، حراس T3 × 30,000) — بحماية احتفاظ 4 ساعات وباف وحيد لا يتراكب حسب الهرمية (temple ⟩ shrine ⟩ altar ⟩ sanctum). في قلب منطقة Zone 3 يُفتح دوريًا **المعبد المفقود** (الحراس مزيج من كل الدرجات حتى T5، 60,000 جندي) منذ اليوم 40: من يحتفظه 8 ساعات متواصلة يتوَّج تحالفه **ملك المملكة**، وتنتقل الملكية فور فقدان المعبد أو تحرره. 50% من جرحى القتال في المعبد يموتون فوق قاعدة المستشفى كعقوبة قتالية.

## الملفات

| الملف | الدور |
|-------|-------|
| `src/data/holy_sites.json` | البيانات الوحيدة: 12 موقعًا (6 sanctum/4 altar/2 shrine) + المعبد المفقود + الثوابت (hold 4h، دورة مناظعة 3 أيام، مكسب الاحتلال) + بافات الأنواع الأربعة |
| `src/do/sim/holy_sites.ts` | منطق نقّي جديد: `siteGuardTroops`/`templeGuardTroops`/`templeUnlocked`/`siteCaptureGain`/`templeWoundedDeadShare`/`holdDurationMs`/`holdForKingMs`/`bestHeldSiteBuff` |
| `src/do/KingdomShard.ts` | `HolySiteEntity`/`KingEntity` + migration ver<10 + `seedHolySites`/`persistHolySite`/`loadHolySites` + snapshot/worldDelta يعيدان `holySites` + `king`؛ منطق الاحتلال في التسوية (حامية + enhance 100 + باف الموقع + XP)؛ tick: انتهاء الحيازة يحرر الموقع + تتويج الملك بعد 8h + إزالة اللقب عند فقدان المعبد؛ `createMarch`/`redirectMarch` يقبلان `holy_site` مع zone check وcheck فتح المعبد (day 40 + contest نشط)؛ جرحى المعبد 50% موت |
| `src/http/router.ts` | `GET /v1/meta/holy-sites` + `meta/all` يعيد `holySites` للعميل + `holySiteId` في march/redirect body |
| `scripts/holy_sites_offline_test.mjs` | الحارس — عقد JSON + تركيب الحامية + المكسب + الهرمية + التوزيع + EXIT=0 |

## القواعد الأساسية

1. **البيانات لا hard-code:** الأنواع والحراس والثوابت من `holy_sites.json`؛ المنطق في `holy_sites.ts` قرءة نقية.
2. **المناظعة:** أي تحالف يرسل مسيرة `holy_site` بقوات؛ التقدم يُحسب من `siteCaptureGain(قوة القوات المتبقية بعد القتال)`، وعند اكتمال 100 يصبح الموقع مملوكًا بالتحالف (`holy_site_captured` story + enhance 100 للتحالف).
3. **الحيازة 4 ساعات:** `heldSinceMs` عند الظفر؛ انتهاءها يحرر الموقع تلقائيًا في tick (المعبد مستثنى — ملكيته دائمة ما دامت مضمونة).
4. **الباف:** أعلى نوع مملوك فقط يسري (لا تراكم)؛ باف المعبد يتخطى كل الأنواع.
5. **المعبد المفقود:** لا يُستهدف قبل اليوم 40 ولا خارج contest نشط؛ حارسه مزيج T1–T5 (60k)؛ قتاله أشد قسوة — 50% من الجرحى الخطرين يموتون فوق سقف المستشفى.
6. **الملك:** 8 ساعات احتفاظ مستمر بالمعبد = `king_crowned` story + `king` في snapshot؛ انتقال المعبد لتحالف آخر يُلغي لقب الملك الفوري.
7. **المسيرة:** targetType جديد `holy_site`، وwhite-list targetId في الراوتر؛ إعادة التوجيه إلى موقع مقدس مدعومة؛ أهداف المنطقة المقفلة تُرفض (`zone_locked`).

## التحقق الآلي

`npm run test:p8-t4-holy-sites` (EXIT=0) مدرج في `npm run check` بعد `test:p8-t3-troops`، وفي `scripts/run_offline_tests.mjs` ضمن `npm run test:offline`.

## تحفظات

- الواجهة الأمامية (UE) لمواقع الاحتلال على الخريطة + إشعارات التتويج تُبنى لاحقًا؛ كل السلوك الآن في الـ shard والبث (`holy_site_changed`) جاهز للعميل.
- دورة المناظعة (3 أيام) تُطبق منقولة مع contest العرش الحالي؛ دورة منفصلة مستقلة للمعبد يمكن إضافتها في P8-T8 بناءً على نتائج الاختبار الحي.

## مراجع

- وثائقا `P8_T1_TALENTS.md` و`P8_T2_TROOPS.md`؛ منطق `core_objective` القائم (إنهاء الاحتلال عند 100) نُسخت منه آلية الاحتلال.
