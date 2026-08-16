// Copyright ROK2. ورقة التدريب والشفاء اللمسية (P18-T2).
//
// قبل هذا البند: زرا «تدريب» و«شفاء» في بطاقة المبنى كانا يبثّان
// OnBuildingAction بلا أي مشترك — والواجهة الوحيدة للتدريب تسكن لوحاً
// مطوياً في CityWidget المتقاعد، وHealWounded لم يكن له أي مستدعٍ.
// هذه الورقة تفتح من بطاقة المبنى (عبر GameMode) بنمطين:
//   train — وحدات فرع المبنى من Meta.TrainableUnits بعدّاد لمسي (−/+) وزر تدريب.
//   heal  — جرحى المستشفى من City.Wounded وزر شفاء لكل وحدة + شفاء الكل.
// كل الأرقام (قابلية التدريب/التكلفة/المدة/السعة) يحرّها الخادم وحده.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2TrainHealSheetWidget.generated.h"

class URok2Api;
class UButton;
class UTextBlock;
class UVerticalBox;

/** وسيط زر لكل صف — يحمل معرّف الوحدة ودلتا العدّاد (0 = زر الإجراء).
 *  نفس نمط URok2ResearchRowProxy: UFUNCTION لا تقبل معاملات ملتقطة. */
UCLASS()
class URok2TrainUnitProxy : public UObject
{
	GENERATED_BODY()
public:
	FString UnitId;
	/** ‑1 أو +1 لزرَي العدّاد؛ 0 لزر الإجراء. */
	int32 Delta = 0;
	bool bIsAction = false;

	UPROPERTY(Transient)
	class URok2TrainHealSheetWidget* Owner = nullptr;

	UFUNCTION()
	void HandleClick();
};

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2TrainHealSheetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** InMode: "train" أو "heal" — يطابق ActionKindForBuilding في بطاقة المبنى. */
	void Setup(URok2Api* InApi, const FString& InMode, const FString& InBuildingId);

	/** عدّاد لمسي ±1 — عام لأن وسيط الزر يناديه. */
	void AdjustCount(const FString& UnitId, int32 Delta);

	/** تدريب/شفاء وحدة واحدة — عام لأن وسيط الزر يناديه. */
	void HandleUnitAction(const FString& UnitId);

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	URok2Api* Api;

	/** train | heal. */
	UPROPERTY(Transient)
	FString Mode;

	/** المبنى المصدر (barracks/stable/... أو hospital) — يحدد فرع الوحدات. */
	UPROPERTY(Transient)
	FString BuildingId;

	UPROPERTY(Transient)
	UVerticalBox* RowsList;

	/** ترويسة الورقة — تعرض سطر حالة (استعمال المستشفى/السعة في نمط الشفاء). */
	UPROPERTY(Transient)
	UTextBlock* HeaderText;

	/** عدّاد كل صف (للتدريب) — المفتاح معرّف الوحدة. */
	UPROPERTY(Transient)
	TMap<FString, int32> Counts;

	UPROPERTY(Transient)
	TArray<URok2TrainUnitProxy*> Proxies;

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnHealAllClicked();

	/** المدينة تصل بعد FetchCity/الأحداث — نمط الشفاء يعيد رسم أعداد الجرحى. */
	UFUNCTION()
	void OnCityLoadedHandler(const FRok2City& City);

	void RebuildRows();

	/** صف واحد: اسم + عدّاد لمسي (تدريب) أو عدد الجرحى (شفاء) + زر إجراء. */
	void BuildRow(const FString& UnitId, const FString& DisplayName, bool bTrainMode);

	/** فرع القوات الذي يدرّبه هذا المبنى — من أسماء buildings.json لا اختراع. */
	FString BranchForBuilding(const FString& InBuildingId) const;

	/** اسم الوحدة من Meta.TrainableUnits وإلا المعرّف نفسه. */
	FString UnitName(const FString& UnitId) const;
};
