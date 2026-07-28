# الممرات والجبال (Passes & Mountains)

## 1) الجبال = هندسة التوجيه

الجبال ليست ديكور؛ هي **Graph constraints**.

```
RegionA ──Pass── RegionB
   │                │
 Mountain        Mountain
   │                │
 (no path)       (no path)
```

### خصائص بلاطة جبل
- `walkable = false`
- `buildable = false`
- `vision_block = optional`
- تظهر في minimap كلون ثابت

### سلاسل الجبال الموصى بها
1. **Radial walls**: تفصل 8 مناطق Zone1 عن بعض
2. **Ring wall**: يفصل Zone1 عن Zone2 (فيه Inner Passes فقط)
3. **Core wall**: يفصل Zone2 عن Zone3 (Final Gates)

---

## 2) الممرات (Passes) — قلب الـ PvP الاستراتيجي

### مستويات الممرات (مستوحى من أنظمة Pass Lv في RoK)

| Level | أين يظهر | صعوبة الاحتلال | أهمية |
|------:|----------|----------------|-------|
| 1–2 | حدود Zone1 الداخلية بين الأقاليم | منخفضة–متوسطة | توسع محلي |
| 3 | Z1↔Z1 الرئيسية / مداخل أقوى | متوسطة | سيطرة إقليم |
| 4–5 | Z1→Z2 وداخل Z2 | عالية | حرب توسع |
| 6–8 | Z2→Z3 / محيط التصفية | عالية جدًا | بوابة الحسم |
| 9–10 | نادرة/موسمية | extreme | أهداف فصل |

> في ملفات اللعبة الأصلية ظهرت نصوص: `Lvl {p1} Pass` حتى مستويات عالية، و`Derelict Pass`، وقيود teleport قرب الممر.

### قواعد عامة ( Prototype-ready )

1. **لا عبور** إن لم يكن الممر ملك تحالفك/ائتلافك.
2. **أول احتلال** يعطي نقاط + إعلان خريطة.
3. **Garrison cap** يزيد مع Level.
4. **Open schedule** (اختياري): بعض الممرات تُفتح في أيام محددة.
5. **Border requirement** للمستويات العالية: لازم Territory يلامس الممر قبل الهجوم.
6. **No city teleport** داخل radius الحماية.
7. **Derelict passes**: مغلقة للأيفنت/القصة (غير قابلة للهجوم).

---

## 3) حالات الممر (State Machine)

```
Locked
  → Sealed (ظاهر لكن غير قابل للهجوم حتى الوقت)
  → Contestable
  → Contested (معركة جارية)
  → Occupied(alliance_id)
  → Reinforced (اختياري)
  → Lost / Neutralized
```

### بيانات الممر
```json
{
  "pass_id": "Z1_R1_R2_P1",
  "level": 2,
  "from_region": "R1",
  "to_region": "R2",
  "zone_link": [1, 1],
  "position": {"x": 1204, "y": 880},
  "garrison_cap": 500000,
  "hold_seconds_to_capture": 0,
  "attack_requirements": {
    "alliance_territory_adjacent": false,
    "min_city_hall": 16
  },
  "buff_on_control": [],
  "teleport_block_radius": 10
}
```

---

## 4) توزيع مقترح للممرات

### Zone1 (8 Regions)
- بين كل جارين: **1 Primary Pass**
- اختياري: Secondary Pass أضعف يفتح لاحقًا
- **Inner Passes** من كل Region (أو كل جهتین) إلى Zone2: 4 أو 8

**حساب سريع:**
- Primary border passes ≈ 8
- Secondary ≈ 0–8
- Inner to Z2 ≈ 4–8
- **إجمالي Z1-related ≈ 12–24 ممر**

### Zone2
- 4 Regions → 4 border passes بينها
- 2–4 Final Gates إلى Zone3

### Zone3
- لا “regions” منفصلة؛ بوابات دخول فقط + أهداف داخلية

---

## 5) Pathfinding

1. ابنِ **navgraph** على البلاطات walkable
2. الممر = edge cost عادي إذا `can_traverse(alliance)`
3. إذا لا يملك العبور: edge inf / غير موجود
4. March يرسم polyline؛ العميل يعرض فقط، السيرفر يحسب ETA

### تكلفة إضافية اختيارية
- عبور قرب عدو: danger tax (مش لازم في MVP)
- طقس Zone2/3: march speed modifier

---

## 6) قراءة اللاعب / UX

- لون الممر حسب المالك
- شارة Level
- قفل 🔒 لو Locked/Sealed
- عند الضغط: Occupant / Garrison / Open time / Attack CTA
- في Strategic Zoom: خطوط تربط الأقاليم عبر الممرات المملوكة فقط

---

## 7) استغلالات يجب إغلاقها

| استغلال | الحل |
|---------|------|
| Teleport خلف الممر | block radius + zone rules |
| احتلال Pass بجيش رمزي | min power/time-to-capture أو NPC guard |
| إغلاق خريطة كاملة بتحالف واحد بدري | open schedule + multiple routes |
| نسيان الدفاع | decay ownership إن لم يُربط بـ Territory؟ (اختياري متقدم) |

---

## 8) ارتباط بالتحالف

- Pass ownership = Alliance (أو Coalition في موسم)
- يمدد إمكانية مد Flags عبر الجهة الأخرى
- Rally على Pass هدف يومي للـ leadership
