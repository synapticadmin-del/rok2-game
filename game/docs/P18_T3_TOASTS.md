# P18-T3 — التوستات: حركتان معرّفتان بلا مستدعٍ، وبطاقات لا تختفي

> المرجع الملزم: `07-game-design/ui-ux-design-system.md` §7 («إشعار داخل اللعبة:
> بطاقة تنزلق من الأعلى بأيقونة الحدث + تتلاشى — ولا توقف اللعب أبداً») و§1
> («لا قفزات جامدة» · «كل تأكيد له وميض ذهبي») و§9 (0.25s ease-out موحّد).
> الحارس: `game/client-unreal/scripts/verify_p18_t3_toasts.mjs` — 40 فحصاً.
> يُشغَّل في `npm run check:ue-contracts` بالوظيفة `test:p18-t3-toasts`.

---

## 1. ما كان مكسوراً

### أ. حركتان في المكتبة بلا مستدعٍ واحد

`URok2MotionLibrary::PlayToastIn` و`PlayToastOut` معرّفتان منذ P6-T3، والبحث
عنهما في المشروع يجدهما في `Rok2MotionLibrary.cpp` **وحده** — تعريفاً وتنفيذاً
وبلا أي موضع استدعاء. فبطاقة الإشعار كانت تظهر فجأة مكتملة الشفافية ولا تختفي
بحركة، وهو نقيض §7 حرفياً.

`PlayGoldFlash` في الحالة نفسها: معرّفة، منفّذة، بلا مستدعٍ. أي أن قاعدة §1 «كل
تأكيد له وميض ذهبي» لم تكن مطبَّقة في أي موضع في اللعبة كلها.

### ب. البطاقة بلا عمر

```cpp
void URok2HudWidget::OnToast(const FString& Message)
{
    if (!ToastsBox || Message.IsEmpty()) return;
    while (ToastsBox->GetChildrenCount() >= 3)
    {
        ToastsBox->RemoveChildAt(0);      // ← قصّ مفاجئ
    }
    UBorder* Card = NewObject<UBorder>(this);
    ...
    ToastsBox->AddChildToVerticalBox(Card)->SetPadding(...);
    // ولا شيء بعدها يزيل البطاقة
}
```

لا مؤقّت ولا عمر: البطاقات الثلاث تبقى على الشاشة حتى يدفعها توست رابع. ومع
`EmitToast` مستدعاة في 63 موضعاً، الشاشة تعلوها ثلاث بطاقات دائمة تحجب زاوية
الطوابير.

والسقف يُطبَّق بـ`RemoveChildAt(0)` — إزالة فورية تقصّ البطاقة من الشاشة، وهي
بالضبط «القفزة الجامدة» التي تمنعها §1.

---

## 2. الحل

### `FToastEntry` — ولماذا بلا `UPROPERTY` داخلها

```cpp
struct FToastEntry
{
    TWeakObjectPtr<UBorder> Card;   // الحركة تُزيل البطاقة من الشجرة
    float Remaining = 0.f;
    bool  bExiting  = false;
};

TArray<FToastEntry> ActiveToasts;

UPROPERTY(Transient)
TArray<UBorder*> ToastCardRefs;     // مرساة الـGC الحقيقية
```

بنية عادية لا `USTRUCT`، وبلا `UPROPERTY` داخلها **عن قصد**: زرع `UPROPERTY` في
بنية غير منعكسة لا يفعل شيئاً — UHT لا يرى الحقل فلا يتبعه جامع القمامة، فيبدو
الكود محمياً وهو ليس كذلك. هذا العطل بعينه يحرسه `verify_compile_risks` بمدقّق
R11، وكان **يفشل قبل هذا البند** لأنه يتحقق من بنية ومرساة لم تكونا موجودتين.

المؤشر ضعيف لأن المكتبة تُزيل البطاقة من الشجرة عند انتهاء حركة الخروج، ومن تلك
اللحظة لا مالك منعكس لها — فالمرساة الصريحة `ToastCardRefs` هي ما يحفظها، وتُنظَّف
مما لم يبق حيّاً كي لا تكبر بلا حدّ خلال جلسة طويلة.

### حارس `bExiting` — جوهر البند

```cpp
void URok2HudWidget::BeginToastExit(FToastEntry& Entry)
{
    if (Entry.bExiting) return;
    Entry.bExiting = true;
    if (UBorder* Card = Entry.Card.Get())
    {
        URok2MotionLibrary::PlayToastOut(Card);
    }
}
```

بدون الراية كان `NativeTick` يُطلق حركة الخروج في **كل إطار** بعد نفاد المدة،
فتُعاد الحركة من بدايتها كل 16ms والبطاقة لا تختفي أبداً — وتتراكم توينات على
الودجة نفسها. الراية تُرفع **قبل** تشغيل الحركة، ومسار الخروج وحيد فلا موضع
ثانٍ يتجاوز الحارس (الحارس البنيوي يتحقق أن `PlayToastOut` تُستدعى مرة واحدة في
الملف).

### العمر بالـDelta الحقيقي

