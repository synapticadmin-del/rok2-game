// Copyright ROK2. Game mode for the kingdom.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Rok2Types.h"
#include "Rok2GameMode.generated.h"

class URok2Api;
class URok2BootWidget;
class URok2HudWidget;
class URok2BuildMenuWidget;
class URok2CommanderWidget;
class URok2AllianceRosterWidget;
class URok2BattleReportWidget;
class URok2OnboardingWidget;
class URok2CivInfoWidget;
class URok2ChatWidget;
class URok2SeasonStoryWidget;
class URok2ResearchWidget;
class URok2TrainHealSheetWidget;
class ARok2ViewManager;
class ARok2CityBuilder;

UCLASS(minimalapi)
class ARok2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARok2GameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The Cloudflare API base URL. Editable in editor for dev vs prod. */
	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString ApiBaseUrl;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString KingdomId;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	FString AdminKey;

	/** Shared HTTP + WS client for all actors. */
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(Transient)
	URok2BootWidget* BootWidget;

	/**
	 * HUD موحد بأسلوب RoK (P5-T3): موارد + أزرار دائرية + طوابير + إشعارات.
	 *
	 * P24-T1: صار الطبقة الوحيدة فوق العالم بعد تقاعد `URok2CityWidget`. تلك
	 * الودجة كانت تبني ثلاثة ألواح ثم تخفيها كلها بـ`ESlateVisibility::Collapsed`
	 * بلا أي مسار يعيد إظهارها، وتُحدّث محتواها كل ثانية في `NativeTick` وهي غير
	 * مرئية. ما كان فريداً فيها انتقل إلى مواضعه: التحصيل والتدريب وتسريع
	 * الطوابير إلى الـHUD، وإنشاء التحالف إلى شاشة التحالف.
	 */
	UPROPERTY(Transient)
	URok2HudWidget* HudWidget;

	/**
	 * طبقة إرشاد الدقيقة الأولى (P6-T4). ترتيب الطبقات: Boot 100 > الإرشاد 60
	 * > اللوحات 50 > HUD 20 — فوق اللوحات لتبقى البطاقة مرئية وورقة البناء
	 * مفتوحة (وهي الورقة التي تُرشد إليها الخطوة الأولى)، وتحت Boot فلا تظهر
	 * على شاشة التحميل.
	 */
	UPROPERTY(Transient)
	URok2OnboardingWidget* OnboardingWidget;

	/** مدير العرض مدينة/خريطة */
	UPROPERTY(Transient)
	ARok2ViewManager* ViewManager;

	/** واجهات تُفتح عند الطلب (تُنشأ مرة وتُخفى) */
	UPROPERTY(Transient)
	URok2BuildMenuWidget* BuildMenuWidget;

	UPROPERTY(Transient)
	URok2CommanderWidget* CommanderWidget;

	UPROPERTY(Transient)
	URok2AllianceRosterWidget* AllianceWidget;

	UPROPERTY(Transient)
	URok2BattleReportWidget* BattleReportWidget;

	/** P6-T5: شاشة هوية الحضارة — نبذتها الأدبية وتحيتها وتلميحاتها */
	UPROPERTY(Transient)
	URok2CivInfoWidget* CivInfoWidget;

	// P6-T6: دردشة حية
	UPROPERTY(Transient)
	URok2ChatWidget* ChatWidget;

	// P7-T1: حكاية المملكة — تُنشأ عند الطلب وتبقى متزامنة مع لقطة API.
	UPROPERTY(Transient)
	URok2SeasonStoryWidget* SeasonStoryWidget;

	// P18-T1: شاشة البحث العلمي — تُنشأ عند الطلب (من الأكاديمية أو بطاقة المبنى).
	UPROPERTY(Transient)
	URok2ResearchWidget* ResearchWidget;

	/**
	 * P18-T6: شاشة الإعدادات — تُنشأ عند أول فتح وتبقى. لا حالة لعب فيها،
	 * فإعادة عرضها لا تحتاج مزامنة مع الخادم (تُقرأ من `URok2Accessibility`
	 * و`URok2AudioManager` عند كل فتح).
	 */
	UPROPERTY(Transient)
	class URok2SettingsWidget* SettingsWidget;

	/** P19-T5: شاشة الحقيبة — تُنشأ عند أول فتح وتُعاد، وتُجلب لقطتها كل فتح. */
	UPROPERTY(Transient)
	class URok2BagWidget* BagWidget;

	/** P19-T4: شاشة الحانة والصناديق — تُفتح من بطاقة مبنى الحانة. */
	UPROPERTY(Transient)
	class URok2TavernWidget* TavernWidget;

	/** P18-T2: مسار الزر الثانوي لبطاقة المبنى (research/train/heal/chests) —
	 *  عام لأن CityBuilder يربطه عبر AddDynamic عند إنشاء البطاقة؛ كان
	 *  الحدث يُبث بلا أي مشترك فأزرار (تدريب/شفاء/بحث/صناديق) تفتح لا شيء. */
	UFUNCTION()
	void HandleBuildingAction(const FString& BuildingId, const FString& ActionKind);

	/** P18-T1: فتح شاشة البحث (إنشاء كسول + إحضار الشجرة من الخادم). */
	void OpenResearchScreen();

	/** P19-T4: فتح شاشة الحانة (إنشاء كسول + جلب حالة المفاتيح والرميات). */
	void OpenTavernScreen();

protected:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Rok2")
	float TickIntervalSeconds;

	UFUNCTION()
	void OnPlayerLoadedHandler(const FRok2Player& Player);

	// --- معالجات أحداث HUD (P5-T3) ---
	UFUNCTION() void HandleBuildAction();
	UFUNCTION() void HandleEditCityAction();
	UFUNCTION() void HandleCommandersAction();
	UFUNCTION() void HandleAllianceAction();
	UFUNCTION() void HandleItemsAction();
	UFUNCTION() void HandleEventsAction();
	UFUNCTION() void HandleMapAction();
	UFUNCTION() void HandleReportsAction();
	UFUNCTION() void HandleCivInfoAction();
	UFUNCTION() void HandleChatAction();
	UFUNCTION() void HandleSeasonStoryAction();
	UFUNCTION() void HandleResearchAction();
	UFUNCTION() void HandleCollectAction();
	UFUNCTION() void HandleTrainAction();
	UFUNCTION() void HandleSettingsAction();
	UFUNCTION() void HandleSeasonStoryEvent(const FRok2SeasonStoryEntry& Event);
	UFUNCTION() void HandleBuildMenuPick(const FString& BuildingId);

	/** ربط أحداث HUD بالمعالجات بعد إنشائها */
	void BindHudEvents();

	/** يجلب/ينشئ ViewManager ويربطه بالـ CityBuilder والكاميرا */
	void EnsureViewManager();
};
