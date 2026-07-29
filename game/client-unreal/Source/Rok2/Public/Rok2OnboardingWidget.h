// Copyright ROK2. First-minute onboarding — guidance overlay (P6-T4).
//
// طبقة الإرشاد: بطاقة أنيقة تعرض الخطوة الحالية، وحلقة ذهبية نابضة تُبرز الزر
// المطلوب. النموذج (الخطوات والنصوص والتقدّم) في Rok2Onboarding — هذه الودجة
// **عرض فقط**: تسأل النموذج وترسم، ولا تحمل نصاً ولا تعرف ترتيب الخطوات.
//
// ────────────────────────────────────────────────────────────────────────────
//  موضع البطاقة — مقيس من تخطيط الـHUD لا مُختار بالذوق
// ────────────────────────────────────────────────────────────────────────────
// الشاشة مشغولة في ثلاث جهات من أربع:
//   • الشريط العلوي: عرض كامل × 46px  (BuildTopBar)
//   • أعلى يمين: لوحة الطوابير + رصّة الإشعارات  (أنكر 1,0)
//   • أسفل يمين: عنقود الأزرار الدائرية  (أنكر 1,1) ← وهو **هدف الخطوة الأولى**
//   • أسفل: ورقة البناء السفلية عرضٌ كامل × 340px + خلفية معتمة تغطي الشاشة
//
// فالبطاقة **أعلى اليسار تحت الشريط** (16, 58): الجهة الحرة الوحيدة، وهي نفسها
// ما تنصّ عليه وثيقة UI §3.5 «اليسار — المهام». وأي موضع سفلي كان سيُدفن تحت
// ورقة البناء البالغة 340px — وهي الورقة التي تُفتح في الخطوة الأولى بالضبط،
// أي أن البطاقة كانت ستختفي في اللحظة التي تُرشد فيها.
//
// ────────────────────────────────────────────────────────────────────────────
//  الطبقة لا تأكل لمسة واحدة
// ────────────────────────────────────────────────────────────────────────────
// البطاقة والحلقة كلتاهما HitTestInvisible. طبقةٌ إرشادية تحجب الإدخال تحوّل
// الدرس إلى طريق مسدود: الحلقة تحيط بالزر المطلوب، فلو التقطت اللمس لصار الزر
// الذي تشير إليه غير قابل للضغط. ولا زرّ «تخطٍّ» لأن المواصفة لا تطلبه —
// والبديل اللطيف أن الطبقة غير حاجبة أصلاً، فمن أراد تجاهلها لعب فوقها بحرية.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2Onboarding.h"
#include "Rok2OnboardingWidget.generated.h"

class URok2Api;
class UBorder;
class UTextBlock;
class UImage;
class UCanvasPanel;
class UVerticalBox;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2OnboardingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** يربط الـApi ويشترك في مفوَّضاته — لا Tick لتتبّع التقدّم */
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	void Setup(URok2Api* InApi);

	/** يعيد تقييم الخطوة ويحدّث البطاقة والحلقة */
	UFUNCTION(BlueprintCallable, Category = "Rok2|FTUE")
	void Refresh();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	/** جذر الطبقة — البطاقة والحلقة أبناؤه */
	UPROPERTY(Transient)
	UCanvasPanel* RootCanvas;

	// --- البطاقة ---
	UPROPERTY(Transient) UBorder* Card;
	UPROPERTY(Transient) UImage* StepIcon;
	UPROPERTY(Transient) UTextBlock* OrdinalText;
	UPROPERTY(Transient) UTextBlock* TitleText;
	UPROPERTY(Transient) UTextBlock* StoryText;
	UPROPERTY(Transient) UTextBlock* ActionText;

	virtual void NativeConstruct() override;

	void BuildCard();

	/** يملأ البطاقة من تعريف الخطوة */
	void ApplyStepInfo(const FRok2FtueStepInfo& Info);

	UFUNCTION()
	void OnCityLoadedHandler(const FRok2City& City);

	UFUNCTION()
	void OnWorldSnapshotHandler(const FRok2WorldSnapshot& Snapshot);

private:
	/**
	 * آخر خطوة رُسمت. تمنع إعادة بناء البطاقة وإعادة تشغيل الحركة في كل
	 * لقطة عالم واردة — المفوَّضان يُبثّان كثيراً، وبطاقة تُعيد الانزلاق مع
	 * كل نبضة شبكة تصير وميضاً مزعجاً لا إرشاداً.
	 */
	ERok2FtueStep LastRenderedStep = ERok2FtueStep::None;

	/** هل عُرضت بطاقة التتويج؟ (تُعرض مرة واحدة ثم تتلاشى الطبقة) */
	bool bCelebrated = false;
};
