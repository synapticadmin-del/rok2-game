// Copyright ROK2. Shared types / structs mirroring Cloudflare API.

#pragma once

#include "CoreMinimal.h"
#include "Rok2Types.generated.h"

USTRUCT(BlueprintType)
struct FRok2Resources
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Food = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Wood = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Stone = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Gold = 0;
};

USTRUCT(BlueprintType)
struct FRok2Player
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Civ;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RegionId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Power = 0;
};

USTRUCT(BlueprintType)
struct FRok2QueueEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Type; // "building", "training", etc.
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RefId; // e.g. "castle", "infantry_t1"
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 StartMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 EndMs = 0;
	/** زمن متبقٍ صادر من الخادم؛ يعاد حسابه دورياً فقط للعرض. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 RemainingSeconds = 0;
	/** سعر إتمام الطابور بالجواهر، محسوب في الخادم من كتالوج التسريعات. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 FinishCostGems = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString State;
};

USTRUCT(BlueprintType)
struct FRok2CityLayoutPlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	FString BuildingId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	int32 Q = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	int32 R = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	int32 RotationSteps = 0;
	/** standard / ceremonial / fortified؛ قيمة تجميلية صادرة من الخادم. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	FString Facade = TEXT("standard");
};

USTRUCT(BlueprintType)
struct FRok2City
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 HallLevel = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2Resources Resources;
	/** رصيد عملة التسريع داخل اللعبة؛ يُحدَّث من الخادم فقط. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Gems = 0;
	/** معدلات الإنتاج في الساعة — يرسلها الخادم بعد تطبيق المباني والأبحاث وVIP. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2Resources Rates;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 UpdatedAt = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2QueueEntry> ActiveQueues;
	/** تخطيط القلعة المعتمد على الخادم؛ فارغ عند أول دخول أو تعذر القراءة. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	TArray<FRok2CityLayoutPlacement> LayoutPlacements;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	int32 LayoutVersion = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|City Layout")
	int64 LayoutUpdatedAt = 0;
};

USTRUCT(BlueprintType)
struct FRok2TroopEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString UnitId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRok2PassEntity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString OwnerAllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double CaptureProgress = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString State;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString From;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString To;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 UnlockDay = 0;
};

USTRUCT(BlueprintType)
struct FRok2CityEntity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 HallLevel = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RegionId;
};

USTRUCT(BlueprintType)
struct FRok2MarchEntity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString OwnerPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromX = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromY = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToX = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToY = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 StartMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 EtaMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString State;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetType;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TMap<FString, int32> Troops;
	/** نوع الحمولة لمسيرات الجمع (food/wood/stone/gold) — يملؤه ParseMarchEntity من m.payload.kind */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind;

	/**
	 * P7-T10: الفرع الأبرز للمسيرات (infantry/cavalry/archer/siege) المشتق من
	 * Troops map؛ يقود اختيار أيقونة المسيرة من حزمة WorldMapIcons.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Branch = TEXT("infantry");
};

USTRUCT(BlueprintType)
struct FRok2NodeEntity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Remaining = 0;
};

/** كشافة واحدة على الخريطة (P5-T5) */
/** منشأة تحالف ثابتة تُبث من الخادم السلطوي مع نطاقها الفعلي. */
USTRUCT(BlueprintType)
struct FRok2AllianceStructure
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Radius = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ProtectionRadius = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double MarchDamageReduction = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString MapMarker;
};

USTRUCT(BlueprintType)
struct FRok2ScoutEntity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString OwnerPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromX = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromY = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToX = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToY = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 StartMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 EtaMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString State; // "marching" / "arrived" / "returned"
};

// P6-T6: رسالة دردشة حية (قناة المملكة أو التحالف)
USTRUCT(BlueprintType)
struct FRok2ChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Channel; // "kingdom" / "alliance"
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerName;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Civ;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Text;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 TimestampMs = 0;
};

/**
 * معلم عام في الموسم مصدره KingdomShard. لا يحمل تفاصيل قتالية خاصة؛
 * يُستخدم في snapshot.seasonStory وحدث WS من النوع season_story_event.
 */
