// Copyright ROK2. حلّال طبقات الواجهة لزر الرجوع (P18-T5) — header.
//
// «الرجوع» يجب أن يغلق **آخر** ما فتحه اللاعب. المشروع لا يحتفظ بمكدّس لوحات،
// لكنه لا يحتاج واحداً: كل لوحة تُضاف بـ`AddToViewport(ZOrder)` وترتيبها
// محفوظ في `UGameViewportSubsystem`. فالطبقة العليا = أعلى ZOrder بين
// اللوحات المفتوحة، وهذا يطابق ما يراه اللاعب حرفياً.
//
// لماذا لا مكدّس خاص: أي سِجل محلي يصبح مصدر حقيقة ثانياً ينحرف عند أول لوحة
// تُغلق من مسار آخر (لمس الخلفية، زر إغلاق، تسريح تلقائي بعد إرسال أمر) —
// وكلها مسارات قائمة في هذا المشروع. قراءة المنفذ لا تنحرف أبداً.
//
// الترتيب القائم (من `ARok2GameMode` و`ARok2CityBuilder`):
//   Boot 100 · بطاقة المبنى 200 · ورقة التدريب 150 · الإرشاد 60
//   تقارير الرالي 55 · اللوحات 50 · الـHUD 20
//
// شاشة التحميل (Boot) مستثناة صريحاً: إغلاقها بزر الرجوع يترك اللاعب على عالم
// بلا واجهة دخول ولا مسار عودة.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Rok2UiStack.generated.h"

class UUserWidget;

UCLASS()
class ROK2_API URok2UiStack : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * أعلى طبقة مفتوحة قابلة للتسريح، أو nullptr إن لم يكن على الشاشة إلا
	 * الـHUD. تتجاهل `URok2BootWidget` وأي ودجة تحت `HudZOrder`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* FindTopDismissibleLayer(UObject* WorldContextObject);

	/**
	 * يغلق أعلى طبقة مفتوحة ويعيد true إن أغلق شيئاً. false تعني «لا شيء
	 * ليُغلق» — وعندها يتولّى المتصل قرار الخروج.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|UI", meta = (WorldContext = "WorldContextObject"))
	static bool DismissTopLayer(UObject* WorldContextObject);

	/** عدد الطبقات المفتوحة الآن — للتشخيص والاختبار. */
	UFUNCTION(BlueprintPure, Category = "Rok2|UI", meta = (WorldContext = "WorldContextObject"))
	static int32 CountOpenLayers(UObject* WorldContextObject);

	/** ترتيب الـHUD في المنفذ؛ ما دونه ليس «طبقة» تُغلق بالرجوع. */
	static constexpr int32 HudZOrder = 20;
};
