// Copyright ROK2. شاشة الإعدادات (P18-T6) — header.
//
// المشكلة: `URok2Accessibility::SetUiScale` و`SetHighContrast` معرّفتان منذ
// P7-T7 **بلا أي مستدعٍ في المشروع** — فمقياس الواجهة 1.0 دائماً والتباين
// العالي مطفأ دائماً، وكل ما بُني عليهما (`ScaledSize`, `ScaledIconSize`,
// `GetScaledPx`, `AccessibleTextFor` في عشرات المواضع) كان يعمل على قيمة ثابتة.
// وكذلك `MasterVolume` و`bAudioEnabled` في مدير الصوت: حقلان عامّان لا شاشة
// تلمسهما.
//
// هذه الشاشة هي المستدعي الغائب: أربعة إعدادات تُطبَّق **فوراً** وتُحفظ محلياً.
//
// لماذا لا `GameUserSettings`: إعداداتنا أربعة تفضيلات واجهة (مقياس/تباين/
// صوتان) لا إعدادات رسم (دقة/جودة/ظلال). ربطها بـ`UGameUserSettings` كان
// سيجرّ نظام مستويات الجودة كله لأجل أربعة حقول، بينما `USaveGame` هو نمط
// التخزين المحلي القائم في المشروع (`URok2CityLayoutSaveGame`).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2SettingsWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2SettingsWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	// P18-T6: الرجوع يغلق الشاشة كزر «تم». لا زر «إلغاء»: كل تغيير يسري فوراً
	// ويُحفظ لحظتها، فلا حالة معلّقة يمكن التراجع عنها — وهذا اختيار مقصود،
	// لأن سماع الصوت ورؤية الحجم هما طريقة ضبطهما الوحيدة.
	virtual void DismissLayer() override { OnCloseClicked(); }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** شرائح المستويات والمقياس. */
	UPROPERTY(Transient) USlider* MusicSlider;
	UPROPERTY(Transient) USlider* SfxSlider;
	UPROPERTY(Transient) USlider* UiScaleSlider;

	/** قيمة كل شريح كنص — لا شريح بلا رقم مقروء (§8.3 «مؤشر غير لوني»). */
	UPROPERTY(Transient) UTextBlock* MusicValueText;
	UPROPERTY(Transient) UTextBlock* SfxValueText;
	UPROPERTY(Transient) UTextBlock* UiScaleValueText;

	UPROPERTY(Transient) UCheckBox* HighContrastCheck;
	UPROPERTY(Transient) UCheckBox* AudioEnabledCheck;

	UFUNCTION() void OnMusicVolumeChanged(float Value);
	UFUNCTION() void OnSfxVolumeChanged(float Value);
	UFUNCTION() void OnUiScaleChanged(float Value);
	UFUNCTION() void OnHighContrastChanged(bool bChecked);
	UFUNCTION() void OnAudioEnabledChanged(bool bChecked);
	UFUNCTION() void OnResetClicked();
	UFUNCTION() void OnCloseClicked();

	/**
	 * صف واحد: عنوان + شريح + قيمة نصية. لا يربط المعالج — الربط في
	 * `NativeConstruct` بـ`AddDynamic` المتحقَّق منه وقت الترجمة، لا بالاسم
	 * الذي يفشل بصمت (وهو ما يحرسه `Rok2DelegateBind.h`).
	 */
	USlider* BuildSliderRow(UVerticalBox* Column, const FString& Label, const FString& IconId,
		float Value, float Min, float Max, UTextBlock*& OutValueText);

	/** صف تبديل: عنوان + مربع اختيار + شرح سطر واحد. */
	UCheckBox* BuildToggleRow(UVerticalBox* Column, const FString& Label, const FString& IconId,
		const FString& Hint, bool bChecked);

	/** يملأ الودجات من الحالة الحيّة — يُنادى عند البناء وبعد الاستعادة. */
	void SyncFromState();

	/** نصوص القيم: نسبة مئوية للصوت ومضاعف للمقياس. */
	void UpdateValueTexts();
};
