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
	UPROPERTY(Transient) UVerticalBox* ToastsBox;
	UPROPERTY(Transient) UBorder* NotifCenterPanel;
	UPROPERTY(Transient) UScrollBox* NotifList;

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
	UFUNCTION() void OnBellClicked();

	UFUNCTION() void OnNotification(const FRok2HudNotification& N);
	UFUNCTION() void OnZones(const TArray<FRok2ZoneStatus>& Zones);
	UFUNCTION() void OnConnState(bool bOnline, const FString& StatusMessage);

	void UpdateResources();
	void UpdateQueues();
	void UpdateSeasonAndZones();
	void UpdateNotifications();
	void UpdateBellBadge();
	void UpdateBuildBadge();
	void UpdateChatBadge();

private:
	int32 LastNotifCount = 0;
};
