// Copyright ROK2. Battle Report Widget — قائمة التقارير + عرض الخسائر التفصيلي.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2BattleReportWidget.generated.h"

class URok2Api;
class UTextBlock;
class UVerticalBox;
class UButton;

UCLASS()
class URok2BattleReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** يربط الواجهة بالـ API ويبني القائمة الأولى */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Battle")
	void Setup(URok2Api* InApi);

	/** يبني الواجهة كاملة برمجياً (بدون Blueprint assets) */
	virtual void NativeConstruct() override;

	/** زر الإغلاق — يزيل الواجهة من الشاشة */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Battle")
	void Close();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	/** قائمة التقارير (يمين اللوحة) */
	UPROPERTY(Transient)
	UVerticalBox* ReportList;

	/** لوحة التفاصيل (شمال اللوحة) */
	UPROPERTY(Transient)
	UVerticalBox* DetailPanel;

	UFUNCTION()
	void OnBattleReports(const TArray<FRok2BattleReport>& Reports);

	UFUNCTION()
	void OnCloseClicked();

public:
	/** يعيد بناء قائمة التقارير */
	void RebuildList(const TArray<FRok2BattleReport>& Reports);

	/** يعرض تقريراً واحداً في لوحة التفاصيل */
	void ShowReport(const FRok2BattleReport& R);

	/** يلخّص خسائر طرف في سطر واحد: "مشاة 120 (قتلى 6 · خطير 72 · خفيف 42)" */
	static FString SummarizeSide(const FRok2BattleSide& Side);

	/** اسم ودّي لنوع المعركة */
	static FString KindLabel(const FString& Kind);

	TArray<FRok2BattleReport> Current;
};

/** handler لزر تقرير في القائمة */
UCLASS()
class URok2ReportRowHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Index = 0;

	UPROPERTY()
	URok2BattleReportWidget* Widget = nullptr;

	UFUNCTION()
	void OnClick();
};
