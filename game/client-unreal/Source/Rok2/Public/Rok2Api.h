// Copyright ROK2. Cloudflare API + WebSocket client UObject.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "IWebSocket.h"
#include "Templates/SharedPointer.h"
#include "Rok2Types.h"
#include "Rok2Api.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginComplete, const FString&, Token);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCityLoaded, const FRok2City&, City);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLoaded, const FRok2Player&, Player);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldSnapshot, const FRok2WorldSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApiError, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToast, const FString&, Message);

/** حالة اتصال العميل بالخادم — تُبث للواجهات لعرض شاشة التحميل/إعادة الاتصال */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConnectionState, bool, bOnline, const FString&, StatusMessage);

/** يُبث عند وصول تقرير قتال جديد أو تحديث قائمة التقارير (P1-T4) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleReports, const TArray<FRok2BattleReport>&, Reports);

/** يُبث عند اكتمال سحب بيانات التوازن من الخادم (P1-T6) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMetaLoaded, bool, bFromServer);

/** يُبث عند تحديث حالة المناطق (فتح/قفل) — P2-T4/P2-T6 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonesUpdated, const TArray<FRok2ZoneStatus>&, Zones);

/** يُبث عند إضافة إشعار HUD جديد (P2-T6) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHudNotification, const FRok2HudNotification&, Notification);

UCLASS(BlueprintType, Blueprintable)
class URok2Api : public UObject
{
	GENERATED_BODY()

public:
	// Lifecycle
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Init(const FString& ApiBaseUrl, const FString& KingdomId, const FString& AdminKey);

	/** يسحب بيانات التوازن الموحدة من /v1/meta/all (P1-T6) — تُستدعى تلقائياً من Init */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FetchMeta();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void LoginAsGuest();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void InitCity(const FString& Civ, const FString& PlayerName);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void LoadCity();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UpgradeBuilding(const FString& BuildingId);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SpeedupQueue(const FString& QueueId);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Train(const FString& UnitId, int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void CreateAlliance(const FString& Name, const FString& Tag);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RefreshWorld();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AttackPass(const FString& PassId, const TMap<FString, int32>& Troops);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void MarchTo(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& Troops);
	
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void DispatchMarch(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& Troops, const FString& PrimaryCommander, const FString& SecondaryCommander);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AllianceHelp();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ForceTick();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetSeasonDay(int32 Day);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ConnectWebSocket();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void DisconnectWebSocket();

	// Polling pump - called from GameMode Tick
	void PumpEvents(float DeltaSeconds);

	// Populate civilization list from BlueprintLibrary
	void SetCivilizations(const TArray<FRok2Civilization>& InCivs) { Civilizations = InCivs; }

	// Accessors
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FString& GetToken() const { return Token; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2Player& GetPlayer() const { return Player; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2City& GetCity() const { return City; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2Civilization>& GetCivilizations() const { return Civilizations; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2WorldSnapshot& GetWorldSnapshot() const { return World; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TMap<FString, int32>& GetBuildings() const { return Buildings; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2TroopEntry>& GetTroops() const { return Troops; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2Commander>& GetCommanders() const { return Commanders; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2BattleReport>& GetBattleReports() const { return BattleReports; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2GameMeta& GetMeta() const { return Meta; }

	/** إشعارات الـ HUD المخزنة (الأحدث أولاً، حد أقصى 20) — P2-T6 */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2HudNotification>& GetNotifications() const { return Notifications; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	int32 GetUnreadNotificationsCount() const { return UnreadNotifications; }

	/** يصفّر عدّاد غير المقروء (عند فتح مركز الإشعارات) — P2-T6 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void MarkNotificationsRead() { UnreadNotifications = 0; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsLoggedIn() const { return !Token.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool HasPlayer() const { return !Player.Id.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsWsConnected() const { return bWsConnected; }

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnLoginComplete OnLoginComplete;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnCityLoaded OnCityLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnPlayerLoaded OnPlayerLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnWorldSnapshot OnWorldSnapshot;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnApiError OnApiError;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnToast OnToast;

	/** يُبث عند تغير حالة الاتصال: دخول/خروج وضع عدم الاتصال أو بدء إعادة المحاولة */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnConnectionState OnConnectionState;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnBattleReports OnBattleReports;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnMetaLoaded OnMetaLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnZonesUpdated OnZonesUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnHudNotification OnHudNotification;

protected:
	FString BaseUrl;
	FString KingdomId;
	FString AdminKey;
	FString Token;
	FString DeviceId;

	FRok2Player Player;
	FRok2City City;
	TArray<FRok2Civilization> Civilizations;
	FRok2WorldSnapshot World;
	TArray<FRok2TroopEntry> Troops;
	TMap<FString, int32> Buildings;
	TArray<FRok2Commander> Commanders;
	TArray<FRok2BattleReport> BattleReports;
	FRok2GameMeta Meta;

	// ---- HUD الموحد (P2-T6) ----
	/** سجل الإشعارات (الأحدث أولاً) */
	TArray<FRok2HudNotification> Notifications;
	int32 UnreadNotifications = 0;
	int32 NotificationSeq = 0;
	/** يضيف إشعاراً ويبثه للـ HUD */
	void PushNotification(const FString& Kind, const FString& Title, const FString& Body, float TtlSeconds = 6.f);

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> PendingRequest;
	TSharedPtr<IWebSocket> WebSocket;
	float WorldPollTimer = 0.f;
	bool bWsConnected = false;

	// ---- الموارد الحية (P1-T5) ----
	/** مؤقّت مزامنة المدينة من الخادم — يعمل فقط عند اتصال الـ WS (غير متصل = polling عالم سريع) */
	float CitySyncTimer = 0.f;
	/** الفاصل الزمني لمزامنة المدينة عبر REST عند اتصال الـ WS (ثانية) */
	static constexpr float CitySyncIntervalSeconds = 30.f;

	/** يعيد حساب معدلات الإنتاج من مستويات المباني (نفس معادلة الخادم) */
	void RecomputeResourceRates();

	// ---- إعادة الاتصال التلقائي (P1-T2) ----
	/** مؤقّت إعادة محاولة WebSocket — backoff أسّي حتى WsReconnectMaxDelay */
	float WsReconnectTimer = 0.f;
	float WsReconnectDelay = 2.f;
	/** هل طُلب الاتصال الحي؟ نعيد المحاولة فقط إذا كان اللاعب دخل فعلاً */
	bool bWsDesired = false;
	/** أقصى تأخير بين محاولات إعادة الاتصال (ثانية) */
	static constexpr float WsReconnectMaxDelay = 30.f;
	/** مهلة طلب HTTP الافتراضية (ثانية) */
	static constexpr float HttpTimeoutSeconds = 15.f;
	/** أقصى عدد محاولات إعادة لطلبات القراءة عند أخطاء الشبكة */
	static constexpr int32 HttpMaxRetries = 2;

	void SetOnline(bool bNewOnline, const FString& Reason);

	void Get(const FString& Path, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk);
	void Post(const FString& Path, const FString& JsonBody, bool bAuth, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr = nullptr);

	/** HTTP داخلي مع retry backoff لأخطاء الشبكة (لا يعيد المحاولة على أخطاء 4xx المنطقية) */
	void RequestWithRetry(const FString& Verb, const FString& Path, const FString& JsonBody, bool bAuth,
		TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr, int32 MaxRetries);

	void ParsePlayer(const TSharedPtr<FJsonObject>& Obj);
	void ParseCity(const TSharedPtr<FJsonObject>& Obj);
	void ParseWorld(const TSharedPtr<FJsonObject>& Obj);

	/** يحوّل كائن march من JSON إلى FRok2MarchEntity (P1-T3) */
	void ParseMarchEntity(const TSharedPtr<FJsonObject>& M, FRok2MarchEntity& E) const;
	/** يضيف/يحدّث/يزيل مسيرة في World.Marches ويبث التحديث (من أحداث الـ WS) */
	void UpsertMarch(const FRok2MarchEntity& E);

	/** يحوّل خريطة troops JSON إلى قائمة خسائر مرتبة (P1-T4) */
	static void ParseTroopMap(const TSharedPtr<FJsonObject>& Obj, TArray<FRok2TroopLoss>& Out);
	/** يحوّل تقرير قتال من JSON إلى FRok2BattleReport */
	void ParseBattleReport(const TSharedPtr<FJsonObject>& Obj, FRok2BattleReport& Out) const;
	/** يضيف تقريراً في مقدمة القائمة (حد أقصى 25) ويبث التحديث */
	void AddBattleReport(const FRok2BattleReport& R);

	FString AuthHeader() const;
	FString BuildUrl(const FString& Path) const;
	void EmitToast(const FString& Msg) { OnToast.Broadcast(Msg); }
	void EmitError(const FString& Msg) { OnApiError.Broadcast(Msg); }
};
