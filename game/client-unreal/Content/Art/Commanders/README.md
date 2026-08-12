# Commander Portraits — ROK2 (P4-T2 + P7-T14)

بورتريهات القادة الـ18 من `data/commanders.json` — أصول PNG حقيقية
(512×512) بأسلوب painterly-realistic مطابق للمرجع البصري المعتمد في
`07-game-design/assets/commanders-lineup.jpg`: bust framing، إضاءة حافة
ذهبية درامية، خلفية داكنة معمارية لكل حضارة.

## الفهرس — القادة المبدئيون (starters)

| الملف | القائد | الندرة | الحضارة |
|---|---|---|---|
| cmd_rome_starter.png | Scipio | elite | روما |
| cmd_china_starter.png | Sun Tzu | epic | الصين |
| cmd_arabia_starter.png | Baibars | epic | العرب |
| cmd_egypt_starter.png | Cleopatra | epic | مصر |
| cmd_vikings_starter.png | Ragnar | epic | الفايكنج |
| cmd_japan_starter.png | Kusunoki Masashige | epic | اليابان |

## بورتريهات roster 2 (cmd_*_2 — P7-T14)

| الملف | القائد | الندرة | الحضارة |
|---|---|---|---|
| cmd_rome_2.png | Germanicus | epic | روما |
| cmd_china_2.png | Zhuge Liang | legendary | الصين |
| cmd_arabia_2.png | Saladin | legendary | العرب |
| cmd_egypt_2.png | Imhotep | epic | مصر |
| cmd_vikings_2.png | Lagertha | epic | الفايكنج |
| cmd_japan_2.png | Oda Nobunaga | legendary | اليابان |

## البورتريهات الأسطورية (legends)

| الملف | القائد | الندرة | الحضارة |
|---|---|---|---|
| julius_caesar.png | Julius Caesar | legendary | روما |
| richard_lionheart.png | Richard the Lionheart | legendary | الفايكنج |
| yi_seong_gye.png | Yi Seong-Gye | legendary | اليابان |
| genghis_khan.png | Genghis Khan | legendary | العرب |
| joan_of_arc.png | Joan of Arc | legendary | مصر |
| alexander_great.png | Alexander the Great | legendary | روما |

## كيف تُستخدم

`URok2CommanderWidget` يحمّل البورتريه من `/Game/Art/Commanders/<id>`
(الأصل المستورد) في بطاقات القائمة ولوحة التفاصيل — وإن لم يُستورد يبقى
الـ placeholder الملوّن بحرف الاسم (لا يُكسر العرض). الاستيراد عبر
`setup_level.py` (Step 1c) أو سحب الملفات إلى Content Browser.

## التخزين (base64)

كما في اصطلاح P2-T7: الملفات على الفرع نص base64، ويُفك ترميزها تلقائياً
قبل الاستيراد (`setup_level.py` / `scripts/decode_binary_assets.py`).
