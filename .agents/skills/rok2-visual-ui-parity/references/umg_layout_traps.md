# مصائد تخطيط UMG — أعطال حقيقية وقعت في هذا المشروع

> كلٌّ منها كان يمرّ بحارس بنيوي أخضر. الترتيب بالأثر البصري.

## المحتويات

1. `SetDesiredSizeOverride` لا يُخزَّن في `UImage`
2. `bMatchSize=true` يفرض مقاس الملف على التخطيط
3. طبقات `UOverlay` لا تتمدد افتراضياً
4. `SetAutoWrapText` بلا حدٍّ أفقي لا يلتفّ
5. ارتفاع ثابت + `Fill` = تراكب
6. لوح ثابت بلا `ScrollBox` يقصّ صامتاً
7. `AddToViewport()` بلا ZOrder يضع اللوح تحت الـHUD
8. لوح يُبنى ثم يُخفى بلا مسار إظهار
9. `PlayFadeOut` يترك خصائص الرسم فيعود اللوح شفافاً

---

## 1. `SetDesiredSizeOverride` لا يُخزَّن في `UImage`

تنفيذ المحرك (`Engine/Source/Runtime/UMG/Private/Components/Image.cpp`):

```cpp
void UImage::SetDesiredSizeOverride(FVector2D DesiredSize)
{
    if (MyImage.IsValid()) { MyImage->SetDesiredSizeOverride(DesiredSize); }
}
```

لا حقل يُحفظ، ولا `UPROPERTY` في `Image.h`، ولا إعادة تطبيق في
`SynchronizeProperties`. فنداؤه داخل `NativeConstruct` على ودجة بُنيت لحظتها
— **قبل** `Super::RebuildWidget()` الذي ينشئ `MyImage` — **لا يفعل شيئاً**.

**الحل:** ثبّت `Brush.ImageSize` (وهو `UPROPERTY` يبقى) ثم استدعِ الـoverride
بعده ليسري على ودجة معروضة:

```cpp
FSlateBrush Brush;
Brush.ImageSize = DrawSize;
Brush.SetResourceObject(Texture);
Image->SetBrush(Brush);
Image->SetDesiredSizeOverride(DrawSize);
```

## 2. `bMatchSize=true` يفرض مقاس الملف

`SetBrushFromTexture(Tex, true)` يضبط `ImageSize` إلى أبعاد الملف الكاملة، و
`SImage::ComputeDesiredSize` تعود إليها عند غياب override. فخلفية 2560×1440
وشعار 1920×1920 فرضا على بطاقة 840×680 حجماً أكبر من الشاشة ودفعا كل ما بعدهما
خارجها.

**لا تستخدم `bMatchSize=true` في تخطيط محدود.** مرِّر `false` واضبط المقاس.

## 3. طبقات `UOverlay` لا تتمدد افتراضياً

`UOverlaySlot` افتراضه `HAlign_Left` / `VAlign_Top`. فطبقة الخلفية ترسم بمقاس
فرشاتها في زاوية الطبقة لا تملأها.

```cpp
UOverlaySlot* Slot = Layers->AddChildToOverlay(Image);
Slot->SetHorizontalAlignment(HAlign_Fill);
Slot->SetVerticalAlignment(VAlign_Fill);
```

(بخلاف `UVerticalBoxSlot` الذي افتراضه `Fill`/`Fill` مع `Size = Automatic`.)

## 4. `SetAutoWrapText` بلا حدٍّ أفقي لا يلتفّ

النصّ العربي أطول من عرض البطاقة، والالتفاف يحتاج حدّاً. حُلّ مرتين في هذا
المشروع: `USizeBox` بـ`SetWidthOverride` (لوحة النبذة، بطاقة الإرشاد)، أو
`ESlateSizeRule::Fill` على شقّ العمود **مع** صفٍّ أبيه على `HAlign_Fill` —
لأن `Fill` داخل صفٍّ موسَّط بالتوسيط لا يملك عرضاً يلتفّ فيه.

## 5. ارتفاع ثابت + `Fill` = تراكب

`SizeBox` بارتفاع 268px ومحتواه يحتاج ≈310px، وصفّ التفاصيل على
`ESlateSizeRule::Fill`: النتيجة أن `Fill` يأخذ ما بقي **وهو سالب**، فيُسحق صفّ
التنقّل داخل النصّ فتركب الأزرار على الكلام.

**قبل تثبيت ارتفاع، اجمع احتياج المحتوى**: (شعار 64) + (اسم ≈34) + (فانتازي
≈24) + (تفاصيل 96) + (تنقّل ≈40) + الهوامش. وإن لم يكن الرقم أكيداً، اترك
الارتفاع تلقائياً وحُدّ العرض وحده.

## 6. لوح ثابت بلا `ScrollBox` يقصّ صامتاً

بطاقة 840×680 ومحتوى ينمو مع كل بند: ما يفيض **يُبتر بلا أي أثر** — وهكذا اختفى
زر «ابدأ رحلة». أضف `UScrollBox` عمودياً:

```cpp
UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(...);
Scroll->SetOrientation(EOrientation::Orient_Vertical);
Card->SetContent(Scroll);
Scroll->AddChild(VBox);
```

عمودي فقط: العرض محدود بالبطاقة، وأي تمرير أفقي يعني تخطيطاً مكسوراً.

## 7. `AddToViewport()` بلا ZOrder

الافتراضي 0، أي **تحت** الـHUD (20). ترتيب الطبقات في المشروع:

```
Boot 100 · BuildingDetail 200 · TrainHealSheet 150 · ExitConfirm 90
Onboarding 60 · Settings 58 · RallyReports 55 · لوحات 50 · HUD 20
```

## 8. لوح يُبنى ثم يُخفى بلا مسار إظهار

`URok2CityWidget` كانت تبني ثلاثة ألواح ثم تخفيها بـ`Collapsed` بلا أي مسار في
المشروع يعيد إظهارها — فبقيت ~250 سطراً تُحدَّث كل ثانية وهي غير مرئية، وثلاثة
أوامر خادمية بلا مستدعٍ يراه لاعب. **عند إخفاء لوح، ابحث عن مسار الإظهار
بـgrep؛ إن لم يوجد فاللوح ميت.**

## 9. `PlayFadeOut` يترك خصائص الرسم

بعد الإزالة بتلاشٍ تبقى `RenderOpacity=0` فيعود اللوح **شفافاً** في الفتح
الثاني. أعِد التصفير:

```cpp
W->RemoveFromParent();
W->SetRenderOpacity(1.f);
W->SetRenderTranslation(FVector2D::ZeroVector);
W->SetRenderScale(FVector2D(1.f, 1.f));
```
