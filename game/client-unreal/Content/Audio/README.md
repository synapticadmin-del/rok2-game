# Produced Audio Assets — ROK2 (P4-T2 + P4-T3)

أصول صوتية حقيقية مولّدة إجرائياً (procedural synthesis) خصيصاً للمشروع —
لا أصول طرف ثالث، فلا قيود ترخيص. التوليد عبر `scripts/generate_audio.py`
(numpy فقط) ومخرجاته 16-bit PCM WAV 44.1kHz Mono — الصيغة القياسية
لاستيراد Unreal Engine كـ `USoundWave`.

## البصمة الصوتية لكل حضارة

| الحضارة | السلم/الأسلوب | الطابع |
|---|---|---|
| rome | خماسي رسمي + طبول رباعية | فنفار نحاسية وقورة |
| china | خماسي صيني (C-D-E-G-A) | كوتشين هادئ متداخل |
| arabia | حجاز (D-Eb-F#-G) | ناي + إيقاع مقسوم متمايل |
| egypt | فريجي (E-F-G-A-B) | ناي طويل + قرع هيبطي |
| vikings | درون باس + أبواق حرب | قاسٍ بطيء ثقيل |
| japan | insen scale (B-C-E-F-G) | كوتو متباعد + شاكوهاشي |

كل موسيقى حلقة 20-24 ثانية بتلاشي نهائي (تتكرر بسلاسة في `URok2AudioManager`).

## موسيقى المعركة (P4-T3)

`battle.wav` لكل حضارة: نسخة قتالية من نفس سلم الحضارة — نبض أسرع
(126-150 BPM مقابل 70-110)، طبول حرب مكثفة، نغمات قصيرة عدوانية متصاعدة.
`URok2AudioManager::EnterBattleMode()` يبدّل إليها عند تقرير قتال ويعود
تلقائياً لموسيقى السلام بعد `BattleModeTimeout` (30 ثانية افتراضياً).

## المؤثرات (sfx/)

build_complete, upgrade, victory, defeat, march_start, button_click,
notification — كلها مؤثرات قصيرة (0.1-2 ثانية) مشتركة بين الحضارات.

## مؤثرات أحداث اللعب (P4-T4)

| الملف | الحدث المربوط |
|---|---|
| gather_complete.wav | عودة مسيرة جمع بالموارد (march_returning بـ kind gather/node) |
| research_complete.wav | اكتمال بحث (WS tech_researched) |
| heal_complete.wav | بدء شفاء جرحى (URok2Api::HealWounded → POST /v1/city/heal) |
| zone_unlock.wav | فتح منطقة جديدة (WS zone_unlocked) |
| rally_launch.wav | انطلاق حملة rally (WS rally_launched) |

## التخزين (base64)

الملفات على الفرع مخزّنة نصياً base64 (نفس اصطلاح P2-T7 للـ GLB).
`game/client-unreal/setup_level.py` يفك ترميزها تلقائياً قبل الاستيراد
(خطوة Step 1c)، و`scripts/decode_binary_assets.py` يفكها دفعة واحدة
خارج المحرر. بدون فك الترميز يبقى `URok2AudioManager` على الصمت
الافتراضي (placeholder — لا يُكسر البناء).

## إعادة التوليد

```bash
python3 scripts/generate_audio.py   # يعيد بناء كل WAV (سلام + معركة + مؤثرات)
```
