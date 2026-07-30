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
};

USTRUCT(BlueprintType)
struct FRok2City
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 HallLevel = 1;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2Resources Resources;
	/** معدلات الإنتاج في الساعة — تحسب من المباني لعرض العدّاد الحي (P1-T5) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FRok2Resources Rates;
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 UpdatedAt = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2QueueEntry> ActiveQueues;
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

	/** حالة قفل/فتح المناطق (P2-T4) — يملؤه الـ HUD لعرض مؤقت المناطق (P2-T6) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2ZoneStatus> Zones;

	/** الكشافة النشطة على الخريطة (P5-T5) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2ScoutEntity> Scouts;
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
