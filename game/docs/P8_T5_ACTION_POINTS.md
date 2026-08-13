# P8-T5: نقاط العمل (AP) والدروع والتهجير

## الهدف

إضافة عمق اقتصادي جديد لمنظومة القتال والخريطة: رصيد **نقاط عمل (Action Points)** عالمي لكل مدينة يُستهلَك عند إطلاق أي مسيرة هجومية (برابرة / موقع مقدس / مدينة)، **درع الحماية (Peace Shield)** القابل للشراء بالجواهر، **حمى الحرب (War Frenzy)** التي تمنع المدافعين المهاجَمين من تفعيل الدرع ساعة كاملة، و**التهجير (Relocation)** العشوائي والموجَّه بـ cooldown.

## الملفات

| الملف | الدور |
|---|---|
| `game/backend/src/data/action_points.json` | مصدر الحقيقة للثوابت: السقف، التجديد، تكاليف المسيرات، خيارات الدرع، حمى الحرب، التهجير |
| `game/backend/src/do/sim/action_points.ts` | المنطق النقّي: `regenAp`، `apCost`، `shieldOptions`، `canActivateShield`، `warFrenzyDurationMs`، `relocationCooldowns` |
| `game/backend/src/do/KingdomShard.ts` | تنفيذ الشارد: حقول `CityEntity` الجديدة، migration `ver < 11`، tick التجديد/الانتهاء، `deductApFromCity`، endpoints داخلية `relocate` / `activate-shield` / `ap-state`، حمى الحرب في `settleAttackerCombat`، رفض المدينة المحمية في `createMarch` و`redirectMarch` |
| `game/backend/src/http/router.ts` | endpoints العمومية: `GET /v1/ap/state`، `POST /v1/shield/activate`، `POST /v1/city/relocate` مع نمط خصم D1 ثم تنسيق الشارد واسترجاع الفشل |
| `game/backend/scripts/action_points_offline_test.mjs` | حارس جودة نقّي (بيانات + صيغ + أسلاك مصدرية) |
| `game/backend/src/data/anticheat.json` | rate limits جديدة: `shield_activate`، `city_relocate` |

## القواعد الأساسية

1. **الرصيد:** سقف 1000 نقطة، تجديد نقطة واحدة كل 45 ثانية، يبدأ عند 1000 (sandbox). التجديد idempotent ولا يتجاوز السقف.
2. **التكاليف (من JSON):** برابرة = 5، موقع مقدس = 15، مدينة = 5. الرالي = 10. أي مسيرة أخرى 0.
3. **الدرع:** ثلاثة خيارات (8h/100 gems، 24h/250 gems، 3d/500 gems). لا يُفعَّل إن كان درع نشطًا أو حمى حرب جارية. يُستهلَك عند أي هجوم على المدينة.
4. **حمى الحرب:** عند أي هجوم ناجح على مدينة، يُمنَع الدافع من تفعيل الدرع لمدة ساعة (`war_frenzy.duration_ms`). التهجير الموجه ممنوع أيضًا خلالها.
5. **التهجير:** cooldown 30 يومًا. عشوائي يكلف 50 gems + 50 AP (يُنقَل لموقع جديد في Zone 1). موجه يكلف 200 gems + 200 AP (يجب أن يكون الهدف داخل منطقة Zone 1 مفتوحة).
6. **الاستهداف:** مدينة محمية بدرع نشط تُرفض كهدف مسيرة أو إعادة توجيه (`target_city_shielded`).
7. **الخصم:** يُجرَّد التجديد اللحظي أولًا ثم يُخصَم الرصيد؛ إن لم يكفِ تُرفض المسيرة (`not_enough_ap`).

## التحقق الآلي

- حارس `scripts/action_points_offline_test.mjs` (ALL PASSED) — بنية JSON + صيغ التجديد + أسلاك `KingdomShard`/`router` مصدرية.
- job جديد `test:p8-t5-action-points` داخل `npm run check`.
- TypeScript نظيف (`tsc --noEmit`).

## تحفظات

- AP/الدرع/حمى الحرب/التهجير تُخزَّن في Durable Objects (`map_cities`، migration `ver < 11`) وليس D1؛ يُقرأ الرصيد عبر `GET /v1/ap/state` من الشارد مع تجديد لحظي قبل العرض.
- حارس الشارد ضد التهجير الموجه أثناء حمى الحرب موجود في `/do/relocate` إضافة إلى router.
- rollback كامل عند فشل الشارد: gems تُسترد، وإحداثيات اللاعب في D1 تعود لموضعها.