USTRUCT(BlueprintType)
struct FRok2SeasonStoryEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	int32 SeasonDay = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	int64 CreatedAt = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	FString SubjectId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	FString PreviousAllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	int32 Score = 0;
};

/** حالة فتح/قفل منطقة واحدة — من snapshot.zones (P2-T4) */
USTRUCT(BlueprintType)
struct FRok2ZoneStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 ZoneId = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RegionId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bUnlocked = true;
	/** يوم الفتح من الموسم (0 = مفتوحة دائماً) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 UnlockDay = 0;
};

/** رالي تحالف نشط كما يقدمه الخادم السلطوي للعضو الحالي. */
USTRUCT(BlueprintType)
struct FRok2AllianceRally
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString LeaderPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetType;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Status;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 StartMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 LaunchMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString MarchId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Participants = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bIsJoined = false;
};

USTRUCT(BlueprintType)
struct FRok2WorldSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 SeasonDay = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2CityEntity> Cities;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2PassEntity> Passes;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2MarchEntity> Marches;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2NodeEntity> Nodes;

	/** خط زمني عام للموسم من KingdomShard (P6-T10؛ يصل فعلياً في P7-T1). */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|Season Story")
	TArray<FRok2SeasonStoryEntry> SeasonStory;

	/** منشآت التحالف المرئية ونطاقات الحماية الخاصة بها. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2AllianceStructure> AllianceStructures;

	/** حالة قفل/فتح المناطق (P2-T4) — يملؤه الـ HUD لعرض مؤقت المناطق (P2-T6) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2ZoneStatus> Zones;

	/** الكشافة النشطة على الخريطة (P5-T5) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2ScoutEntity> Scouts;

	/** موقع الملك الحالي وتوقيت تتويجه (P8-T7؛ يملؤه ParseWorld من snapshot وmeta/all) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2KingMarker King;
	/** المواقع المقدسة المحتلّة حاليًا (P8-T7؛ يملؤه ParseWorld من meta/holy-sites) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> CapturedSiteIds;
	/** آخر حالة AP للمدينة (P8-T7؛ يملؤها FetchApState) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2ActionPointState ApState;
};

USTRUCT(BlueprintType)
struct FRok2Civilization
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Fantasy;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString SpecialUnit;

	/**
	 * P6-T5: الاسم والفانتازي بالعربية — من name_ar/fantasy_ar في
	 * data/civilizations.json. الحقلان أعلاه يحملان اللاتيني كما تخدمه البيانات
	 * (وهو ما يُرسَل للخادم ويُسجَّل في اللوجات)، وهذان ما يُعرض للاعب.
	 *
	 * محمولان في البنية لا مأخوذان من URok2CivLore عند كل عرض: هذه البنية هي
	 * سجلّ الحضارة عند العميل، فإرسال كل مستهلك لها إلى سجلٍّ ثانٍ ليعرف اسمها
	 * يجعل «الاسم» بلا مالك واضح.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString NameAr;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString FantasyAr;

	/** الاسم المعروض: العربي إن وُجد وإلا اللاتيني — فلا صفٌّ فارغ في قائمة */
	FString DisplayName() const { return NameAr.IsEmpty() ? Name : NameAr; }
};

USTRUCT(BlueprintType)
struct FRok2MapRegion
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 ZoneId = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<double> Aabb;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString SpawnAnchor;
};

USTRUCT(BlueprintType)
struct FRok2Commander
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Rarity;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Nation;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> Tags;
	/** المستوى والخبرة والتومات مصدرها /v1/commanders؛ لا تُحسب في العميل. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Xp = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 XpToNext = 1000;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Tomes = 0;
	/** ترتيب مستويات المهارات يطابق تعريف القائد في البيانات الموحدة. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<int32> SkillLevels;
};


USTRUCT(BlueprintType)
struct FRok2TroopLoss
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString UnitId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Count = 0;
};

USTRUCT(BlueprintType)
struct FRok2BattleSide
{
	GENERATED_BODY()

	/** خسائر كل وحدة */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Losses;
	/** المتبقي بعد المعركة */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Remaining;
	/** تقسيم الخسائر: قتلى */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Dead;
	/** جرحى خطيرين (مستشفى) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Severely;
	/** جرحى خفيفين */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Slightly;
	/** القوة قبل المعركة */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 PowerBefore = 0;
};

