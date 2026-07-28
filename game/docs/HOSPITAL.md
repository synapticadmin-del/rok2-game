# نظام المستشفى (Hospital) — P2-T2

> المصدر الموحد للثوابت: `data/buildings.json` (مفتاح `hospital`).
> الخادم هو السلطة: استقبال الجرحى وخصم الخسائر والشفاء كلها تُحسب على السيرفر.

## الثوابت (`data/buildings.json → hospital`)

| المفتاح | القيمة | المعنى |
|---------|--------|--------|
| `base_capacity` | 200 | سعة المستشفى عند المستوى 1 |
| `capacity_per_level` | 150 | زيادة السعة لكل مستوى إضافي |
| `heal_cost_factor` | 0.5 | تكلفة الشفاء = نصف تكلفة التدريب لكل وحدة |
| `heal_seconds_per_troop` | 5 | ثواني الشفاء لكل جندي |

السعة: `capacity = base_capacity + capacity_per_level × (level − 1)` — مستشفى L1 يستوعب 200، L5 يستوعب 800.

## دورة حياة الجريح

1. **المعركة:** `resolveCombat` يقسّم خسائر المهاجم إلى slightly / severely / dead (حسب نسب الـ zone).
2. **الاستقبال (عند حلول المعركة في `KingdomShard`):** الجرحى الخطيرون (`attackerSplit.severely`) يُقبَصون بالسعة المتاحة:
   - **المقبولون** → يُسجلون في D1 كـ `troops.status = 'severely_wounded'` ويُخصمون من رصيد `marching`.
   - **الفائض فوق السعة** → يموت (لا يعود للمدينة).
   - كل الخسائر (موتى المعركة + الجرحى) تُخصم من رصيد `marching` فوراً؛ الناجون يعودون عند `resolveMarchReturn`.
   - ملخص الاستقبال (`hospital: { admitted, died, capacity }`) يُضمَّن في تقرير المعركة.
3. **الشفاء:** `POST /v1/city/heal { troops }`:
   - يتحقق من وجود الجرحى فعلاً + كفاية الموارد (نصف تكلفة التدريب — تُخصم فوراً).
   - ينقل الجرحى من `severely_wounded` إلى طابور `heal` في الـ Durable Object.
   - عند اكتمال الطابور في الـ tick → يعودون `status = 'home'`.

## القراءة من الـ API

`GET /v1/city` يعيد الآن:

```json
{
  "wounded": { "infantry_t1": 45 },
  "hospital": { "level": 1, "capacity": 200, "used": 45, "free": 155 }
}
```

## الاختبار

```bash
# رياضيات النظام بدون سيرفر (11 فحصاً)
node scripts/hospital_offline_test.mjs

# E2E ضد الإنتاج
BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/hospital_test.mjs
```

يغطي الـ E2E: كتلة المستشفى في `/v1/city`، تسجيل الجرحى بعد معركة ممر مع قب السعة، ملخص المستشفى في التقرير، رفض الشفاء الزائد، الشفاء بالتكلفة، وعودة القوات المشفية للمدينة.
