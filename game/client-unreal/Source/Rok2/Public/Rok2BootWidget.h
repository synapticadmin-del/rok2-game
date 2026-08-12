// Copyright ROK2. Boot UI widget (login + civ select + loading/connection status).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// ESelectInfo::Type يظهر في توقيع معالج تغيّر القائمة أدناه، فيُضمَّن صريحاً
// بدل الاعتماد على وصوله عَرَضاً عبر هيدر آخر (SlateCore وحدة عامة).
#include "Types/SlateEnums.h"
#include "Rok2Types.h"
#include "Rok2BootWidget.generated.h"

class URok2Api;
class UComboBoxString;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UBorder;
class UVerticalBox;
class UImage;

UCLASS()
class URok2BootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* EnterButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* NameInput;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* CivCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* StartButton;

	/** شاشة التحميل: نص الحالة أسفل البطاقة (جاري الاتصال / إعادة المحاولة / فشل) */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StatusText;

	/** حاوية مؤشر التحميل — تظهر أثناء أي عملية شبكة */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* LoadingPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* LoadingText;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	// -----------------------------------------------------------------------
	// P6-T5: بطاقة النَفَس القصصي — «نبذة أدبية تظهر عند اختيار الحضارة»
	//
	// تجلس تحت القائمة المنسدلة داخل نفس البطاقة، وتتبدّل نصّاً مع كل اختيار.
	// وثيقة UI §6 تطلب في النهاية كاروسيل بطاقات كاملة الشاشة بفنّ المدينة؛
	// هذا البند نصّي لا بصري، فالنصّ يحلّ الآن في مكانه من التخطيط القائم
	// ويرثه الكاروسيل حين يُبنى — لا نصّ يُكتب مرتين.
	// -----------------------------------------------------------------------

	/** لوحة النبذة (تظهر مع ظهور قائمة الاختيار) */
	UPROPERTY(Transient)
	UBorder* LorePanel;

	/** الاسم العربي + جملة الفانتازي في سطر واحد */
	UPROPERTY(Transient)
	UTextBlock* LoreHeadingText;

	/** النبذة الأدبية — 3-4 أسطر بفواصل مؤلَّفة */
	UPROPERTY(Transient)
	UTextBlock* LoreStoryText;

	/** التحية بنبرة الحضارة */
	UPROPERTY(Transient)
	UTextBlock* LoreGreetingText;

	/** آخر معرّف عُرضت نبذته — يمنع إعادة الحركة على اختيارٍ لم يتغيّر */
	FString LastLoreCivId;

	// P7-T3: بطاقة حضارة مرئية. تبقى CivCombo مصدر المعرّف المتوافق مع الخادم،
	// فيما ينقل هذا العرض الاختيار من قائمة نصية إلى كاروسيل قابل للمراجعة.
	UPROPERTY(Transient)
	UBorder* CivShowcasePanel;
	UPROPERTY(Transient)
	UImage* CivBackdropImage;
	UPROPERTY(Transient)
	UImage* CivEmblemImage;
	UPROPERTY(Transient)
	UImage* CivCommanderImage;
	UPROPERTY(Transient)
	UTextBlock* CivNameText;
	UPROPERTY(Transient)
	UTextBlock* CivFantasyText;
	UPROPERTY(Transient)
	UTextBlock* CivPerksText;
	UPROPERTY(Transient)
	UTextBlock* CivUnitText;
	UPROPERTY(Transient)
	UTextBlock* CivCounterText;
	UPROPERTY(Transient)
	UButton* PreviousCivButton;
	UPROPERTY(Transient)
	UButton* NextCivButton;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** يبني لوحة النبذة داخل الصندوق العمودي للبطاقة */
	void BuildLorePanel(class UVerticalBox* VBox);

	/** معرّف الحضارة المختارة الآن في القائمة (فارغ إن لا قائمة) */
	FString SelectedCivId() const;

	/** يعرض نبذة حضارة. معرّف بلا نبذة يُخفي اللوحة بلا ضجيج. */
	void ShowLoreFor(const FString& CivId);

	/** يبني كاروسيل الحضارة، ويقرأ صوره من /Game/Art بعد استيراد PNGs. */
	void BuildCivShowcase(class UVerticalBox* VBox);

	/** يحدّث الخلفية والشعار والقائد والنصوص من الحضارة المختارة. */
	void ShowCivVisuals(const FString& CivId);

	/** يختار حضارة بحسب ترتيب البيانات؛ يُستدعى من سهمَي الكاروسيل. */
	void SelectCivIndex(int32 RequestedIndex);

	UFUNCTION()
	void OnCivSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnPreviousCivClicked();

	UFUNCTION()
	void OnNextCivClicked();

	/** حمولة الخادم قد تصل بعد بناء القائمة — تُعاد الملء ويُحفظ الاختيار */
	UFUNCTION()
	void OnMetaLoaded(bool bFromServer);

	/** يملأ القائمة من الحضارات الحالية ويحاول استعادة معرّف مختار */
	void PopulateCivCombo(const FString& PreferCivId);

	UFUNCTION()
	void OnEnterClicked();

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnLoginComplete(const FString& Token);

	UFUNCTION()
	void OnApiError(const FString& Message);

	UFUNCTION()
	void OnPlayerLoaded(const FRok2Player& Player);

	UFUNCTION()
	void OnConnectionState(bool bOnline, const FString& StatusMessage);

	/** إظهار/إخفاء مؤشر التحميل مع نص اختياري */
	void SetLoading(bool bShow, const FString& Message = TEXT(""));

	/** نقاط متحركة لمؤشر التحميل */
	float LoadingDotsTimer = 0.f;
	FString LoadingBaseMessage;
	bool bLoadingVisible = false;
};
