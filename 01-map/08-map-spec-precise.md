# Map Spec الدقيق — ROK2 (إحداثيات + بلاطات + 8 مناطق)

**الحالة:** مواصفة تصميم تنفيذية (ليست نسخة إحداثيات من سيرفر RoK)  
**الأساس:**  
1) طلبك: Zone1=8 مناطق + جبال، Zone2، Zone3 تصفية  
2) ما أكدته نصوص RoK: *eight starting provinces* + *King's Land* center + Pass level gates  

---

## 1) وحدات القياس

| البند | القيمة |
|-------|--------|
| نظام الإحداثيات | Cartesian 2D, origin = **أسفل-يسار** الخريطة `(0,0)` |
| المحور X | يمين |
| المحور Y | أعلى |
| **1 Tile** | وحدة عالم واحدة (1u) |
| حجم العالم | **2400 × 2400** tiles |
| مركز العالم | `(1200, 1200)` |
| أقصى إحداثي شامل | X,Y ∈ `[0, 2400)` |
| مقاس عرض مدينة اللاعب (footprint) | **2×2** tiles (مركز المدينة على بلاطة منطقية) |
| نصف قطر حماية مدينة (no-build/no-spawn) | **5** tiles |
| نصف قطر منع Teleport حول Pass | **12** tiles |
| ارتفاع كاميرا/LOD | خارج هذا المستند (بصري) |

### لماذا 2400؟
- يتسع لـ 8 أقاليم + حلقتين + قلب بدون ازدحام.
- MVP يمكن تشغيل **Scale 0.5** → عالم 1200×1200 بنفس النسب (كل الأرقام ×0.5).

**معامل التحويل:** `world = spec * scale` حيث `scale ∈ {1.0, 0.5}`

---

## 2) الطبقات (Layers)

1. **TerrainGrid** (2400×2400): `plain | mountain | water | forest_visual`
2. **RegionPolygonLayer**: 8 + 4 + 1
3. **PassGraph**: edges بين الأقاليم
4. **Holy/Objectives layer**
5. **Resource nodes layer** (spawn tables)
6. **Alliance territory overlay** (runtime)
7. **AOI grid**: خلايا **20×20** tiles (120×120 AOI cells)

---

## 3) تعريف الـ Zones كمساحات

نستخدم **3 حلقات مربعة** (أسهل للتنفيذ من الدائرة، مع jogs بصرية لاحقًا).

### Zone 3 — Finals Core (مربع المركز)
```
AABB: [1000,1000] → [1400,1400]
Size: 400 × 400
Center: (1200,1200)
```

### Zone 2 — Expansion Ring (إطار حول Z3)
```
Outer AABB: [700,700] → [1700,1700]   (1000×1000)
Inner hole: Zone3 AABB
Effective ring width: 300 tiles
```

### Zone 1 — Home Ring (إطار خارجي)
```
Outer AABB: [0,0] → [2400,2400]
Inner hole: Zone2 outer [700,700]–[1700,1700]
```

```
0                         2400
┌──────────────────────────┐
│         ZONE 1           │
│   ┌──────────────────┐   │
│   │     ZONE 2       │   │
│   │   ┌──────────┐   │   │
│   │   │  ZONE 3  │   │   │
│   │   │  400^2   │   │   │
│   │   └──────────┘   │   │
│   └──────────────────┘   │
└──────────────────────────┘
```

---

## 4) Zone 1 — الثماني مناطق (قطاعات)

Zone1 تُقسّم إلى **8 قطاعات** باتجاهات بوصلة، مفصولة **ممرات جبلية سمكها 40 tile** (mountain belts).

### 4.1 حدود القطاعات (Polar-octant على الحلقة)

نعرّف القطاع بزاوية من المركز، ثم نقطعها مع حلقة Zone1.

| ID | الاسم | زاوية المركز (deg) | اتجاه |
|----|-------|-------------------:|--------|
| R1 | Northern Marches | 112.5 | NW |
| R2 | Frostvale | 67.5 | N |
| R3 | Eagle Highlands | 22.5 | NE |
| R4 | Sunrise Plains | -22.5 / 337.5 | E |
| R5 | Amber Coast | -67.5 | SE |
| R6 | Southern Dunes | -112.5 | S |
| R7 | Ironwood Expanse | -157.5 | SW |
| R8 | Riverlands | 157.5 | W |

