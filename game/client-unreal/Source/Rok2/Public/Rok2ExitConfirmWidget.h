// Copyright ROK2. لوحة تأكيد الخروج (P18-T5) — header.
//
// آخر ضغطة رجوع على أندرويد تُغلق التطبيق. من غير تأكيد، لمسةٌ واحدة على زر
// النظام تُخرج اللاعب من مملكته وهو ينظر إلى الخريطة — وهو ما تمنعه القاعدة
// «لا فعل لا رجعة فيه بلا تأكيد» في `07-game-design/ui-ux-design-system.md` §6.
//
// اللوحة عمداً بلا اتصال بالـ API: لا حالة لعب فيها ولا رقم توازن، فهي سطح
// واجهة صافٍ. `URok2Api` لا يُمرَّر إليها كي لا تُخلق تبعية لا سبب لها.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2ExitConfirmWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2ExitConfirmWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	/**
	 * P18-T5: الرجوع **داخل** لوحة التأكيد يعني «تراجعت» لا «أكّد». ضغطة رجوع
	 * ثانية كانت ستُغلق التطبيق لو لم تكن اللوحة طبقة قابلة للتسريح، لأن
	 * `DismissTopLayer` كانت ستعيد false وقد يفهمها المتصل «لا شيء ليُغلق».
	 */
	virtual void DismissLayer() override { OnCancelClicked(); }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();
};
