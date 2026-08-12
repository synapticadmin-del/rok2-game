# P7-T7 — قابلية الوصول والتعريب النهائي

حزمة قابلية الوصول الشاملة لواجهة الـ UMG بالكامل. يوثق هذا الدليل ما أُضيف، وكيف يُختبر في دورة PIE، ولماذا بقي البند في حالة WIP حتى التحقق الفعلي على الجهاز.

## ما الذي أُضيف

| المحور | التنفيذ | الموقع |
|---|---|---|
| RTL عربي فعلي | `URok2Accessibility::IsRtl()` — العنصر المركزي؛ كل نصوص الواجهة عربية ثابتة والترتيب الهيكلي يُبنى من اليمين (موارد HUD من اليمين، أزرار HUD مصطفة يمينًا) | `Source/Rok2/Public/Rok2Accessibility.h` |
| تكبير واجهة | `GetUiScale()` من `GetDPIScaleFactorAtPoint` (clamp 0.85–1.6) + `SetUiScale()` ديناميكيًا، و`SizeOf()` في `URok2Typography` تضرب كل أحجام النصوص بالمقياس (clamp 12–64) | `Rok2Accessibility.cpp` + `Rok2Typography.cpp` |
| أيقونات مقروءة | `ScaledIconSize()` — الأيقونات لا تصغر عن 18px حتى مع مقياس أقل من 1 | `Rok2Accessibility.cpp` |
| تباين WCAG AA | `AccessibleTextFor()` يتحقق من نسبة التباين ≥ 4.5:1 فوق `#1A120B` (relLum 0.0128) ويعوّض بالإيفوري عند الضعف؛ `HighContrastForState()` لألوان الحالات (أخضر/ذهبي/أحمر فاتحة AA) | `Rok2Accessibility.cpp` |
| وضع التباين العالي | `SetHighContrast(bool)` + `OnAccessibilityChanged` delegate | `Rok2Accessibility.h` |
| لا اعتماد على اللون فقط | حالة الاتصال صارت نصًا + لونًا (`ConnStateText` «متصل/منقطع»)، حالة الرالي صارت نصًا + بادئة شكلية (✔/◔/▲)، شارة unread بصيغة `(%d)`، أشرطة التقدم حصلت على نسبة نصية (`DetailXpText` «خبرة X/Y (٪)`) | `Rok2HudWidget` / `Rok2AllianceRallyWidget` / `Rok2ChatWidget` / `Rok2CommanderWidget` |
| نصوص بديلة للأيقونات | `LabelForIcon(IconId)` — قاموس عربي (~45 معرّفًا)، وُضع على كل أيقونة HUD رئيسية عبر `SetToolTipText()` في الودجات الـ 18 | `Rok2Accessibility.cpp` + كل `Rok2*Widget.cpp` |
| ألوان أحداث متباينة | ألوان `Rok2SeasonStory` (Gold/Azure/Crimson/Jade) فُتحت لتجاوز 4.5:1 فوق الخلفية الداكنة، مع بادئات رمزية (★/⚔/◈/•) لنوع الحدث | `Rok2SeasonStoryWidget.cpp` |

## كيف يُختبر في PIE

1. افتح العالم وشغّل HUD: يجب أن تظهر الموارد والأزرار مصطفة من اليمين.
2. اضبط DPI أعلى من الشاشة (أو استدعِ `SetUiScale(1.25)` في الكونسول/بلوبرنت): يجب أن يكبر كل نص وكل أيقونة دون كسر التخطيط.
3. مرر الفأرة فوق أي أيقونة HUD (موارد/أجراس/دردشة/تبويبات/بطاقات مباني): يجب أن يظهر tooltip عربي («طعام»، «إشعارات»، «دردشة المملكة»، «بناء»...).
4. افصل الشبكة (أو عطّل الـ backend): يجب أن يظهر نص «متصل/منقطع» بلون AA بجانب أيقونة الاتصال — وليس لونًا فقط.
5. انضم لرالي «قيد التجميع» ثم «انطلق»: يجب أن تختلف بادئة النص (◔/▲) مع اختلاف اللون.
6. فعّل وضع التباين العالي: النصوص المصبوغة تتحول للإيفوري القابل للقراءة.

## البند في حالة WIP حتى دورة PIE

لا توجد أدوات Unreal في بيئة التطوير؛ التعديلات هنا موثقة كودية (سكربت التحقق أدناه) والقبول النهائي مشروط بدورة PIE على Windows.

## التحقق الآلي

`game/backend/scripts/verify_p7_t7_accessibility.mjs` — يثبت وجود وحدة `URok2Accessibility`، وتوسيع `SizeOf` بالمقياس، ووجود `LabelForIcon`/`ToolTipText` في الودجات الـ 18، و`ConnStateText`، و`HighContrastForState`، وبادئات حالة الرالي، والنسبة النصية في `DetailXpText`.

```bash
cd game/backend && npm run test:p7-t7-accessibility   # تحقق مخصص
cd game/backend && npm run check                     # البوابة كاملة (جميع الفحوصات)
```

## سجل التغييرات

| الملف | التغيير |
|---|---|
| `Rok2Accessibility.h/.cpp` | وحدة جديدة: RTL/مقياس/تباين/قاموس مسميات الأيقونات |
| `Rok2Typography.cpp` | `SizeOf()` يضرب بالمقياس |
| `Rok2HudWidget.h/.cpp` | `ConnStateText`، أحجام مكبرة، tooltips، ألوان AA للحالات والشارات |
| `Rok2CommanderWidget.h/.cpp` | `DetailXpText` نسبة نصية لشريط الخبرة |
| `Rok2ChatWidget.cpp` | tooltips للتبويبات والأزرار + نص مرسل + شارة `(n)` بلون AA |
| `Rok2CityWidget.cpp` | tooltips (governor/conn/build/speedup) |
| `Rok2BattleReportWidget.cpp` | tooltips لنتيجة المعركة والأطراف |
| `Rok2BuildingDetailWidget.cpp` | tooltips لرأس المبنى وأيقونات الكلفة |
| `Rok2MarchPanel.cpp` | tooltips للمسيرات (scout/flag/redirect/dispatch) |
| `Rok2AllianceRosterWidget.cpp` | tooltip لزر مساعدة التحالف |
| `Rok2AllianceRallyWidget.cpp` | بادئات حالة الرالي + ألوان AA |
| `Rok2BootWidget.cpp` | tooltips لأزرار الدخول والبدء والتاج |
| `Rok2CivInfoWidget.cpp` | tooltips للتاج وزر الإغلاق |
| `Rok2BuildMenuWidget.cpp` | tooltips لعنوان القائمة والتبويبات وأيقونات المباني |
| `Rok2ResearchWidget.cpp` | tooltip لزر البحث |
| `Rok2OnboardingWidget.cpp` | tooltip لأيقونة خطوة الإرشاد |
| `Rok2SeasonStoryWidget.cpp` | ألوان AA + بادئات رمزية لنوع الحدث |