كل قطاع يغطي **45°** (360/8).

### 4.2 تمثيل عملي للتنفيذ (Polygons تقريبية AABB-sectors)

لأن pathfinding أسهل مع مضلعات، نعرّف لكل Region مضلعًا (مستطيل مشذّب) داخل الحلقة:

> الصيغة: نقسم الإطار الخارجي إلى 8 مستطيلات متداخلة جزئيًا مع belt الجبال.

#### إحداثيات AABB الأساسية لكل Region (قبل خصم mountain corridor)

نفترض إطار Zone1 مقسومًا كشبكة 3×3 منطقية (الزوايا+الأضلاع) مع المركز = Z2/Z3:

```
Cell size outer band:
  west band  X: 0..700
  east band  X: 1700..2400
  south band Y: 0..700
  north band Y: 1700..2400
  (corners belong to diagonal regions)
```

| Region | AABB [x0,y0,x1,y1] | ملاحظات |
|--------|---------------------|----------|
| **R2 Frostvale (N)** | [700,1700, 1700,2400] | ضلع شمالي |
| **R4 Sunrise (E)** | [1700,700, 2400,1700] | ضلع شرقي |
| **R6 Dunes (S)** | [700,0, 1700,700] | ضلع جنوبي |
| **R8 Riverlands (W)** | [0,700, 700,1700] | ضلع غربي |
| **R3 Eagle (NE)** | [1700,1700, 2400,2400] | ركن NE |
| **R5 Amber (SE)** | [1700,0, 2400,700] | ركن SE |
| **R7 Ironwood (SW)** | [0,0, 700,700] | ركن SW |
| **R1 Northern Marches (NW)** | [0,1700, 700,2400] | ركن NW |

هذه الـ 8 AABBs = **Partition كامل** لـ Zone1 بدون تداخل (ما عدا الحدود المشتركة).

### 4.3 أحزمة الجبال بين الأقاليم (Mountain Belts)

سمك الحزام: **40 tiles** يتمركز على الحدود المشتركة.

أمثلة:
- بين R2 و R3: شريط حول X=1700, Y=1700..2400 → mountain
- بين R2 و R1: شريط حول X=700, Y=1700..2400
- بين R2 و Zone2: شريط داخلي حول Y=1700, X=700..1700

**الممر (Pass)** = فتحة walkable داخل الحزام بعرض **20 tiles** وعمق = سمك الحزام.

```
Mountain belt thickness Tm = 40
Pass opening width Wp = 20
Pass center sits on shared border midpoint
```

---

## 5) Zone 2 — 4 مناطق توسع

Zone2 outer `[700,700]–[1700,1700]` minus Zone3 `[1000,1000]–[1400,1400]`.

نقسم الحلقة لـ 4 أقاليم:

| ID | الاسم | AABB التقريبي (الجزء الخارجي) | يغذي من |
|----|-------|-------------------------------|---------|
| Z2N | North Expansion | [700,1400,1700,1700] + أكتاف | R1,R2,R3 |
| Z2E | East Expansion | [1400,700,1700,1700] | R3,R4,R5 |
| Z2S | South Expansion | [700,700,1700,1000] | R5,R6,R7 |
| Z2W | West Expansion | [700,700,1000,1700] | R7,R8,R1 |

> عند التنفيذ: استخدم polygon = (Z2 outer quadrant) − (Z3 rectangle) لتجنب تداخل القلب.

جبال داخلية بين أرباع Z2: أحزمة 30 tile + Pass Lv4.

---

## 6) Zone 3 — التصفية (تفاصيل إحداثيات)

```
Zone3: [1000,1000]–[1400,1400]
```

### حلقات داخلية اختيارية (للقراءة البصرية فقط)
| الحلقة | AABB | محتوى |
|--------|------|--------|
| Outer objectives | [1000,1000]–[1400,1400] minus mid | 4–8 Forts |
| Mid | [1080,1080]–[1320,1320] | Side altars |
| Core podium | [1140,1140]–[1260,1260] | Throne |

### Throne / Core object
```
id: throne_core
position: (1200, 1200)
footprint: 12×12
capture_radius: 8
hold_score_per_minute: 10
```

