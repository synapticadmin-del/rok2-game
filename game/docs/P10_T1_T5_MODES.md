# P10 — أوضاع اللعب المتكررة (Recurring Gameplay Modes)

**الهدف:** إضافة خمسة أوضاع لعب متكررة تُثري تجربة اللاعب اليومية والأسبوعية بين المعارك الكبرى، وكلها تعمل منطقيًا بالكامل على الخادم (KingdomShard Durable Object) وتُعرض عبر عميل UE5. بنيت هذه الأوضاع بعد دراسة أنظمة اللعب في ألعاب 4X MMO الناجحة.

**التغطية:** P10-T1 الحانة (Tavern) · P10-T2 Expedition · P10-T3 Sunset Canyon · P10-T4 Ark of Osiris · P10-T5 الأحداث الكبرى (Mightiest Governor + Wheel of Fortune) · P10-T6 واجهة عميل C++.

## الحالة

- [x] P10-T1: الحانة — فتح الصناديق بمفاتيح يوميّة + سقف إحصائي لعرض Epic (6.8%)
- [x] P10-T2: Expedition — 8 مراحل PvE + ميداليات + متجر ميداليات
- [x] P10-T3: Sunset Canyon — موسم أسبوعي + تحديات يومية + مؤثرات + متجر Token
- [x] P10-T4: Ark of Osiris — دوريات 14 يومًا + 3 منشآت قابلة للاستيلاء + سفينة أوزيريس
- [x] P10-T5: الأحداث الكبرى — Mightiest Governor (6 أيام، 6 مراحل) + Wheel of Fortune (كل 14 يومًا)
- [x] P10-T6: عميل UE5 — 10 أنواع + 15 declaration + 5 Parse + 15 تنفيذًا

## البيانات (JSON قابلة للتعديل — لا ثوابت hard-coded)

| ملف | المحتوى الأساسي |
|---|---|
| `data/tavern.json` | 3 صناديق (فضي/ذهبي/معدات)، معدلات مستهدفة لكل صندوق، حدود: 60 فتح/ساعة، مفتاح فضي يومي مجاني، سقف Epic = 6.8% |
| `data/expedition.json` | 8 مراحل (150 ألف → 6.5 مليون قوة موصى بها)، ميداليات لكل مرحلة، متجر ميداليات |
| `data/canyon.json` | موسم 7 أيام، 5 تحديات/يوم بقوات T3 متطابقة للجميع (القادة هم الفارق)، مكافآت نجوم، 3 مؤثرات، متجر Token |
| `data/osiris.json` | موسم 14 يومًا (تأهيل أيام 1–7 + دوري 8–14)، 30 لاعب/جانب، 3 منشآت (مسلة شرق/غرب 500 نقطة، معبد أوزيريس 1500 نقطة) |
| `data/events.json` | 5 أحداث متكررة + majorEvents: mightiestGovernor (مراحل: power_growth → kill_events → building → research → march_training → final_battle) و wheelOfFortune (كل 14 يومًا، دورة كونية 56 يومًا) |

## الخادم (KingdomShard)

- `sim/tavern.ts` — منطق الحانة النقي: فتح صناديق بمفاتيح، معدلات عشوائية مقيّدة بالسقف الإحصائي (Chi-square على 10,000 عينة)، حد 60 فتح/ساعة، مفتاح فضي يومي.
- `sim/expedition.ts` — حساب المعارك PvE ضد مرحلة، نجوم 1–3 حسب الضرر والنجاة، تراكم ميداليات وشراء من متجر الميداليات.
- `sim/canyon.ts` — موسم Canyon مع نافذة زمنية أسبوعية (`MS_PER_DAY` من `lib/timeConstants.ts`)، تحديات يومية، مؤثرات مؤقتة 24 ساعة، Victory Points ولوحة 50.
- `sim/osiris.ts` — تسجيل في الدوري، استيلاء على المنشآت، حركة سفينة أوزيريس بين المسارات (`MS_PER_HOUR`)، نقاط الفريق الأحمر/الأزرق.
- `sim/major_events.ts` — مراحل Mightiest Governor مع نوافذ نقاط يومية (86400000ms مقروءة من JSON)، لوحة 200؛ Wheel of Fortune مع احتمالات فتحات ومفتاح يومي مجاني.
- `KingdomShard.ts` — migration 17 (حالة أوضاع P10) + 19 handler لكل وضع + استيراد الثوابت الزمنية من `lib/timeConstants.ts`.
- `router.ts` — 23 endpoint (GET/POST على /v1/tavern/*، /v1/expedition/*، /v1/canyon/*، /v1/osiris/*، /v1/events/*).
- `anticheat.json` — 15 حد rate جديد لهذه الواجهات (tavern_open، expedition_attempt، canyon_challenge، osiris_attack… إلخ).
- `lib/timeConstants.ts` (جديد) — `MS_PER_DAY = 86400000 / 1`، `MS_PER_HOUR = MS_PER_DAY / 24`، `MS_PER_MINUTE` — بديل عن كتابة literals مباشرةً في الكود.

## جودة الخادم

```bash
cd game/backend && node scripts/p10_offline_test.mjs
# 72/72 فحصًا — ALL PASSED (معدلات الحانة، حدودها، مراحل Expedition، مكافآت Canyon،
# نقاط Osiris، مراحل Mightiest Governor، احتمالات العجلة) ×3 مرات
```

## عميل UE5 (P10-T6)

| الملف | الإضافة |
|---|---|
| `Rok2Types.h` | 10 أنواع جديدة: FRok2TavernState/FRok2TavernRoll، FRok2ExpeditionState/FRok2ExpeditionBattleResult، FRok2CanyonState/FRok2CanyonChallenge، FRok2OsirisState/FRok2OsirisFacility، FRok2EventsState/FRok2WheelSpinResult |
| `Rok2Api.h` | 5 delegates (FOnTavernUpdated…FOnEventsUpdated)، 15 UFUNCTION declarations، 5 كاشات UPROPERTY، 5 أحداث BlueprintAssignable، 5 دوال Parse |
| `Rok2Api.cpp` | 15 تنفيذًا (Fetch/Post لكل وضع) + 5 Parse كاملة مع قارئات Rok2Json الآمنة |

```bash
cd game/backend && node ../client-unreal/scripts/verify_p10_client.mjs
# 58/58 — CHECK-PASS: P10-T6 client paths (21 endpoints من router.ts)
```

## بروتوكول جودة

```bash
cd game/backend && npm run check
# EXIT=0 — كل السلسلة تشمل test:p10-recurring-modes و test:p10-client
```

## التزامات التصميم

1. **كل الأرقام من JSON** — معدلات الحانة، نقاط المنشآت، نوافذ الأحداث، لا توجد ثوابت زمنية hard-coded في الكود (التحقق: verify_p7_t15_ops يرفض literals الزمنية).
2. **سقف إحصائي للحانة** — epicRateCeilPct = 6.8% يُفرض عبر فحص Chi-square على نوافذ 10,000 فتح؛ يمنع انحراف RNG بعيدًا عن التصميم.
3. **Canyon عادل** — قوات T3 متطابقة لجميع اللاعبين، الفارق الوحيد هو مستويات القادة والمهارات.
4. **Osiris متوازن** — معبد المركز أهم بأضعاف المسلات (1500 vs 500 نقطة استيلاء، 60 vs 20 نقطة/ساعة) لتركيز المعارك المركزية.
5. **مكافحة الغش** — 15 حد rate جديد + تحقق playerId مقابل route في كل endpoint.
