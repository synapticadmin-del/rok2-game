// Copyright ROK2. Unified HUD widget (P2-T6) — top bar, queues, notifications, bottom bar.
// Built fully in code (no Blueprint asset required), layered above URok2CityWidget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2HudWidget.generated.h"

class URok2Api;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UButton;
class UBorder;
class UProgressBar;
class UScrollBox;

/** حدث زر في الـ HUD — يفوَّض لـ Blueprint/GameMode */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHudAction);

/**
 * HUD موحد احترافي (P2-T6):
 *  - شريط علوي: الموارد الحية (تُحدَّث كل Tick) + يوم الموسم + مؤقّت فتح المناطق + شارة الاتصال + جرس الإشعارات.
 *  - لوحة طوابير جانبية: كل طابور بشريط تقدم حي وعدّ تنازلي.
 *  - بطاقات إشعارات تتراكم يميناً وتتلاشى (قتال/مناطق/rally/أبحاث).
 *  - شريط سفلي: أزرار التنقل (خريطة/تقارير) — تفوَّض لـ Blueprint عبر أحداث.
 *  - مركز إشعارات قابل للطي يعرض السجل الكامل من URok2Api.
 */
UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2HudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** اربط الـ HUD بمصدر البيانات */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

	/** يُستدعى من Blueprint عند ضغط زر الخريطة في الشريط السفلي */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnHudAction OnMapAction;

	/** يُستدعى من Blueprint عند ضغط زر التقارير */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnHudAction OnReportsAction;

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	// شريط علوي
	UPROPERTY(Transient) UTextBlock* HudResourcesText;
	UPROPERTY(Transient) UTextBlock* SeasonText;
	UPROPERTY(Transient) UTextBlock* ZoneTimerText;
	UPROPERTY(Transient) UTextBlock* ConnText;
	UPROPERTY(Transient) UTextBlock* BellBadgeText;

	// طوابير
	UPROPERTY(Transient) UVerticalBox* QueuesBox;
	UPROPERTY(Transient) UBorder* QueuesPanel;

	// إشعارات
	UPROPERTY(Transient) UVerticalBox* ToastsBox;
	UPROPERTY(Transient) UBorder* NotifCenterPanel;
	UPROPERTY(Transient) UScrollBox* NotifList;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void BuildTopBar(class UCanvasPanel* RootCanvas);
	void BuildQueuesPanel(class UCanvasPanel* RootCanvas);
	void BuildToastsStack(class UCanvasPanel* RootCanvas);
	void BuildBottomBar(class UCanvasPanel* RootCanvas);
	void BuildNotifCenter(class UCanvasPanel* RootCanvas);

	void UpdateResources();
	void UpdateSeasonAndZones();
	void UpdateQueues();
	void UpdateBellBadge();
	void TickToasts(float DeltaSeconds);

	UFUNCTION()
	void OnNotification(const FRok2HudNotification& N);

	UFUNCTION()
	void OnZones(const TArray<FRok2ZoneStatus>& Zones);

	UFUNCTION()
	void OnConnState(bool bOnline, const FString& StatusMessage);

	UFUNCTION()
	void OnBellClicked();

	UFUNCTION()
	void OnMapBtnClicked();

	UFUNCTION()
	void OnReportsBtnClicked();

private:
	struct FToastEntry
	{
		FString Id;
		float Remaining;

		UPROPERTY()
		UBorder* Card = nullptr;
	};

	UPROPERTY(Transient)
	TArray<UBorder*> ToastCardRefs; // إبقاء بطاقات الإشعارات حية للـ GC

	TArray<FToastEntry> ActiveToasts;
	float QueuesRefreshTimer = 0.f;
	bool bNotifCenterOpen = false;
};