### Outer forts (4)
| ID | Position |
|----|----------|
| OF_N | (1200, 1320) |
| OF_E | (1320, 1200) |
| OF_S | (1200, 1080) |
| OF_W | (1080, 1200) |

### Side altars (4)
| ID | Position |
|----|----------|
| SA_NE | (1300, 1300) |
| SA_SE | (1300, 1100) |
| SA_SW | (1100, 1100) |
| SA_NW | (1100, 1300) |

---

## 7) مواصفات الممرات (Pass Spec) بالإحداثيات

### 7.1 Border Passes داخل Zone1 (8 أساسية)

المركز = منتصف الحد المشترك:

| Pass ID | من | إلى | مركز تقريبي (x,y) | Level |
|---------|----|----|---------------------|------:|
| P_R1_R2 | R1 | R2 | (700, 2050) | 2 |
| P_R2_R3 | R2 | R3 | (1700, 2050) | 2 |
| P_R3_R4 | R3 | R4 | (2050, 1700) | 2 |
| P_R4_R5 | R4 | R5 | (2050, 700) | 2 |
| P_R5_R6 | R5 | R6 | (1700, 350) | 2 |
| P_R6_R7 | R6 | R7 | (700, 350) | 2 |
| P_R7_R8 | R7 | R8 | (350, 700) | 2 |
| P_R8_R1 | R8 | R1 | (350, 1700) | 2 |

**هندسة الممر:**
```
pass_aabb centered on center:
  if vertical border: width=Tm=40, height=Wp=20
  if horizontal border: width=Wp=20, height=Tm=40
terrain inside pass_aabb = plain (walkable)
surrounding belt = mountain
```

### 7.2 Inner Passes Zone1 → Zone2 (8)

على الحد الداخلي `700/1700`:

| Pass ID | من | إلى | مركز (x,y) | Level | unlock_day |
|---------|----|----|------------|------:|-----------:|
| P_R2_Z2 | R2 | Z2N | (1200, 1700) | 3 | 10 |
| P_R4_Z2 | R4 | Z2E | (1700, 1200) | 3 | 10 |
| P_R6_Z2 | R6 | Z2S | (1200, 700) | 3 | 10 |
| P_R8_Z2 | R8 | Z2W | (700, 1200) | 3 | 10 |
| P_R3_Z2 | R3 | Z2N/Z2E | (1700, 1700) | 3 | 14 |
| P_R5_Z2 | R5 | Z2E/Z2S | (1700, 700) | 3 | 14 |
| P_R7_Z2 | R7 | Z2S/Z2W | (700, 700) | 3 | 14 |
| P_R1_Z2 | R1 | Z2W/Z2N | (700, 1700) | 3 | 14 |

### 7.3 Passes داخل Zone2 (4)
| Pass ID | من | إلى | مركز | Level |
|---------|----|----|------|------:|
| P_Z2N_Z2E | Z2N | Z2E | (1550, 1550) | 4 |
| P_Z2E_Z2S | Z2E | Z2S | (1550, 850) | 4 |
| P_Z2S_Z2W | Z2S | Z2W | (850, 850) | 4 |
| P_Z2W_Z2N | Z2W | Z2N | (850, 1550) | 4 |

### 7.4 Final Gates → Zone3 (4)
| Gate ID | من | مركز على حد Z3 | Level | unlock_day |
|---------|----|----------------|------:|-----------:|
| FG_N | Z2N | (1200, 1400) | 6 | 35 |
| FG_E | Z2E | (1400, 1200) | 6 | 35 |
| FG_S | Z2S | (1200, 1000) | 6 | 35 |
| FG_W | Z2W | (1000, 1200) | 6 | 35 |

---

## 8) نقاط الولادة (Spawn)

لكل Region Zone1: **6–12 spawn slots** داخل Safe band بعيدًا عن الحدود بـ ≥ 60 tiles.

### مراكز spawn المقترحة (نقطة مرجعية)
| Region | Spawn anchor (x,y) |
|--------|---------------------|
| R1 | (350, 2050) |
| R2 | (1200, 2050) |
| R3 | (2050, 2050) |
| R4 | (2050, 1200) |
| R5 | (2050, 350) |
| R6 | (1200, 350) |
| R7 | (350, 350) |
| R8 | (350, 1200) |

