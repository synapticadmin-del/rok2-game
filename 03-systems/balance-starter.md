# القيم الرقمية الابتدائية (Starter Balance)

> ليست نهائية — للـ prototype والمحاكاة على Excel/Sheets.

## 1) إنتاج مباني مستوى 1 (لكل ساعة)
| مبنى | إنتاج |
|------|------:|
| Farm L1 | 100 food |
| Lumber L1 | 100 wood |
| Quarry L1 | 70 stone |
| Goldmine L1 | 40 gold |

كل مستوى ≈ +15% إلى +25% عن السابق (منحنى متناقص لاحقًا).

## 2) أوقات البناء (L1→L2 أمثلة)
| مبنى | وقت |
|------|-----|
| Farm | 10s–1m مبكرًا ثم يتصاعد |
| City Hall | الأطول في كل bracket |
| Hospital/Academy | متوسط-عالي |

استخدم speedups كـ sink أساسي.

## 3) مسيرات
| النوع | سرعة نسبية |
|------|------------|
| Cavalry | 1.15 |
| Infantry | 1.00 |
| Archer | 0.95 |
| Siege | 0.80 |
| Scout | 1.60 |

## 4) Counter multipliers (مسودة)
| Attacker → Defender | Mult |
|---------------------|-----:|
| Inf → Cav | 1.15 |
| Cav → Arch | 1.15 |
| Arch → Inf | 1.15 |
| Siege → City garrison buildings | 1.25 |
| Mirror matchups | 1.00 |
| Weak side | 0.87 |

## 5) Pass capture
- DPS على garrison bar أو hold timer
- Pass L2: يحتاج تنظيم صغير
- Final Gate L6: يحتاج تحالف كامل + نوافذ زمنية

## 6) Zone scoring (Finals)
- Core control: 10 pts/min
- Outer fort: 3 pts/min
- Kill points inside Z3: x1.5

## 7) F2P vs P2P
الهدف: الدفع يسرّع الزمن 2x–5x في مسارات، لا يخلق وحدة غير قابلة للقتل.