USTRUCT(BlueprintType)
struct FRok2RallyReportParticipant
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Committed;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Remaining;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Losses;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Dead;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Severely;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> Slightly;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> HospitalAdmitted;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TroopLoss> HospitalDied;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 HospitalCapacity = 0;
};

USTRUCT(BlueprintType)
struct FRok2BattleReward
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Amount = 0;
};

USTRUCT(BlueprintType)
struct FRok2BattleReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 CreatedAt = 0;
	/** نوع المعركة: pass_attack / throne_attack / barb */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AttackerPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AttackerAllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PassId;
	/** winner: attacker / defender / draw */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Winner;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2BattleSide Attacker;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2BattleSide Defender;
	/** حقل غير فارغ يعني أن القتال نتج عن رالي تحالف. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RallyId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RallyAllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString RallyLeaderPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2RallyReportParticipant> RallyParticipants;
	/** مكافآت صادرة من الخادم فقط، مثل نقاط الموسم أو أول احتلال. */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2BattleReward> Rewards;
};


USTRUCT(BlueprintType)
struct FRok2TrainableUnit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Branch;
};

USTRUCT(BlueprintType)
struct FRok2BuildingMeta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Category;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Desc;
};

/** بيانات التوازن الموحدة من /v1/meta/all (P1-T6) */
USTRUCT(BlueprintType)
struct FRok2GameMeta
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TrainableUnit> TrainableUnits;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2BuildingMeta> Buildings;
	/** معدلات الإنتاج الأساسية لكل مبنى (من الخادم) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TMap<FString, double> ProductionBase;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ProductionLevelMult = 1.2;
	/** هل تم السحب من الخادم فعلاً؟ (false = قيم fallback محلية) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bLoaded = false;

	/** P8-T7: شجرتا مواهب القادة (troop_type + role) — خزينة مواهب العميل */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	TArray<FRok2TalentTree> TalentTrees;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	int32 TalentPointsPerLevel = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	int32 PointsCapCommon = 40;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	int32 MaxPointsPerNode = 5;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	double ResetRefundRatio = 0.8;

	/** P8-T7: مواصفات الحدادة (Blacksmith) — الخانات وblueprints من الخادم */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	TArray<FString> EquipmentSlots;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P8")
	TArray<FRok2EquipmentBlueprint> EquipmentBlueprints;
};

USTRUCT(BlueprintType)
struct FRok2TechNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Category;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 MaxLevel = 5;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 TimeSeconds = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> Prerequisites;
};

/** إشعار HUD لحظي — يُعرض كبطاقة ثم يتلاشى (P2-T6) */
USTRUCT(BlueprintType)
struct FRok2HudNotification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Title;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Body;
	/** toast | combat | zone | rally */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind = TEXT("toast");
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 CreatedAtUtcMs = 0;
	/** مدة البقاء قبل التلاشي (ثانية) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	float TtlSeconds = 6.f;
};

// ---------------------------------------------------------------------------
// P8-T7: أنظمة القادة العميقة (مواهب/معدات) + حماية + مهام يومية + الملك.
//
// هذه الأنواع تُعرَّض عبر Blueprint (BlueprintType) لتُستخدم مباشرة من
// شاشات UMG الجديدة (Rok2TalentsWidget/Rok2EquipmentWidget/Rok2ShieldWidget/
// Rok2QuestsWidget/Rok2KingWidget) دون أي تحويلات؛ وهي مرآة مطابقة
// لتعريفات الخادم في `data/talents.json` و`data/equipment.json`
// و`data/action_points.json` و`data/daily_quests.json` وKingdomShard.
// ---------------------------------------------------------------------------

/** شجرة مواهب كاملة (troop_type أو role) من GET /v1/meta/talents. */
USTRUCT(BlueprintType)
struct FRok2TalentTree
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TalentNode> Nodes;
};

/** قالب قطعة من GET /v1/meta/equipment (blueprints by slot). */
USTRUCT(BlueprintType)
struct FRok2EquipmentBlueprint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Slot;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 CraftGoldBase = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double CraftGoldQualityMult = 1.0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> StatPool;
};

