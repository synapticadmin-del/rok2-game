// Copyright ROK2. عقد «طبقة قابلة للتسريح» (P18-T5).
//
// المشكلة: `ARok2PlayerController::OnEscape` كان جسمه تعليقاً واحداً
// («could close UI panels») — أي أن مفتاح Escape على الحاسوب وزر الرجوع على
// أندرويد لا يفعلان شيئاً مهما تراكمت اللوحات على الشاشة. وكل لوحة تُغلق نفسها
// بطريقتها: بعضها `PlayFadeOut(this)`، وبعضها `RemoveFromParent()` عارياً،
// وحكاية المملكة تبقى في المنفذ وتُطوى بـ`Collapsed`.
//
// لجعل «الرجوع» يغلق **الطبقة العليا** لا لوحة عشوائية، يلزم شيئان:
//   ١) طريقة موحّدة لسؤال لوحة: أأنت مفتوحة الآن؟ وأغلقي نفسك.
//   ٢) ترتيب طبقات معروف — وهو موجود أصلاً في `AddToViewport(ZOrder)`.
//
// هذا الملف يوفّر الأول. الثاني يقرأه `URok2UiStack` من
// `UGameViewportSubsystem::GetWidgetSlot`. فلا تحتفظ أي لوحة بسِجل خاص، ولا
// يوجد مصدر حقيقة ثانٍ لـ«ما هو مفتوح» يمكن أن ينحرف عن المنفذ الفعلي.
//
// الاستخدام في لوحة:
//     class URok2FooWidget : public UUserWidget, public IRok2DismissibleLayer
//     {
//         ...
//         virtual void DismissLayer() override { OnCloseClicked(); }
//     };
//
// التسريح **يجب** أن يمرّ بحركة (`URok2MotionLibrary::PlayFadeOut`) لا بإزالة
// مفاجئة — قاعدة `07-game-design/ui-ux-design-system.md` §1 «لا قفزات جامدة».

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Rok2DismissibleLayer.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class URok2DismissibleLayer : public UInterface
{
	GENERATED_BODY()
};

class ROK2_API IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	/**
	 * يغلق هذه الطبقة كما لو ضغط اللاعب زر الإغلاق فيها — بنفس الحركة والصوت
	 * وأي تنظيف تفعله اللوحة عادةً. لا تُنفّذ إزالة مباشرة هنا.
	 */
	virtual void DismissLayer() = 0;

	/**
	 * هل الطبقة مفتوحة ومرئية للاعب الآن؟
	 *
	 * التنفيذ الافتراضي يكفي كل لوحات المشروع: «في المنفذ وغير مطويّة». وهو
	 * ليس ترفاً — `URok2SeasonStoryWidget` تبقى في المنفذ بعد إغلاقها وتُطوى
	 * بـ`ESlateVisibility::Collapsed` فقط (اصطلاح قائم منذ P7-T1). بدون هذا
	 * السؤال كان زر الرجوع «يغلق» لوحةً مطويّة أصلاً فلا يرى اللاعب أثراً
	 * لضغطته — وهو أسوأ من ألّا يفعل شيئاً، لأنه يستهلك ضغطة صامتة.
	 */
	virtual bool IsLayerOpen() const;
};
