// Copyright ROK2. Civilization identity screen (P6-T5) — «شاشة معلوماتها».
//
// المواصفة (PLAN.md P6-T5): النبذة الأدبية «تظهر عند اختيار الحضارة **وفي
// شاشة معلوماتها**». الأولى في Rok2BootWidget، وهذه الثانية: الشاشة التي
// يفتحها اللاعب بعد أن صارت له حضارة، فيقرأ حكايتها وتحيتها وتلميحاتها.
//
// ────────────────────────────────────────────────────────────────────────────
//  لوحة سفلية لا نافذة مركزية
// ────────────────────────────────────────────────────────────────────────────
// ui-ux-design-system.md §1: «كل لوحة تنزلق من أسفل كـ Bottom Sheet»، و§9
// يمنع «شاشات منفصلة بخلفية سوداء» — فاللوحة تُفتح فوق المدينة الحيّة وتُغلق
// بلمس الخلفية، كبطاقة المبنى تماماً. النَفَس القصصي يُقرأ **والمدينة مرئية
// خلفه**، وهو المقصد: الحكاية عن هذه المدينة لا عن شيء في قائمة.
//
// ────────────────────────────────────────────────────────────────────────────
//  حضارة اللاعب من الخادم لا من اختيار محلي
// ────────────────────────────────────────────────────────────────────────────
// الحضارة تُقرأ من FRok2Player::Civ الذي يملؤه ParseCity من حمولة الخادم —
// لا من آخر ما اختاره اللاعب في القائمة. الفرق يظهر في حالة واقعية: لاعب
// عائد لم يمرّ بشاشة الاختيار في هذه الجلسة إطلاقاً، فقراءة الاختيار المحلي
// كانت ستريه حكاية روما لأنها أول خيار في القائمة.
//
// وحضارة بلا نبذة (معرّف من خادم أحدث من نصوص العميل) تُعرض بترويستها
// واسمها بلا متن — لا نصّ بديل يُخترع، ولا شاشة فارغة تُفتح بلا سبب.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2CivInfoWidget.generated.h"

class URok2Api;
class UBorder;
class UTextBlock;
class UVerticalBox;
class UCanvasPanel;

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2CivInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** يربط الـApi ويرسم حضارة اللاعب الحالية */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Lore")
	void Setup(URok2Api* InApi);

	/**
	 * يعيد الرسم من حضارة اللاعب الحالية. يُنادى عند كل فتح — فاللوحة تُنشأ
	 * مرة وتُعاد للعرض مراراً (اصطلاح GameMode)، ولو رُسمت مرة واحدة لبقيت
	 * على حضارة قديمة بعد تغيير الحضارة.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Lore")
	void RefreshFromPlayer();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	UBorder* SheetBorder;

	/** اسم الحضارة العربي — دور Display */
	UPROPERTY(Transient)
	UTextBlock* NameText;

	/** جملة الفانتازي الواحدة (Pillar Fantasy) */
	UPROPERTY(Transient)
	UTextBlock* FantasyText;

	/** النبذة الأدبية بأسطرها المؤلَّفة */
	UPROPERTY(Transient)
	UTextBlock* StoryText;

	/** التحية بنبرة الحضارة */
	UPROPERTY(Transient)
	UTextBlock* GreetingText;

	/** حاوية التلميحات — سطر لكل تلميح بأيقونة */
	UPROPERTY(Transient)
	UVerticalBox* HintsBox;

	virtual void NativeConstruct() override;

	/** يبني اللوحة السفلية وأقسامها */
	void BuildSheet(UCanvasPanel* RootCanvas);

	/** يملأ التلميحات (يُفرَّغ أولاً — اللوحة تُعاد للعرض مراراً) */
	void FillHints(const TArray<FString>& Hints);

	UFUNCTION()
	void OnCloseClicked();

	/** الحضارة معروضة الآن — يمنع إعادة بناء صفوف التلميحات بلا تغيّر */
	FString RenderedCivId;
};