/** عقدة في شجرة مواهب القائد (من /v1/commanders/talents). */
USTRUCT(BlueprintType)
struct FRok2TalentNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Tree; // war | development | leadership
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 MaxLevel = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> Prerequisites;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TMap<FString, double> StatMods; // atk_def/atk_cav/hp_infantry/...
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 PowerCost = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bUnlocked = false;
};

/** حالة مواهب قائد كامل (مستلمة من /v1/commanders/talents). */
USTRUCT(BlueprintType)
struct FRok2CommanderTalents
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString CommanderId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2TalentNode> Nodes;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 PointsAvailable = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 PointsSpent = 0;
};

/** قطعة معدات للقائد (من /v1/blacksmith/items). */
USTRUCT(BlueprintType)
struct FRok2EquipmentItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Slot; // weapon / helm / chest / gloves / legs / boots
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Rarity; // common / uncommon / rare / epic / legendary
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Material; // iron / steel / mithril / dragonsteel / adamantite
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TMap<FString, double> StatMods;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FString> SetParts;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Level = 1;
};

/** خانة معدات مرفوع فيها قطعة (من /v1/blacksmith/equip). */
USTRUCT(BlueprintType)
struct FRok2EquipmentSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Slot; // weapon / helm / chest / gloves / legs / boots
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bFilled = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2EquipmentItem Item;
};

/** خيار درع حماية للمدينة (من GET /v1/ap/state). */
USTRUCT(BlueprintType)
struct FRok2ShieldOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id; // 8h / 24h / 3d
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 DurationMinutes = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Gems = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Ap = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bAvailable = true;
};

/** حالة نقاط العمل والحماية للمدينة (من GET /v1/ap/state). */
USTRUCT(BlueprintType)
struct FRok2ActionPointState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Ap = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 ApCap = 1000;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 ShieldUntilMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 WarFrenzyUntilMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 LastRelocationMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2ShieldOption> ShieldOptions;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 NowMs = 0;
};

/** مهمة يومية/أسبوعية (من GET /v1/quests). */
USTRUCT(BlueprintType)
struct FRok2DailyQuest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id; // daily_123_0 / weekly_24_0
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Kind; // daily | weekly
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString SourceKey; // train / battle_win / barb_kill / gather / help / research / speedup / build
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Title;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Description;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Goal = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Progress = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bCompleted = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bClaimed = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 PointsReward = 0;
};

/** حالة المهام اليومية والأسبوعية (من GET /v1/quests). */
USTRUCT(BlueprintType)
struct FRok2QuestState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2DailyQuest> DailyQuests;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2DailyQuest> WeeklyQuests;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 DailyPoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 DailyPointsCap = 100;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 WeeklyPoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 WeeklyPointsCap = 300;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bGoldenKeyAvailable = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bWeeklyChestAvailable = false;
};

// ---------------------------------------------------------------------------
// P9-T7: النسيج الاجتماعي والاقتصادي — تحالف حي (تقنية/أرض/متجر/ألقاب)
// واقتصاد متقدم (VIP كامل + Trading Post + صناديق هدايا جماعية).
// الأنواع مرآة لتعريفات الخادم في data/alliance_tech.json وdata/alliance_territory.json
// وdata/alliance_shop.json وdata/shop.json وdata/trading.json وdata/alliance_gifts.json.
// ---------------------------------------------------------------------------

/** عقدة بحث تقنية تحالف (من GET /v1/alliance/tech). */
USTRUCT(BlueprintType)
struct FRok2AllianceTechNode
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Category; // development | territory | war | skill
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 Level = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 MaxLevel = 5;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 Points = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 PointsRequired = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bCompleted = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	double BuffValue = 0;
};

/** حالة أراضي التحالف ومراكز الموارد (من GET /v1/territory/state). */
USTRUCT(BlueprintType)
struct FRok2AllianceTerritoryState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	TArray<FString> CenterIds; // مراكز الموارد المحتلة
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	TArray<FString> OutpostIds; // قلاع Outposts النشطة
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 PatrolModifierPct = 25;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 GatherMultiplierPct = 125;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bInsideTerritory = false;
};

