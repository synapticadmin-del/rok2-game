// Copyright ROK2. Hex grid utilities (axial coordinates) for the hex castle city (P5-T1).
//
// المواصفة: 07-game-design/castle-hex-city.md
// شبكة axial (q, r) — نقي بدون UObject ليسهل اختبارها واستخدامها من أي مكان.

#pragma once

#include "CoreMinimal.h"
#include "Rok2HexGrid.generated.h"

/** خلية سداسية بإحداثيات axial (q, r). s = -q-r ضمنياً (إحداثيات مكعبة). */
USTRUCT(BlueprintType)
struct FRok2HexCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 R = 0;

	FRok2HexCell() {}
	FRok2HexCell(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

	int32 S() const { return -Q - R; }

	/** المسافة بين خليتين (عدد الخطوات على الشبكة). */
	int32 DistanceTo(const FRok2HexCell& Other) const
	{
		const int32 dq = Q - Other.Q;
		const int32 dr = R - Other.R;
		const int32 ds = S() - Other.S();
		return (FMath::Abs(dq) + FMath::Abs(dr) + FMath::Abs(ds)) / 2;
	}

	bool operator==(const FRok2HexCell& Other) const { return Q == Other.Q && R == Other.R; }
	bool operator!=(const FRok2HexCell& Other) const { return !(*this == Other); }
};

FORCEINLINE uint32 GetTypeHash(const FRok2HexCell& Cell)
{
	return HashCombine(GetTypeHash(Cell.Q), GetTypeHash(Cell.R));
}

/** مكتبة ثوابت وتحويلات الشبكة السداسية. */
UCLASS()
class ROK2_API URok2HexGrid : public UObject
{
	GENERATED_BODY()

public:
	/** الاتجاهات الستة للجوار في axial. */
	static const TArray<FRok2HexCell>& Directions();

	/** تحويل خلية hex إلى موضع عالم (سمك الخلية = الحجم الأفقي بين المركز والرأس). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static FVector HexToWorld(const FRok2HexCell& Cell, float Size);

	/** تحويل موضع عالم إلى أقرب خلية hex. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static FRok2HexCell WorldToHex(const FVector& World, float Size);

	/** هل الخلية داخل سور سداسي نصف قطره Radius؟ (الحلقة max(|q|,|r|,|s|) <= Radius) */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static bool IsInsideRadius(const FRok2HexCell& Cell, int32 Radius);

	/** حلقة خلايا عند مسافة Radius بالضبط (خلايا السور نفسه). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static TArray<FRok2HexCell> Ring(int32 Radius);

	/** كل الخلايا داخل نصف القطر (بما فيه المركز). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static TArray<FRok2HexCell> FilledHexagon(int32 Radius);

	/** زهرة hex (مركز + 6 جوار) — بصمة المباني المتوسطة. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static TArray<FRok2HexCell> Flower(const FRok2HexCell& Center);

	/** جوار خلية (6 خلايا). */
	UFUNCTION(BlueprintPure, Category = "Rok2|Hex")
	static TArray<FRok2HexCell> Neighbors(const FRok2HexCell& Cell);

	/** تقريب كسور axial إلى أقرب خلية صحيحة (hex rounding). */
	static FRok2HexCell RoundHex(float Qf, float Rf);
};
