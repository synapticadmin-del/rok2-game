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

// FJsonObject نوع غير منعكس، فلا يولّد UHT تصريحاً أمامياً له —
// وكل استخداماته هنا عبر TSharedPtr، فالتصريح الأمامي كافٍ.
class FJsonObject;

// سياق إعادة المحاولة معرَّف داخل Rok2Api.cpp فقط — تصريح أمامي كافٍ لـ TSharedPtr.
struct FRok2RetryCtx;

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

/** يُبث عند اكتمال مزامنة القادة المملوكين من الخادم. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCommandersLoaded);

/** يُبث عند وصول شجرة البحث من الخادم (P18-T1). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResearchLoaded);

/** يُبث عند تحديث حالة المناطق (فتح/قفل) — P2-T4/P2-T6 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZonesUpdated, const TArray<FRok2ZoneStatus>&, Zones);

/** يُبث عند إضافة إشعار HUD جديد (P2-T6) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHudNotification, const FRok2HudNotification&, Notification);

// P6-T6: يُبث عند وصول رسالة دردشة جديدة
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessage, const FRok2ChatMessage&, Message);

// P7-T1: يُبث عند وصول معلم عام جديد في خط حكاية المملكة.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonStoryEvent, const FRok2SeasonStoryEntry&, Event);

/** يُبث عند مزامنة الراليات النشطة للتحالف؛ لا تُشتق من واجهة العميل. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceRalliesUpdated, const TArray<FRok2AllianceRally>&, Rallies);

// P8-T7: أنظمة القادة العميقة + حماية المدينة + المهام اليومية + الملك.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommanderTalents, const FRok2CommanderTalents&, Talents);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentUpdated, const TArray<FRok2EquipmentSlot>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldOptions, const TArray<FRok2ShieldOption>&, Options);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApStateChanged, const FRok2ActionPointState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestsUpdated, const FRok2QuestState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKingUpdated);

// P19-T5: الحقيبة — لقطة العناصر من `GET /v1/items/bag`.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagUpdated, const FRok2BagState&, State);

// P9-T7: النسيج الاجتماعي والاقتصادي — تقنية/أرض/متجر/ألقاب/VIP/Trading/هدايا.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceTechUpdated, const TArray<FRok2AllianceTechNode>&, Nodes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceTerritoryUpdated, const FRok2AllianceTerritoryState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceShopUpdated, const TArray<FRok2AllianceShopItem>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceTitleChanged, const FRok2AllianceTitle&, Title);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVipStatusUpdated, const FRok2VipStatus&, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradingOffersUpdated, const TArray<FRok2TradingOffer>&, Offers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllianceGiftsUpdated, const TArray<FRok2AllianceGift>&, Gifts);
// P10-T6: delegates لأوضاع اللعب المتكررة.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTavernUpdated, const FRok2TavernState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExpeditionUpdated, const FRok2ExpeditionState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCanyonUpdated, const FRok2CanyonState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOsirisUpdated, const FRok2OsirisState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventsUpdated, const FRok2EventsState&, State);
// P11-T6: delegates لـ Lost Kingdom
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLostKingdomUpdated, const FRok2LostKingdomState&, State);
// P12-T6: نهاية الموسم
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSeasonReportUpdated, const FRok2SeasonReport&, Report);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSeasonEnded);

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

	/** يحفظ تخطيط القلعة التجميلي كاملاً؛ الخادم يتحقق من الملكية والحدود والتراكب. لا تُستدعى نتيجة النجاح إلا بعد قبول النسخة السلطوية. */
	void SaveCityLayout(const TArray<FRok2CityLayoutPlacement>& Placements, TFunction<void(bool)> OnCompleted = nullptr);

	/** يسحب القادة المملوكين ومستوياتهم من الخادم السلطوي. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FetchCommanders();

	/** يسحب شجرة البحث كاملة: المستويات الحالية والتكاليف والمتطلبات — كلها
	 *  محسوبة على الخادم. يبثّ OnResearchLoaded عند الوصول. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Research")
	void FetchResearch();

	/** يبدأ بحث المستوى التالي لتقنية. الخادم يفرض الأكاديمية والمتطلبات
	 *  والتكلفة ويضيف الطابور؛ العميل لا يخصم مورداً ولا يقدّر مدة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Research")
	void StartResearch(const FString& TechId);

	/** آخر حالة بحث وصلت من الخادم. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Research")
	const FRok2ResearchState& GetResearchState() const { return ResearchState; }

	/** يبثّ رسالة قصيرة لكل مستمعي OnToast — عام لأن GameMode يستخدمه
	 *  لإعلانات الواجهة (مثل توجيه التافرنا إلى شاشة P19-T4). */
	void EmitToast(const FString& Msg) { OnToast.Broadcast(Msg); }

	/** يستهلك تومات خبرة لرفع مستوى قائد مملوك؛ الخادم يتحقق من كل القيم. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void LevelUpCommander(const FString& CommanderId, int32 Tomes);

	/** يرفع مهارة قائد مملوك في خانة 1..3؛ الخادم يفرض شروط المستوى والتكلفة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UpgradeCommanderSkill(const FString& CommanderId, int32 SkillSlot);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UpgradeBuilding(const FString& BuildingId);

	/** زر التسريع الافتراضي: ينهي الطابور بالجواهر وفق السعر الصادر من الخادم. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SpeedupQueue(const FString& QueueId);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void FinishQueueWithGems(const FString& QueueId);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UseSpeedupItem(const FString& QueueId, const FString& ItemId);

	// -----------------------------------------------------------------------
	// P19-T5: الحقيبة.
	//
	// لم يكن للحقيبة مسار في العميل ولا في الخادم: `HandleItemsAction` توست
	// «قيد التجهيز»، و`/v1/shop/catalog` يعيد `{ item_id: count }` بلا اسم ولا
	// أيقونة ولا فئة.
	// -----------------------------------------------------------------------

	/** يسحب الحقيبة من `GET /v1/items/bag` ويبثها على `OnBagUpdated`. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Items")
	void FetchBag();

	/** آخر لقطة حقيبة وصلت (فارغة قبل أول جلب — `bLoaded` يفصلهما). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Items")
	const FRok2BagState& GetBag() const { return BagState; }

	/**
	 * يستخدم عنصر تسريع من الحقيبة على طابور — نفس `UseSpeedupItem` لكنه يعيد
	 * قراءة الحقيبة بعده، فينقص العدد على الشاشة بلا إعادة فتح.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Items")
	void UseBagItemOnQueue(const FString& ItemId, const FString& QueueId);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void CollectCityProduction();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Train(const FString& UnitId, int32 Count);

	/** يبدأ شفاء جرحى خطيرين (P4-T4 — backend P2-T2) + صوت HealComplete. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void HealWounded(const TMap<FString, int32>& Troops);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void CreateAlliance(const FString& Name, const FString& Tag);

	/** يطلب وضع منشأة تحالف على الخريطة؛ الخادم يتحقق من الرتبة والإقليم والمسافة والسقف. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance Structures")
	void BuildAllianceStructure(const FString& StructureKind, double X, double Y);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RefreshWorld();

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AttackPass(const FString& PassId, const TMap<FString, int32>& Troops);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void MarchTo(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& Troops);
	
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void DispatchMarch(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& Troops, const FString& PrimaryCommander, const FString& SecondaryCommander);

	/** يغيّر وجهة مسيرة اللاعب المتحركة؛ الخادم يعيد حساب الموضع ووقت الوصول ولا يغيّر القوات. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|World")
	void RedirectMarch(const FString& MarchId, const FString& TargetType, const FString& TargetId, double ToX, double ToY);

	/** يرسل كشافة لنقطة على الخريطة (P5-T5) */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SendScout(double ToX, double ToY);

	/** ينشئ رالي تحالف على ممر أو عرش؛ الخادم يفرض الرتبة والهدف والقوات. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void LaunchAllianceRally(const FString& TargetType, const FString& TargetId, const TMap<FString, int32>& Troops, const FString& PrimaryCommanderId);

	/** ينضم اللاعب إلى رالي قيد التجميع بقواته المنزلية فقط. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void JoinAllianceRally(const FString& RallyId, const TMap<FString, int32>& Troops);

	/** يسحب الراليات النشطة للتحالف الحالي من المصدر السلطوي. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void FetchAllianceRallies();

	/** يسترد سجل تقارير القتال المرئي للاعب من الخادم السلطوي. */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Combat")
	void FetchBattleReports();

	// P6-T6: إرسال رسالة دردشة عبر WebSocket
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SendChat(const FString& Channel, const FString& Text);

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

	/**
	 * يعيد سحب الحالة السلطوية الخاصة باللاعب بعد عودة WebSocket: المدينة، لقطة
	 * العالم، التقارير، القادة، وراليات التحالف. آمن للتكرار ولا يغيّر حالة لعب محلية.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Connection")
	void RestoreAuthoritativeState();

	// ---------------------------------------------------------------------------
	// P8-T7: مواهب القادة — شجرة قابلة للتمرير بنقاط تُخصص من الشاشة.
	// ---------------------------------------------------------------------------
	/** يسحب قادة اللاعب (مع مواهبهم ومعداتهم) من GET /v1/commanders ويبث موهبة القائد على OnCommanderTalents */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Talents")
	void FetchTalents(const FString& CommanderId);

	/** يخصص نقاط موهبة لعقدة — POST /v1/commander/talent/allocate {commanderId, nodeId, points} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Talents")
	void AllocateTalent(const FString& CommanderId, const FString& NodeId, int32 Points = 1);

	/** يعيد توزيع كل نقاط مواهب القائد — POST /v1/commander/talent/reset (استرجاع reset_refund_ratio) */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Talents")
	void RespecTalents(const FString& CommanderId);

	// ---------------------------------------------------------------------------
	// P8-T7: معدات القائد (Blacksmith) — 6 خانات + تصنيع + دمج للترقية.
	// ---------------------------------------------------------------------------
	/** يسحب معدات القائد (الخانات الست المجهزة) من GET /v1/commander/equipment ويبثها على OnEquipmentUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Equipment")
	void FetchEquipment(const FString& CommanderId);

	/** يصنع قطعة في خانة بجودة — POST /v1/commander/equipment/craft {commanderId, slot, quality} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Equipment")
	void CraftEquipment(const FString& CommanderId, const FString& Slot, const FString& Quality);

	/** يجهّز قطعة من المخزون في خانتها — POST /v1/commander/equipment/equip {commanderId, itemId} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Equipment")
	void EquipItem(const FString& CommanderId, const FString& ItemId);

	/** يخلع قطعة مجهزة لتعود للمخزون — POST /v1/commander/equipment/unequip {commanderId, slot} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Equipment")
	void UnequipItem(const FString& CommanderId, const FString& Slot);

	/** يدمج 4 قطع متطابقة للترقية إلى الجودة التالية — POST /v1/commander/equipment/merge {commanderId, itemIds} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Equipment")
	void MergeItems(const FString& CommanderId, const TArray<FString>& ItemIds);

	// ---------------------------------------------------------------------------
	// P8-T7: حماية المدينة (دروع AP) والتهجير.
	// ---------------------------------------------------------------------------
	/** يسحب خيارات الدروع وحالة AP للمدينة — /v1/ap/state ويبثها على OnShieldOptions + OnApStateChanged */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Shield")
	void FetchShieldOptions();

	/** يفعّل درع حماية على المدينة — POST /v1/shield/activate {duration_minutes} (خصم gems وفق المدة) */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Shield")
	void ActivateShield(int32 DurationMinutes);

	/** يهجّر المدينة: Mode = "random" (spawn عشوائي في Zone 1) أو "targeted" (إلى ToX/ToY) — POST /v1/city/relocate */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Shield")
	void RelocateCity(const FString& Mode, double ToX = 0, double ToY = 0);

	// ---------------------------------------------------------------------------
	// P8-T7: المهام اليومية والجوائز + ملك المملكة.
	// ---------------------------------------------------------------------------
	/** يسحب المهام اليومية والأسبوعية ونقاطها — /v1/quests ويبثها على OnQuestsUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Quests")
	void FetchQuests();

	/** يطالب بجائزة مهمة مكتملة — /v1/quests/claim */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Quests")
	void ClaimQuest(const FString& QuestId);

	/** يبدّل المفتاح الذهبي اليومي (100 نقطة يومية → 200 جوهرة) */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Quests")
	void RedeemGoldenKey();

	/** يبدّل الصندوق الأسبوعي (300 نقطة أسبوعية → 500 جوهرة + 2 مسرّع ساعة) */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Quests")
	void RedeemWeeklyChest();

	/** يسحب الملك الحالي عبر تحديث لقطة العالم (snapshot يحوي king + throne) ويبث OnKingUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|World")
	void FetchKing();

	/** ينشئ مسيرة نحو موقع مقدس (P8-T4/P8-T5: targetType=holy_site مع فحص unlock) */
	UFUNCTION(BlueprintCallable, Category = "Rok2|HolySites")
	void MarchToHolySite(const FString& SiteId, const FString& PrimaryCommander, const FString& SecondaryCommander);

	// ---------------------------------------------------------------------------
	// P9-T7: تقنية التحالف — بحث جماعي بباڤات لكل الأعضاء (من sim/alliance_tech).
	// ---------------------------------------------------------------------------
	/** يسحب تقنية التحالف من GET /v1/alliance/tech ويبثها على OnAllianceTechUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void FetchAllianceTech();

	/** يتبرع بنقاط بحث في تقنية — POST /v1/alliance/tech/donate {techId, points} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void DonateAllianceTech(const FString& TechId, int32 Points);

	// ---------------------------------------------------------------------------
	// P9-T7: أراضي التحالف ومراكز الموارد (من sim/territory).
	// ---------------------------------------------------------------------------
	/** يسحب حالة أرض التحالف من GET /v1/territory/state ويبثها على OnAllianceTerritoryUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void FetchAllianceTerritory();

	// ---------------------------------------------------------------------------
	// P9-T7: متجر التحالف والألقاب (من sim/alliance_shop).
	// ---------------------------------------------------------------------------
	/** يسحب رصيد متجر التحالف والعناصر من GET /v1/alliance/shop-state ويبثها على OnAllianceShopUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void FetchAllianceShop();

	/** يشتري عنصرًا من رصيد التحالف — POST /v1/alliance/shop/purchase {itemId} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void PurchaseAllianceShopItem(const FString& ItemId);

	// ---------------------------------------------------------------------------
	// P9-T7: نظام VIP (15 مستوى — من sim/shop/vip).
	// ---------------------------------------------------------------------------
	/** يسحب حالة VIP من GET /v1/vip/status (منح يومي تلقائي) ويبثها على OnVipStatusUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|VIP")
	void FetchVipStatus();

	// ---------------------------------------------------------------------------
	// P9-T7: Trading Post — سوق تبادل موارد بين لاعبي المملكة (من sim/trading).
	// ---------------------------------------------------------------------------
	/** يسحب عروض التداول النشطة من GET /v1/trading/list ويبثها على OnTradingOffersUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Trading")
	void FetchTradingOffers();

	/** ينشر عرض تبادل — POST /v1/trading/offer {sellResource, buyResource, amount} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Trading")
	void PostTradingOffer(const FString& SellResource, const FString& BuyResource, int32 Amount);

	/** يشتري عرض تداول قائمًا — POST /v1/trading/claim {offerId} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Trading")
	void ClaimTradingOffer(const FString& OfferId);

	// ---------------------------------------------------------------------------
	// P9-T7: صناديق هدايا التحالف الجماعية (من sim/alliance_gifts).
	// ---------------------------------------------------------------------------
	/** يسحب الصناديق النشطة للتحالف من GET /v1/alliance/gifts/list ويبثها على OnAllianceGiftsUpdated */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void FetchAllianceGifts();

	/** يفتح فتحة في صندوق هدية تحالف — POST /v1/alliance/gifts/claim {giftId} */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Alliance")
	void ClaimAllianceGift(const FString& GiftId);
	// P10-T6: أوضاع اللعب المتكررة.
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void FetchTavernState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void OpenTavernBox(const FString& BoxId);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void FetchExpeditionState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void AttemptExpeditionBattle(const FString& StageId, const TArray<int32>& TroopCounts);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void FetchCanyonState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void CreateCanyonChallenge();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void CompleteCanyonChallenge(const FString& ChallengeId, int32 Stars);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void ActivateCanyonBuff(const FString& BuffId);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void FetchOsirisState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void RegisterOsiris(const FString& Team);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void AttackOsirisFacility(const FString& FacilityId);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void MoveOsirisArk(const FString& FacilityId);
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void FetchEventsState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void SpinWheel();
	UFUNCTION(BlueprintCallable, Category = "Rok2|P10")
	void SubmitMGScore(double Points);

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

	UFUNCTION(BlueprintPure, Category = "Rok2|Alliance")
	const TArray<FRok2AllianceRally>& GetAllianceRallies() const { return AllianceRallies; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	const FRok2GameMeta& GetMeta() const { return Meta; }

	// P8-T7: وصول الكاشات للواجهات الجديدة.
	UFUNCTION(BlueprintPure, Category = "Rok2|Talents")
	const FRok2CommanderTalents& GetCommanderTalents() const { return CommanderTalents; }

	UFUNCTION(BlueprintPure, Category = "Rok2|Equipment")
	const TArray<FRok2EquipmentSlot>& GetEquipmentSlots() const { return EquipmentSlots; }

	UFUNCTION(BlueprintPure, Category = "Rok2|Equipment")
	const TArray<FRok2EquipmentItem>& GetEquipmentInventory() const { return EquipmentInventory; }

	UFUNCTION(BlueprintPure, Category = "Rok2|Quests")
	const FRok2QuestState& GetQuestState() const { return QuestState; }

	UFUNCTION(BlueprintPure, Category = "Rok2|World")
	const FRok2ActionPointState& GetApState() const { return World.ApState; }

	UFUNCTION(BlueprintPure, Category = "Rok2|World")
	const FRok2KingMarker& GetKing() const { return World.King; }

	/** إشعارات الـ HUD المخزنة (الأحدث أولاً، حد أقصى 20) — P2-T6 */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2HudNotification>& GetNotifications() const { return Notifications; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	int32 GetUnreadNotificationsCount() const { return UnreadNotifications; }

	/** يصفّر عدّاد غير المقروء (عند فتح مركز الإشعارات) — P2-T6 */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void MarkNotificationsRead() { UnreadNotifications = 0; }

	// P6-T6: الدردشة الحية
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2ChatMessage>& GetChatHistory() const { return ChatHistory; }

	UFUNCTION(BlueprintPure, Category = "Rok2")
	int32 GetUnreadChatCount() const { return UnreadChatCount; }

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void MarkChatRead() { UnreadChatCount = 0; }

	/** حكاية الموسم المحفوظة من آخر لقطة، وتبقى متاحة قبل فتح الودجة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Season Story")
	const TArray<FRok2SeasonStoryEntry>& GetSeasonStory() const { return World.SeasonStory; }

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
	FOnCommandersLoaded OnCommandersLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Research")
	FOnResearchLoaded OnResearchLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnZonesUpdated OnZonesUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnHudNotification OnHudNotification;

	// P6-T6: يُبث عند وصول رسالة دردشة جديدة
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnChatMessage OnChatMessage;

	// P7-T1: حدث حكاية موسم حي، منفصل عن التقارير والرسائل الخاصة.
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Season Story")
	FOnSeasonStoryEvent OnSeasonStoryEvent;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceRalliesUpdated OnAllianceRalliesUpdated;

	// P8-T7: أحداث أنظمة القادة العميقة والحماية والمهام والملك.
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Talents")
	FOnCommanderTalents OnCommanderTalents;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Equipment")
	FOnEquipmentUpdated OnEquipmentUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Shield")
	FOnShieldOptions OnShieldOptions;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Shield")
	FOnApStateChanged OnApStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Quests")
	FOnQuestsUpdated OnQuestsUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|World")
	FOnKingUpdated OnKingUpdated;

	// P9-T7: أحداث النسيج الاجتماعي والاقتصادي.
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceTechUpdated OnAllianceTechUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceTerritoryUpdated OnAllianceTerritoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceShopUpdated OnAllianceShopUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceTitleChanged OnAllianceTitleChanged;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|VIP")
	FOnVipStatusUpdated OnVipStatusUpdated;

	/** P19-T5: تُبثّ عند وصول لقطة الحقيبة. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Items")
	FOnBagUpdated OnBagUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Trading")
	FOnTradingOffersUpdated OnTradingOffersUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Rok2|Alliance")
	FOnAllianceGiftsUpdated OnAllianceGiftsUpdated;
	// P10-T6: أحداث أوضاع اللعب المتكررة.
	UPROPERTY(BlueprintAssignable, Category = "Rok2|P10")
	FOnTavernUpdated OnTavernUpdated;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|P10")
	FOnExpeditionUpdated OnExpeditionUpdated;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|P10")
	FOnCanyonUpdated OnCanyonUpdated;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|P10")
	FOnOsirisUpdated OnOsirisUpdated;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|P10")
	FOnEventsUpdated OnEventsUpdated;

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
	FRok2ResearchState ResearchState;
	TArray<FRok2BattleReport> BattleReports;
	TArray<FRok2AllianceRally> AllianceRallies;
	FRok2GameMeta Meta;
	// ---- P13-T2: كاش محلي لبيانات التوازن — أول فتح بلا انتظار كامل ----
	FString MetaCacheFileName = TEXT("rok2_meta_cache.json");
	bool bMetaCacheLoaded = false;
	/** يحوّل اسم ملف الكاش إلى مسار كامل في GameDir/Saved. */
	FString MetaCachePath() const;
	/** يحفظ بيانات التوازن الحالية إلى ملف الكاش في GameDir (بعد FetchMeta). */
	void SaveMetaCache();
	/** يحمل بيانات التوازن من ملف الكاش إن وُجد (يُنادى من Init). */
	void LoadMetaCache();

	// P8-T7: كاشات أنظمة القادة العميقة والحماية والمهام — تُحدّث من Fetch* وتُبث للواجهات.
	FRok2CommanderTalents CommanderTalents;
	TArray<FRok2EquipmentSlot> EquipmentSlots;
	TArray<FRok2EquipmentItem> EquipmentInventory;
	FRok2QuestState QuestState;
	bool bKingKnown = false;

	// P9-T7: كاشات النسيج الاجتماعي والاقتصادي — تُحدّث من Fetch* وتُبث للواجهات.
	TArray<FRok2AllianceTechNode> AllianceTechNodes;
	FRok2AllianceTerritoryState TerritoryState;
	TArray<FRok2AllianceShopItem> AllianceShopItems;
	int32 AllianceShopBalance = 0;
	FRok2VipStatus VipStatus;
	/** P19-T5: آخر لقطة حقيبة من الخادم. */
	FRok2BagState BagState;
	TArray<FRok2TradingOffer> TradingOffers;
	TArray<FRok2AllianceGift> AllianceGifts;
	// P10-T6: كاشات حالة أوضاع اللعب المتكررة.
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|P10")
	FRok2TavernState TavernState;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|P10")
	FRok2ExpeditionState ExpeditionState;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|P10")
	FRok2CanyonState CanyonState;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|P10")
	FRok2OsirisState OsirisState;
	UPROPERTY(BlueprintReadOnly, Category = "Rok2|P10")
	FRok2EventsState EventsState;

	// ---- HUD الموحد (P2-T6) ----
	/** سجل الإشعارات (الأحدث أولاً) */
	TArray<FRok2HudNotification> Notifications;
	int32 UnreadNotifications = 0;
	int32 NotificationSeq = 0;
	/** يضيف إشعاراً ويبثه للـ HUD */
	void PushNotification(const FString& Kind, const FString& Title, const FString& Body, float TtlSeconds = 6.f);

	// ---- P6-T6: الدردشة الحية ----
	/** سجل رسائل الدردشة */
	TArray<FRok2ChatMessage> ChatHistory;
	int32 UnreadChatCount = 0;
	/** يضيف رسالة دردشة ويبثها */
	void PushChatMessage(const FRok2ChatMessage& Msg);

	/** يزيل التكرار، يخزن حدث الموسم ضمن لقطة العالم ويبثه للواجهة. */
	void PushSeasonStoryEvent(const FRok2SeasonStoryEntry& Event);

	/**
	 * P6-T5: يُلقي تحية الحضارة مرة واحدة في الجلسة عبر نظام الإشعارات.
	 * يُنادى من ParseCity — أول لحظة تكون فيها حضارة اللاعب معروفة من الخادم.
	 */
	void MaybeGreetCiv();

	/** هل أُلقيت تحية الحضارة في هذه الجلسة؟ راية جلسة لا حالة محفوظة. */
	bool bCivGreetingShown = false;

	/**
	 * مدة بقاء بطاقة التحية. أطول من الافتراضي (6s) لأنها جملة أدبية تُقرأ لا
	 * سطر حالة يُلمح — والقراءة العربية لجملة كاملة لا تتمّ في ست ثوانٍ مع
	 * انصراف العين إلى المدينة الجديدة.
	 */
	static constexpr float CivGreetingTtlSeconds = 8.f;

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> PendingRequest;
	TSharedPtr<IWebSocket> WebSocket;
	float WorldPollTimer = 0.f;
	bool bWsConnected = false;

	// ---- P7-T5: استعادة الحالة بعد إعادة الاتصال ----
	/** لا تُستعاد الحالة في أول اتصال؛ تُطلق حصراً بعد انقطاع حي سابق. */
	bool bRestoreOnNextWsConnection = false;
	/** يمنع إشارة قديمة من WS متصل سابقاً من إطلاق استعادة موازية. */
	bool bStateRestoreInFlight = false;
	int32 StateRestorePendingRequests = 0;

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
	// ---- P13-T1: صندوق واردات WebSocket — الرسائل المرسلة قبل الاتصال لا تضيع ----
	TArray<FString> WsOutbox;
	/** يتأكد من أن الرسالة صالحة JSON قبل إضافتها، أو يحفظ النص خامًا. */
	void EnqueueWsMessage(const FString& JsonMessage);
	/** يفرّغ كل الرسائل المتراكمة فور اتصال الـ WebSocket. */
	void FlushWsOutbox();
	// ---- P13-T3: نبض WebSocket + watchdog للانقطاع الصامت ----
	float WsHeartbeatTimer = 0.f;
	float WsLastMessageAt = 0.f;
	/** الفاصل الزمني لنبض القلب (ثانية) */
	static constexpr float WsHeartbeatIntervalSeconds = 30.f;
	/** حد الانقطاع الصامت قبل إعادة الاتصال الإجبارية (ثانية) */
	static constexpr float WsSilentDisconnectThresholdSeconds = 90.f;
	void SendWsHeartbeat();
	/** أقصى عدد محاولات إعادة لطلبات القراءة عند أخطاء الشبكة */
	static constexpr int32 HttpMaxRetries = 2;

	void SetOnline(bool bNewOnline, const FString& Reason);
	void CompleteAuthoritativeStateRestore();
	void FetchCommandersInternal(TFunction<void()> OnFinished);
	void FetchBattleReportsInternal(TFunction<void()> OnFinished);
	void FetchAllianceRalliesInternal(TFunction<void()> OnFinished);

	void Get(const FString& Path, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr = nullptr);
	void Post(const FString& Path, const FString& JsonBody, bool bAuth, TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr = nullptr);

	/** HTTP داخلي مع retry backoff لأخطاء الشبكة (لا يعيد المحاولة على أخطاء 4xx المنطقية) */
	void RequestWithRetry(const FString& Verb, const FString& Path, const FString& JsonBody, bool bAuth,
		TFunction<void(const TSharedPtr<FJsonObject>&)> OnOk, TFunction<void(const FString&)> OnErr, int32 MaxRetries);

	void ParsePlayer(const TSharedPtr<FJsonObject>& Obj);
	// P10-T6: دوال Parse لحالة أوضاع اللعب المتكررة.
	void ParseTavernState(const TSharedPtr<FJsonObject>& Obj);
	void ParseExpeditionState(const TSharedPtr<FJsonObject>& Obj);
	void ParseCanyonState(const TSharedPtr<FJsonObject>& Obj);
	void ParseOsirisState(const TSharedPtr<FJsonObject>& Obj);
	void ParseEventsState(const TSharedPtr<FJsonObject>& Obj);
	void ParseCity(const TSharedPtr<FJsonObject>& Obj);
	void ParseWorld(const TSharedPtr<FJsonObject>& Obj);

	/** يحوّل كائن march من JSON إلى FRok2MarchEntity (P1-T3) */
	void ParseMarchEntity(const TSharedPtr<FJsonObject>& M, FRok2MarchEntity& E) const;

	/** يحوّل كائن scout من JSON إلى FRok2ScoutEntity (P5-T5) */
	void ParseScoutEntity(const TSharedPtr<FJsonObject>& S, FRok2ScoutEntity& E) const;

	/** يحوّل رالي التحالف من استجابة /v1/alliance/rallies إلى نموذج الواجهة. */
	void ParseAllianceRally(const TSharedPtr<FJsonObject>& R, FRok2AllianceRally& E) const;

	/** يحوّل عقدة موهبة من JSON إلى FRok2TalentNode (P8-T7) */
	void ParseTalentNode(const TSharedPtr<FJsonObject>& Obj, FRok2TalentNode& Out) const;

	/** يحوّل قطعة معدات من JSON إلى FRok2EquipmentItem (P8-T7) */
	void ParseEquipmentItem(const TSharedPtr<FJsonObject>& Obj, FRok2EquipmentItem& Out) const;

	/** يحوّل مهمة يومية/أسبوعية من JSON إلى FRok2DailyQuest (P8-T7) */
	void ParseQuest(const TSharedPtr<FJsonObject>& Obj, FRok2DailyQuest& Out) const;

	/** يحدّث World.King ويبث OnKingUpdated (P8-T7) */
	void UpsertKing(const FRok2KingMarker& K);

	/** يستخرج خيارات الدروع من GET /v1/ap/state ويبثها (P8-T7) */
	void ParseShieldState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج المهام اليومية والأسبوعية من GET /v1/quests ويبثها (P8-T7) */
	void ParseQuestState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج حالة الملك من GET /v1/meta/all ويبثها (P8-T7) */
	void ParseKingState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج عقد تقنية التحالف من GET /v1/alliance/tech ويبثها (P9-T7) */
	void ParseAllianceTechState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج حالة أرض التحالف من GET /v1/territory/state ويبثها (P9-T7) */
	void ParseAllianceTerritoryState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج رصيد متجر التحالف وعناصره من GET /v1/alliance/shop-state ويبثها (P9-T7) */
	void ParseAllianceShopState(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج حالة VIP من GET /v1/vip/status ويبثها (P9-T7) */
	void ParseVipStatus(const TSharedPtr<FJsonObject>& Obj);
	/** P19-T5: يقرأ فئات الحقيبة وعناصرها من استجابة `/v1/items/bag`. */
	void ParseBag(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج عروض التداول من GET /v1/trading/list ويبثها (P9-T7) */
	void ParseTradingOffers(const TSharedPtr<FJsonObject>& Obj);

	/** يستخرج صناديق هدايا التحالف من GET /v1/alliance/gifts/list ويبثها (P9-T7) */
	void ParseAllianceGifts(const TSharedPtr<FJsonObject>& Obj);

	/** يضيف/يحدّث/يزيل مسيرة في World.Marches ويبث التحديث (من أحداث الـ WS) */
	void UpsertMarch(const FRok2MarchEntity& E);

	/** يحوّل خريطة troops JSON إلى قائمة خسائر مرتبة (P1-T4) */
	static void ParseTroopMap(const TSharedPtr<FJsonObject>& Obj, TArray<FRok2TroopLoss>& Out);
	/** يحوّل تقرير قتال من JSON إلى FRok2BattleReport */
	void ParseBattleReport(const TSharedPtr<FJsonObject>& Obj, FRok2BattleReport& Out) const;
	/** يضيف تقريراً في مقدمة القائمة (حد أقصى 25) ويبث التحديث */
	void AddBattleReport(const FRok2BattleReport& R);

	/**
	 * ينفّذ طلباً واحداً مع إعادة المحاولة. عضو ساكن لا دالة حرة:
	 * يقرأ حالة محمية (BuildUrl/AuthHeader/SetOnline/EmitError/HttpTimeoutSeconds)،
	 * ودالة حرة غير friend لا تملك هذا الوصول.
	 */
	static void Rok2SendRequest(URok2Api* Self, TSharedPtr<FRok2RetryCtx> Ctx);

	FString AuthHeader() const;
	FString BuildUrl(const FString& Path) const;
	void EmitError(const FString& Msg) { OnApiError.Broadcast(Msg); }
	// P11-T6: Lost Kingdom / KvK
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void FetchLostKingdomState();
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void MigrateToLostKingdom();
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void CaptureHieron(const FString& HieronId);
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void AttackCitadel(const FString& CitadelId, int32 Damage);
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void AttackZiggurat(int32 Damage);
	UFUNCTION(BlueprintCallable, Category = "Rok2|KvK")
	void BuySeasonItem(const FString& ItemId);

	// P11-T6
	UPROPERTY(BlueprintReadOnly)
	FRok2LostKingdomState LostKingdomState;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|KvK")
	FOnLostKingdomUpdated OnLostKingdomUpdated;

	// P11-T6
	void ParseLostKingdomState(const TSharedPtr<FJsonObject>& Json);

	// P12-T6: نهاية الموسم وإعادة الضبط
	UFUNCTION(BlueprintCallable, Category = "Rok2|Season")
	void FetchSeasonReport();
	UFUNCTION(BlueprintPure, Category = "Rok2|Season")
	FRok2SeasonState GetSeasonState() const;
	// P12-T6
	UPROPERTY(BlueprintReadOnly)
	FRok2SeasonReport SeasonReport;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Season")
	FOnSeasonReportUpdated OnSeasonReportUpdated;
	UPROPERTY(BlueprintAssignable, Category = "Rok2|Season")
	FOnSeasonEnded OnSeasonEnded;
	// P12-T6
	void ParseSeasonReport(const TSharedPtr<FJsonObject>& Json);
};