```cpp
void URok2HudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TickToasts(InDeltaTime);                    // ← قبل البوابة

    HudRefreshAccumulator += InDeltaTime;
    if (HudRefreshAccumulator < 0.25f) return;
    ...
}
```

الـHUD يمرّ تحديثه الثقيل (الموارد والطوابير والشارات) عبر بوابة ربع ثانية.
أعمار البطاقات تُحدَّث **قبلها**: لو مرّت عبر البوابة لخُصمت المدة بقفزات 0.25s
فانحرفت مدة العرض عن ثانيتها المعلنة، وتأخّر الخروج إلى أول دورة تحديث.

ولا حساب شفافية يدوي في الودجة — التلاشي مسؤولية المكتبة، وتكراره محلياً ينحرف
عن المعيار الموحد في §9 ويُنتج تلاشيين متزامنين.

### السقف على البطاقات الحيّة

```cpp
int32 Live = 0;
for (const FToastEntry& Entry : ActiveToasts)
    if (!Entry.bExiting) ++Live;

for (int32 i = 0; i < ActiveToasts.Num() && Live >= MaxVisibleToasts; ++i)
{
    if (ActiveToasts[i].bExiting) continue;
    BeginToastExit(ActiveToasts[i]);
    --Live;
}
```

العدّ على أطفال الصندوق كان يشمل بطاقةً في منتصف حركة خروجها، فيتقلّص السقف
الفعلي إلى اثنتين بلا سبب. والتجاوز يُخرج الأقدم **بحركة** لا بقصّ.

### الوميض الذهبي على الجرس

```cpp
void URok2HudWidget::OnNotification(const FRok2HudNotification& N)
{
    UpdateBellBadge();
    if (BellIcon) URok2MotionLibrary::PlayGoldFlash(BellIcon);
}
```

الجرس هدف صحيح لسببين: أولاً إشعارات `combat`/`zone`/`rally` تذهب إلى مركز
الإشعارات بلا توست، فكان الرقم يتغيّر في زاوية الشاشة بلا ما يجذب النظر. وثانياً
الهدف `UImage` لا `UBorder` — المكتبة تصبغ `ColorAndOpacity` للصور فيظهر الذهب
فعلاً، أما الودجات غير المعروفة فتُعطى نبضة شفافية باهتة.

---

## 3. حرّاس أُصلحت أعطالها لا أعراضها

`verify_ui_motion` كان **204/7** قبل هذه الجلسة، وستة من فشله كانت الأعراض
المباشرة لهذا البند:

| الفحص الفاشل | السبب |
|---|---|
| `Rok2HudWidget.cpp uses PlayToastIn` | لا مستدعٍ |
| `Rok2HudWidget.cpp uses PlayToastOut` | لا مستدعٍ |
| `HUD toast exit triggered once (bExiting guard)` | الراية غير موجودة |
| `gold flash actually wired to a confirmation` | لا مستدعٍ لـ`PlayGoldFlash` |
| `flash target is a UImage` | لا هدف أصلاً |
| `all 10 widgets carry a P6-T3 note` | 8/10 — ملفان جديدان من P18 بلا الملاحظة |

صار **211/0**.

الفشل السابع كان **عطلاً في الحارس نفسه**: يبحث حرفياً عن
`ERok2AudioType::ButtonClick` بينما P7-T1 وحّدت أصوات الواجهة على
`UiButtonClick`. وكلاهما يشير إلى الملف نفسه في `BuildAudioPaths`
(`ButtonClick` أُبقي للتوافق مع الودجات القديمة فقط)، فالكود سليم والحارس هو
المخطئ. صار الفحص يقبل أيّهما لأنه يحرس **السلوك** (صوت نقرة عند الضغط) لا
التسمية.

و`verify_compile_risks` كان يفشل بمدقّق R11 على فحص واحد: «`FToastEntry` بلا
`UPROPERTY`، ومرساة الـGC هي `ToastCardRefs`» — بنيةٌ ومرساةٌ لم تكونا موجودتين.
صار أخضر (56 فحصاً + 15 اختبار نفي).

---

## 4. التحقق

| ما شُغِّل | النتيجة |
|---|---|
| `Build-Rok2.ps1 -Target Editor` (UE 5.4.4) | **EXIT=0**، بلا أخطاء |
| `verify_p18_t3_toasts.mjs` | **40 passed, 0 failed** |
| `verify_ui_motion.mjs` | **211/0** (كان 204/7) |
| `verify_compile_risks.mjs` | أخضر (كان يفشل بـR11) |
| `verify_p17_design_system.mjs` | 125/0 |
| `npm run check:fast` | أخضر |
| `npm run check:ue-contracts` | أخضر بالكامل |

---

## 5. التحفّظ الصريح

**لا قبول PIE.** لم يُرَ على الشاشة: انبثاق البطاقة من الأسفل، تلاشيها، مدة
العرض الفعلية، سلوك السقف عند وصول توستات متلاحقة، ولا وميض الجرس. التحقق كله
بنيوي + تجميع ناجح. يُغلق مع قبول PIE في P24-T7 / P23-T2.
