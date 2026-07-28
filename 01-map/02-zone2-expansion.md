# Zone 2 — حلقة التوسع (Expansion Ring)

## الدور
Zone 2 هي **مصفاة القوة التنظيمية** قبل التصفية النهائية.  
مش للجميع بنفس السهولة: الدخول عبر ممرات Zone1→Zone2، والبقاء يحتاج Territory + مستشفيات + تحالف ناضج.

---

## الهيكل المقترح

**عدد المناطق داخل Zone2:** 4 (مُفضّل للوضوح) أو 6 (لو السيرفر ضخم).

### خيار A — 4 أقاليم (موصى به)
```
        Z2-N
   Z2-W      Z2-E
        Z2-S
           ↓
         Zone3
```
- كل إقليم Z2 يخدم جهتین من Zone1 (مثلاً Z2-N يستقبل R1/R2/R3)
- أقل تعقيد pathfinding/UI

### خيار B — 6 أقاليم
توزيع أقرب لسداسي حول Zone3، أنسب لو عندك 12+ ممالك في الحرب.

---

## ماذا يميز Zone2 عن Zone1؟

| البعد | Zone1 | Zone2 |
|------|-------|-------|
| موارد | Lv1–4 | Lv3–6 |
| Pass levels | 1–3 | 3–5 |
| Holy sites | Altars | Shrines / Sanctums أقوى |
| Barbarians | عادي | Forts / أقوى |
| Hospital pressure | متوسط | عالي |
| Reward density | نمو | حرب حقيقية |
| Building rules | مرن | قيود قرب Pass/Holy Site |

---

## المداخل من Zone1

- **Inner Passes** عند الحد الداخلي لكل Region في Z1
- لا تُفتح كلها يوم 1
- جدول فتح تدريجي (مثال):
  1. T+10 أيام: فتح 50% من ممرات Z1→Z2
  2. T+14 أيام: فتح الباقي
  3. أو فتح شرطي: تحالف يسيطر على 2 Border Passes محلية

### قواعد العبور
1. الممر لازم يكون محتل بتحالفك/ائتلافك
2. المسير عبر ممر معاد = غير مسموح (أو يمر بهجوم تلقائي — اختر واحدًا وكن حاسمًا)
3. Teleport لـ Z2 مقيد (غالي / يحتاج item موسمي / قرب Territory فقط)

---

## محتويات كل إقليم Zone2

1. **2–3 Shrines/Sanctums** بافات أقوى من Z1
2. **Resource belt** عالي المستوى
3. **1 Fortress objective** (اختياري) يعطي نقاط موسم
4. **Staging grounds** قرب مداخل Zone3
5. **Danger modifiers**: ليل/عواصف/ cooldowns أحداث

---

## اقتصاد الحرب في Zone2

- سرعة القتل أعلى → استهلاك speedups/heal أعلى
- نقاط الموسم من:
  - احتلال Shrines
  - قتل وحدات
  - السيطرة الزمنية على Passes
  - أهداف ثانوية (Outposts)

### Anti-snowball
- بافات الإقليم تتناقص لو السيطرة بدون contest لفترة طويلة؟ (اختياري)
- مكافآت underdog محدودة
- فتح Zone3 مرتبط بوقت أكثر من “كنس كامل Z2” عشان الضعيف يفضل له دور

---

## شروط التأهيل نحو Zone3 (أمثلة)

اختر واحد أو امزج:

1. **Timer:** بعد يوم 21 يفتح ممر/بوابات Zone3
2. **Score gate:** أعلى N تحالف/ممالك بالنقاط تدخل قلب التصفية
3. **Key objects:** احتلال X من Sanctums في Z2 يفتح بوابة
4. **Hybrid (موصى به):** Timer أساسي + باف لمن سيطر على Z2

---

## تصميم الممرات داخل Zone2

- بين أقاليم Z2: Pass Lv4–5
- نحو Zone3: **Final Gates** (1–4 بوابات) بمستوى أعلى
- البوابات لها:
  - Garrison cap عالي
  - Open windows (أوقات فتح)
  - First-occupy points ضخمة

---

## تجربة اللاعب المستهدفة

- اللاعب المتوسط: يشارك Rally، يحمي gatherers، ينقل قرب Territory
- الضابط/R4-R5: يخطط خطوط الإمداد والممرات
- الـ whale: يفرق في المعارك الكبرى لكن لا يلغي حاجة التحالف
- F2P المنظم: يساهم في garrison/flags/scout

---

## Checklist تنفيذ Zone2

- [ ] 4 Regions data
- [ ] Inner pass unlock schedule
- [ ] Shrine rules (open every N days, hold time)
- [ ] Season score hooks
- [ ] Teleport restrictions
- [ ] Final gate objects to Zone3