/** عنصر متجر التحالف (من GET /v1/alliance/shop-state). */
USTRUCT(BlueprintType)
struct FRok2AllianceShopItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 Price = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 BoughtCount = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 MaxPerAlliance = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bLocked = false;
};

/** لقب تحالف مطبق على اللاعب (من POST /v1/alliance/shop/grant-title). */
USTRUCT(BlueprintType)
struct FRok2AllianceTitle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	TMap<FString, double> Buffs; // atk_def | research_build | hp | gather
};

/** حالة VIP (من GET /v1/vip/status). */
USTRUCT(BlueprintType)
struct FRok2VipStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 Level = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 Points = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 PointsDailyGranted = 40;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bVipStoreOpen = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	double VipStoreDiscount = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bExtraBuildQueue = false;
};

/** عرض تداول في Trading Post (من GET /v1/trading/list). */
USTRUCT(BlueprintType)
struct FRok2TradingOffer
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString OfferId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString SellerPlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString SellResource; // food | wood | stone | gold
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString BuyResource;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 SellAmount = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 BuyAmount = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	double Rate = 1.0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int64 CreatedAtMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int64 ExpiresAtMs = 0;
};

/** صندوق هدية تحالف جماعي (من GET /v1/alliance/gifts/list). */
USTRUCT(BlueprintType)
struct FRok2AllianceGift
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString GiftId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString GiftTypeId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 SlotsRemaining = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int32 SlotsTotal = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	int64 ExpiresAtMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P9")
	bool bExpired = false;
};

/** موقع الملك الحالي على خريطة العالم (من snapshot وGET /v1/meta/all). */
USTRUCT(BlueprintType)
struct FRok2KingMarker
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString PlayerName;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString AllianceId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double X = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double Y = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 CrownedAtMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 ExpiresAtMs = 0;
};

// P10-T6: أوضاع اللعب المتكررة — الحانة، Expedition، Sunset Canyon، Ark of Osiris، الأحداث الكبرى.
USTRUCT(BlueprintType)
struct FRok2TavernRoll
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString BoxId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Kind; // common | rare | materials | epic | legendary
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Quantity = 0;
};

USTRUCT(BlueprintType)
struct FRok2TavernState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TMap<FString, int32> Keys;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TArray<FRok2TavernRoll> LastRolls;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 OpensThisHour = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	bool DailyKeyClaimed = false;
};

USTRUCT(BlueprintType)
struct FRok2ExpeditionBattleResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString StageId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	bool Victory = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Stars = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 DamageTaken = 0;
};

USTRUCT(BlueprintType)
struct FRok2ExpeditionState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TMap<FString, int32> StageStars;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Medals = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 AttemptsToday = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TArray<FRok2ExpeditionBattleResult> RecentResults;
};

USTRUCT(BlueprintType)
struct FRok2CanyonChallenge
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString SeasonId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 DaySlot = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Stars = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Score = 0;
};

USTRUCT(BlueprintType)
struct FRok2CanyonState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TArray<FRok2CanyonChallenge> Challenges;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 ActiveBuffs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Tokens = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 VictoryPoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString CurrentSeasonId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 SeasonDay = 0;
};

USTRUCT(BlueprintType)
struct FRok2OsirisFacility
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Id;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Name;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 CapturePoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int64 HeldUntilMs = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString OwnerTeam; // red | blue
};

USTRUCT(BlueprintType)
struct FRok2OsirisState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString LeagueId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TArray<FRok2OsirisFacility> Facilities;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 RedPoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 BluePoints = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	bool Registered = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int64 NextArkMoveMs = 0;
};

USTRUCT(BlueprintType)
struct FRok2WheelSpinResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString SlotId;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Kind;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 Quantity = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	bool FreeSpin = false;
};

USTRUCT(BlueprintType)
struct FRok2EventsState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 EventDay = 0;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	FString Phase; // gathering | battle | results
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	bool WheelOpen = false;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	TArray<FRok2WheelSpinResult> RecentSpins;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2|P10")
	int32 MGTotalScore = 0;
};
