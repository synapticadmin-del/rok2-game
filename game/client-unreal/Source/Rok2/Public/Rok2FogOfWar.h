// Copyright ROK2. Fog of War + Scouts system (P5-T5).
//
// يدير كشف الخريطة: شبكة خلايا (2400×2400) كل خلية لها حالة كشف
// (غير مكتشفة / مكتشفة جزئياً / مكتشفة). الكشافة تكشف تدريجياً حول موقعها.
// المدينة تكشف دائرة حولها تلقائياً. المدن والممرات والعقد تُخفى في الضباب.
// المرجع: 07-game-design/rok-features-audit.md §9 + GDD.md (الكشافة).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rok2FogOfWar.generated.h"

/** حالة كشف خلية واحدة على الخريطة. */
UENUM(BlueprintType)
enum class ERok2FogState : uint8
{
	Unexplored = 0,  // غير مكتشفة — ضباب كامل
	Partially,       // مكتشفة جزئياً — رؤية خافتة (مرّت بها كشافة)
	Explored         // مكتشفة — رؤية كاملة
};

/** كشافة واحدة تسير لنقطة هدف. */
USTRUCT(BlueprintType)
struct FRok2Scout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Id;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromX = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double FromY = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToX = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	double ToY = 0;

	/** وقت الإرسال (UTC ms) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 StartMs = 0;

	/** وقت الوصول المتوقع (UTC ms) */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int64 EtaMs = 0;

	/** هل وصلت؟ */
	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	bool bArrived = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFogUpdated, const TArray<uint8>&, FogGrid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoutArrived, const FRok2Scout&, Scout);

UCLASS(BlueprintType)
class ROK2_API URok2FogOfWar : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2FogOfWar* Get();

	/** يهيئ الشبكة بأبعاد الخريطة (2400×2400). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Init(int32 MapWidth, int32 MapHeight, int32 CellSize = 50);

	/** يكشف دائرة حول نقطة (بنصف قطر بالخلايا) — يُستدعى عند وصول كشافة أو تحديث مدينة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RevealArea(double CenterX, double CenterY, int32 RadiusCells, bool bFullReveal = true);

	/** يعيد حالة كشف خلية بإحداثيات العالم. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	ERok2FogState GetFogStateAt(double WorldX, double WorldY) const;

	/** هل النقطة مكتشفة (أي حالة غير Unexplored)؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsExplored(double WorldX, double WorldY) const;

	/** يعيد شبكة الكشف كاملة (للرسم). */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<uint8>& GetFogGrid() const { return FogGrid; }

	/** يضيف كشافة جديدة (تُرسل من المدينة لهدف). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void AddScout(const FRok2Scout& Scout);

	/** يحدّث الكشافة (يُستدعى كل Tick — يكشف عند الوصول). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void UpdateScouts(int64 NowMs);

	/** يعيد قائمة الكشافة الحالية. */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	const TArray<FRok2Scout>& GetScouts() const { return Scouts; }

	/** يزيل كشافة (عند وصولها أو إلغائها). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void RemoveScout(const FString& ScoutId);

	/** يُبث عند تحديث شبكة الكشف. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnFogUpdated OnFogUpdated;

	/** يُبث عند وصول كشافة. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnScoutArrived OnScoutArrived;

	/** حجم الخلية الافتراضي (وحدات عالمية). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 CellSize = 50;

	/** نصف قطر الكشف الافتراضي للمدينة (بالخلايا). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 CityRevealRadius = 8;

	/** نصف قطر الكشف الافتراضي للكشافة (بالخلايا). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 ScoutRevealRadius = 5;

protected:
	/** شبكة الكشف (0=غير مكتشفة، 1=جزئي، 2=مكتشفة) — مفلطحة row-major. */
	TArray<uint8> FogGrid;

	/** أبعاد الشبكة بالخلايا. */
	int32 GridWidth = 0;
	int32 GridHeight = 0;

	/** قائمة الكشافة النشطة. */
	TArray<FRok2Scout> Scouts;

	/** يحوّل إحداثيات عالمية إلى فهرس خلية في الشبكة. */
	int32 WorldToGridIndex(double WorldX, double WorldY) const;

	/** يتحقق من صلاحية فهرس الخلية. */
	bool IsValidCell(int32 CellX, int32 CellY) const;

	/** يكشف خلية واحدة (ترقية حالتها). */
	void RevealCell(int32 CellX, int32 CellY, bool bFullReveal);

	bool bInitialized = false;
};
