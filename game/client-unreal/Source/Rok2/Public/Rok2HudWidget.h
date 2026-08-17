// Copyright ROK2. Unified HUD widget (P5-T3) — RoK-style game HUD.
// Built fully in code (no Blueprint asset required), layered above URok2CityWidget.
//
// المواصفة: 07-game-design/ui-ux-design-system.md
//  - شريط موارد علوي ذهبي (طعام/خشب/حجر/ذهب/gems/AP) + يوم الموسم + مؤقّت المناطق + شارة اتصال + جرس.
//  - أزرار دائرية مزخرفة أسفل يمين: البناء (مطرقة كبيرة) + القادة/التحالف/الحقيبة/الأحداث.
//  - زر خريطة + تقارير + زر تحرير المدينة أسفل يسار/وسط.
//  - لوحة طوابير جانبية بشريط تقدم + عدّ تنازلي.
//  - بطاقات إشعارات تتلاشى + مركز إشعارات قابل للطي.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2HudWidget.generated.h"

class URok2Api;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UBorder;
class UProgressBar;
class UScrollBox;
class UImage;
class UCanvasPanel;
class URok2Accessibility;

/** حدث زر في الـ HUD — يفوَّض لـ Blueprint/GameMode */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHudAction);

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2HudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

	// أحداث الأزرار — تفوَّض للخارج
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnMapAction;
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnReportsAction;
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnBuildAction;      // زر البناء الكبير
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnCommandersAction; // القادة
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnAllianceAction;   // التحالف
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnItemsAction;      // الحقيبة
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnEventsAction;     // الأحداث
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnEditCityAction;   // تحرير المدينة
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnCivInfoAction;
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnChatAction;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Season Story") FOnHudAction OnSeasonStoryAction;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Research") FOnHudAction OnResearchAction;

	// P24-T1: أفعال ورثها الـHUD من `URok2CityWidget` المتقاعد. كانت أزرارها
	// تُبنى داخل ألواح مطوية بـ`ESlateVisibility::Collapsed` فلا يراها لاعب،
	// و`CollectCityProduction`/`CreateAlliance`/`SpeedupQueue` لم يكن لها مستدعٍ
	// آخر في المشروع كله — أي أن ثلاث نقاط نهاية خادمية كانت غير قابلة للوصول.
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnCollectAction;   // تحصيل الإنتاج
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnTrainAction;    // ورقة التدريب

	/**
	 * P18-T6: شاشة الإعدادات. `URok2Accessibility::SetUiScale/SetHighContrast`
	 * كانتا بلا مستدعٍ في المشروع كله — فمقياس الواجهة والتباين لم يكن للاعب
	 * سبيلٌ إلى تغييرهما، وكذلك مستوى الصوت.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Rok2") FOnHudAction OnSettingsAction;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	// شريط علوي — معلومات الحاكم والقوة
	UPROPERTY(Transient) UTextBlock* GovernorNameText;
	UPROPERTY(Transient) UTextBlock* GovernorPowerText;

	// شريط علوي — الموارد
	UPROPERTY(Transient) UTextBlock* ResFoodText;
	UPROPERTY(Transient) UTextBlock* ResWoodText;
	UPROPERTY(Transient) UTextBlock* ResStoneText;
	UPROPERTY(Transient) UTextBlock* ResGoldText;
	UPROPERTY(Transient) UTextBlock* ResGemsText;
	UPROPERTY(Transient) UTextBlock* ResApText;
	UPROPERTY(Transient) UTextBlock* SeasonText;
	UPROPERTY(Transient) UTextBlock* ZoneTimerText;
	UPROPERTY(Transient) UImage* ConnIcon;
	UPROPERTY(Transient) UTextBlock* ConnStateText;
	UPROPERTY(Transient) UTextBlock* BellBadgeText;
	UPROPERTY(Transient) UImage* BellIcon;

	// زر الدردشة + شارة غير المقروء
	UPROPERTY(Transient) UButton* ChatButton;
	UPROPERTY(Transient) UTextBlock* ChatBadgeText;
	UPROPERTY(Transient) UImage* ChatIcon;

	// شارة البنّاء الخامل على زر البناء
	UPROPERTY(Transient) UTextBlock* BuildBadgeText;

	// طوابير
	UPROPERTY(Transient) UVerticalBox* QueuesBox;
	UPROPERTY(Transient) UBorder* QueuesPanel;

	// إشعارات
	UPROPERTY(Transient)
	UVerticalBox* ToastsBox;

	/**
	 * P18-T3: بطاقة إشعار حيّة واحدة.
	 *
	 * **بنية عادية لا USTRUCT، وبلا `UPROPERTY` داخلها عن قصد.** زرع `UPROPERTY`
	 * في بنية غير منعكسة لا يفعل شيئاً: UHT لا يرى الحقل فلا يتبعه جامع
	 * القمامة، فيبدو الكود محمياً وهو ليس كذلك. مرساة الـGC الحقيقية هي
	 * `ToastCardRefs` أدناه.
	 */
	struct FToastEntry
	{
		/** ضعيف: الحركة تُزيل البطاقة من الشجرة عند انتهاء الخروج. */
		TWeakObjectPtr<UBorder> Card;

		/** ما بقي قبل بدء الخروج (ثانية). سالبٌ = الخروج جارٍ. */
		float Remaining = 0.f;

		/** أُطلقت حركة الخروج؟ يمنع إطلاقها كل إطار بعد نفاد المدة. */
		bool bExiting = false;
	};

	/** البطاقات الحيّة بترتيب الوصول — الأقدم أولاً. */
	TArray<FToastEntry> ActiveToasts;

	/**
	 * P18-T3: مرساة الـGC لبطاقات الإشعارات.
	 *
	 * البطاقات تُبنى بـ`NewObject` وتُضاف إلى `ToastsBox`، لكن حركة الخروج
	 * تُزيلها من الشجرة قبل أن نُسقط أثرها — ومن تلك اللحظة لا مالك منعكس لها.
	 * هذه القائمة تحفظها حتى نُطلقها بأنفسنا.
	 */
	UPROPERTY(Transient)
	TArray<UBorder*> ToastCardRefs;

	UPROPERTY(Transient)
	UBorder* NotifCenterPanel;
	UPROPERTY(Transient)
	UScrollBox* NotifList;

	/**
	 * P24-T1: معالجات صفوف الطوابير — كل زر تسريع يحمل معرّف طابوره.
	 * `UButton::OnClicked` بلا معاملات، فالمعرّف يسكن في كائن وسيط. القائمة
	 * تُفرَّغ مع كل إعادة بناء وإلا تراكمت عبر الجلسة.
	 */
	UPROPERTY(Transient) TArray<class URok2HudQueueAction*> QueueActions;

	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildTopBar(class UCanvasPanel* RootCanvas);
	void BuildActionCluster(class UCanvasPanel* RootCanvas);
	void BuildLeftCluster(class UCanvasPanel* RootCanvas);
	void BuildQueuesPanel(class UCanvasPanel* RootCanvas);
	void BuildToastsStack(class UCanvasPanel* RootCanvas);
	void BuildNotifCenter(class UCanvasPanel* RootCanvas);

	UFUNCTION() void OnBuildClickedHandler();
	UFUNCTION() void OnCommandersClickedHandler();
	UFUNCTION() void OnAllianceClickedHandler();
	UFUNCTION() void OnItemsClickedHandler();
	UFUNCTION() void OnEventsClickedHandler();
	UFUNCTION() void OnCivInfoClickedHandler();
	UFUNCTION() void OnMapBtnClickedHandler();
	UFUNCTION() void OnReportsBtnClickedHandler();
	UFUNCTION() void OnEditCityClickedHandler();
	UFUNCTION() void OnChatClickedHandler();
	UFUNCTION() void OnSeasonStoryClickedHandler();
	UFUNCTION() void OnResearchClickedHandler();
	UFUNCTION() void OnBellClicked();
	UFUNCTION() void OnCollectClickedHandler();
	UFUNCTION() void OnTrainClickedHandler();
	UFUNCTION() void OnSettingsClickedHandler();

	UFUNCTION() void OnNotification(const FRok2HudNotification& N);
	UFUNCTION() void OnToast(const FString& Message);
	UFUNCTION() void OnZones(const TArray<FRok2ZoneStatus>& Zones);
	UFUNCTION() void OnConnState(bool bOnline, const FString& StatusMessage);

	/**
	 * P18-T3: يُحدّث عمر البطاقات الحيّة ويُطلق الخروج مرة واحدة لكل بطاقة.
	 * يُنادى من `NativeTick` بالـDelta الحقيقي لا بدورة الربع ثانية، فمدة العرض
	 * تظل ثابتة مهما تغيّر معدل الإطارات.
	 */
	void TickToasts(float InDeltaTime);

	/**
	 * يُخرج أقدم بطاقة بحركة `PlayToastOut`. لا تُزال يدوياً — المكتبة تُزيلها
	 * عند انتهاء الحركة (§1 «لا قفزات جامدة»).
	 */
	void BeginToastExit(FToastEntry& Entry);

	/** أقصى عدد بطاقات معروضة معاً — من مواصفة UI/UX §7 وبند PLAN «سقف 3». */
	static constexpr int32 MaxVisibleToasts = 3;

	/** مدة بقاء البطاقة قبل بدء الخروج (ثانية). */
	static constexpr float ToastLifetimeSeconds = 4.f;

	void UpdateResources();
	void UpdateQueues();

	/**
	 * اسم موضوع الطابور بالعربية: وحدة من `Api->GetMeta()` أو مبنى من خريطة
	 * أسماء buildings.json. كان السطر يعرض `RefId Lv%d` — معرّفاً خادمياً
	 * لاتينياً وسط واجهة عربية.
	 */
	FString QueueSubjectName(const FRok2QueueEntry& Q) const;

	void UpdateSeasonAndZones();
	void UpdateNotifications();
	void UpdateBellBadge();
	void UpdateBuildBadge();
	void UpdateChatBadge();

private:
	int32 LastNotifCount = 0;
	float HudRefreshAccumulator = 0.f;
};

/**
 * P24-T1: وسيط زر تسريع طابور. `URok2QueueBtnHandler` كان يخدم نفس الغرض في
 * `URok2CityWidget` المتقاعد؛ هذا نظيره في الـHUD حيث تُعرض الطوابير فعلاً.
 */
UCLASS()
class ROK2_API URok2HudQueueAction : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FString QueueId;
	UPROPERTY() URok2Api* Api;

	UFUNCTION() void HandleClick();
};
