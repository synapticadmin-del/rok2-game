# Zone 3 — منطقة التصفية النهائية (Finals Core)

## الدور
Zone 3 = **منطقة واحدة** في قلب الخريطة.  
هنا تتحسم المراكز، وتتولد لحظة الموسم (Highlights / Screenshots / Drama).

ليست للسكن الدائم العشوائي؛ هي **ساحة تصفية**.

---

## الشكل

```
              [Final Gates from Zone2]
                        |
        ################################
        #                              #
        #     Outer Ring Objectives    #
        #         (Altars/Forts)       #
        #                              #
        #        +--------------+      #
        #        |   CORE /     |      #
        #        |   THRONE /   |      #
        #        | LOST TEMPLE  |      #
        #        +--------------+      #
        #                              #
        ################################
```

- **مساحة واحدة** (Zone3)
- اختيارياً مقسمة لـ **3 حلقات داخلية** (Outer / Mid / Core) بدون ما نسميها Zones جديدة
- الدخول فقط من **Final Gates**

---

## الهدف المركزي (Core Objective)

سمّه حسب ثيمك:
- Throne of Kings
- Lost Temple
- Crystal Heart
- Sun Spire
- عرش الممالك / هيكل النور ...

### قواعد السيطرة المقترحة
1. يفتح في نافذة زمنية معلنة (مثلاً آخر 3–5 أيام من الموسم أو weekends الحسم)
2. يحتاج احتلال + **Hold duration** (مثلاً تراكم نقاط سيطرة كل دقيقة)
3. Heavily garrisoned NPCs في الفتح الأول
4. Wounded rules أقسى (نسبة dead أعلى) لرفع التكلفة
5. ملكية الـ Core تعطي:
   - أكبر باف في الموسم
   - أكبر نقاط ترتيب
   - ألقاب/ملك/مكافآت تحالف

---

## الأهداف الثانوية داخل Zone3

حتى لو تحالف ماسكش العرش، يقدر ينافس:

| الهدف | الوظيفة |
|-------|---------|
| Outer Forts | نقاط + staging |
| Crystal Nodes | موارد موسمية خاصة |
| Banner Sockets | تفعيل بافات مؤقتة |
| Side Altars | باف هجومي/دفاعي أثناء الحسم |
| Supply Depots | تقليل كلفة heal/gather داخل Z3 (اختياري) |

هذا يمنع "全滅 أو لا شيء".

---

## قواعد الدخول والخروج

1. **Gate control:** عبر Zone2 Final Gates
2. **Teleport:** 
   - إما ممنوع لداخل Z3 إلا من نقاط محددة
   - أو مسموح بتكلفة موسمية عالية جدًا
3. **City relocation:** غالبًا يُسمح بنقل محدود قرب Territory داخل Z3 في وقت الحسم (كما في حروب RoK الموسمية)
4. **Safe zones:** لا يُنصح بوجود safe كامل داخل Z3 (تكسر التصفية). ممكن فقط attacker prep zone في أحداث خاصة.

---

## نظام التصفية (Finals Scoring)

### ترتيب الممالك / التحالفات في نهاية الموسم
نقاط من:
- زمن السيطرة على Core
- زمن السيطرة على Outer objectives
- Kill points داخل Z3 (بمعامل أعلى)
- First occupy bonuses
- Mission chapters الموسمية

### مخرجات التصفية
- Rank 1 / 2 / 3 rewards
- ألقاب موسمية
- عملة موسم / commanders sculptures
- Framing للموسم التالي (legacy)

---

## زمنيات مقترحة (مثال موسم 6–8 أسابيع)

| المرحلة | Zone3 |
|---------|-------|
| معظم الموسم | مغلقة أو visible لكن مقفلة |
| قبل الأخير بأسبوع | فتح Outer ring فقط |
| آخر 72–96 ساعة | فتح Core windows |
| آخر 24 ساعة | تسارع النقاط / sudden death partial |

---

## قراءة بصرية مهمة جدًا

Zone3 لازم تبان من الزوم الأبعد:
- لون مختلف / إضاءة / عاصفة / عمود نور
- أيقونة Core ثابتة
- ملكية واضحة (شعار التحالف/المملكة)
- Timer كبير على الشاشة وقت الفتح

---

## أخطاء يجب تجنبها

1. فتح Zone3 بدري → الموسم يموت بدري
2. Core يتحسم بـ 1 rally وحيد بدون counterplay
3. عدم وجود أهداف ثانوية → 90% لاعبين متفرجين بلا دور
4. Teleport حر → تنهار أهمية الممرات
5. مساحة Z3 أكبر من اللازم → مسيرات مملة وفراغ

---

## Checklist تنفيذ Zone3

- [ ] Polygon/region واحد `zone_id=3`
- [ ] 1 Core object + state machine (closed/open/contested/owned)
- [ ] 4–8 outer objectives
- [ ] Final gates link from Zone2
- [ ] Scoring service hooks
- [ ] Severe combat rules config
- [ ] Spectator-friendly map markers