حول كل anchor: توزيع jitter في دائرة نصف قطرها 80–140 tiles مع رفض mountain/water/pass radius.

---

## 9) Holy Sites (Zone1 Altars)

| Region | Altar ID | Position | Buff theme |
|--------|----------|----------|------------|
| R1 | A_R1 | (250, 1900) | Storm (speed) |
| R2 | A_R2 | (1200, 2200) | Earth (def) |
| R3 | A_R3 | (2150, 1900) | Flame (atk) |
| R4 | A_R4 | (2200, 1200) | Harvest (gather) |
| R5 | A_R5 | (2150, 500) | Surge (skill) |
| R6 | A_R6 | (1200, 200) | Wisdom (research/xp) |
| R7 | A_R7 | (250, 500) | Harvest |
| R8 | A_R8 | (200, 1200) | Earth |

---

## 10) كثافة الموارد (جدول توليد)

| Zone | Node levels | كثافة (node / 100×100 tiles) |
|------|-------------|-------------------------------:|
| Z1 | 1–4 | 8–14 |
| Z2 | 3–6 | 6–10 |
| Z3 | 5–7 seasonal | 2–4 (أهداف أهم من farm) |

أوزان الموارد حسب `resource_bias` في `data/zones.json`.

---

## 11) Pathfinding & Traverse Rules (تنفيذ)

1. Grid A* أو JPS على walkable tiles.
2. `mountain/water = blocked`.
3. Edge عبر Pass مسموح فقط إذا:
   `pass.owner_alliance_id in {traveler.alliance, traveler.coalition}`.
4. March ETA:
   `distance_tiles / march_speed_tiles_per_second`.
5. سرعات أساس (tiles/sec) مسودة:
   - Inf 1.00 → 1.0 t/s
   - Cav 1.15
   - Arch 0.95
   - Siege 0.80
   - Scout 1.60  
   (اضبط لاحقًا لزمن عبور إقليم 3–8 دقائق)

### مسافات مرجعية
| المسار | مسافة تقريبية | زمن Inf @1 t/s |
|--------|---------------:|----------------:|
| Spawn R2 → P_R2_R3 | ~500 | ~8.3 min |
| R2 anchor → Inner P_R2_Z2 | ~350 | ~5.8 min |
| FG_N → Throne | ~200 | ~3.3 min |

---

## 12) AOI / الشبكة الشبكية

```
aoi_cell = 20 tiles
grid = 120 x 120 cells
player interest = Moore neighborhood radius 2 (5×5 cells) + always-subscribe own marches/alliance war targets
```

---

## 13) JSON تنفيذي

الملفات:
- `data/zones.json` — بنية zones
- `data/passes.json` — IDs/levels
- `data/map_spec_coordinates.json` — **الإحداثيات الكاملة الدقيقة** (مولَّد مع هذا المستند)

---

## 14) مقياس MVP السريع (1200×1200)

اضرب كل إحداثيات × **0.5**:
- World 1200
- Z3: [500,500]–[700,700]
- Z2 outer: [350,350]–[850,850]
- Tm mountain = 20, Wp pass = 10

---

## 15) Checklist هندسي قبل الكود

- [ ] Tilemap 2400² أو chunked 256²
- [ ] Bake mountain belts + pass openings
- [ ] Region polygon contains(x,y)
- [ ] Pass graph connectivity test (unit tests)
- [ ] Spawn validator
- [ ] March path demo بين R1↔R2 عبر pass فقط
- [ ] Unlock days gate inner/final passes
- [ ] Minimap colors per region + owner outline

---

## 16) علاقة هذا بـ RoK الأصلي

| RoK (من التفكيك) | ROK2 Spec |
|------------------|-----------|
| 8 starting provinces في Lost Kingdom | 8 regions في Zone1 |
| King's Land center | Zone3 throne core |
| Pass levels chapter-gated | unlock_day + levels |
| Mountains/rivers as terrain mats | mountain belts + optional rivers |
| Client visual EZ tiles | محركنا Tilemap/chunks |

**لسنا ننسخ إحداثياتهم؛ نبني مواصفة أوضح للتنفيذ.**
