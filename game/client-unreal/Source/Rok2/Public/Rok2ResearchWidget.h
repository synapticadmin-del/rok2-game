// Copyright ROK2. شاشة البحث العلمي (P18-T1) — header.
//
// ما كانت عليه الشاشة قبل هذا البند: ثلاثة تبويبات بلا سلوك، وصفٌّ واحد
// بنصٍّ ثابت `"Tech: Architecture Lv.1\nReq: Academy Lv.5\nCost: 1000 Wood"`،
// وزر بحث معالجُه فارغ (`// if (Api) ... StartResearch` معطَّل بتعليق).
// ولم تكن الشاشة تُعرض من أي مكان — `URok2ResearchWidget` غير مذكور في
// `Rok2GameMode` ولا في أي ودجت آخر. أي أن نظام البحث كان مكتملاً على الخادم
// (`/v1/research` و`/v1/city/research` وشجرة في `data/research.json`) وغائباً
// عن اللعبة تماماً.
//
// الآن: الشجرة تُقرأ من الخادم، والصفوف تُبنى منها، وكل صف يعرض حالته
// (متاح / سقف / أكاديمية أدنى / متطلب ناقص / موارد ناقصة) بلون وأيقونة ونص —
// لا لوناً وحده.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2Types.h"
#include "Rok2ResearchWidget.generated.h"

class URok2Api;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;

/** وسيط لكل زر بحث — يحمل معرّف التقنية ويعيد بثّ الضغط معه.
 *  نفس نمط `URok2BuildButtonProxy`: UFUNCTION لا تقبل معاملات ملتقطة. */
UCLASS()
class URok2ResearchRowProxy : public UObject
{
	GENERATED_BODY()
public:
	FString TechId;

	UPROPERTY(Transient)
	class URok2ResearchWidget* Owner = nullptr;

	UFUNCTION()
	void HandleClick();
};

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2ResearchWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|Research")
	void Setup(URok2Api* InApi);

	/** يبدأ بحث تقنية — يُنادى من وسيط الصف. */
	void RequestResearch(const FString& TechId);

	// P18-T5: الرجوع يغلق شاشة البحث كزر الإغلاق فيها.
	virtual void DismissLayer() override { OnCloseClicked(); }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	URok2Api* Api;

	/** الفرع المعروض: economy | military | defense — تطابق `branch` في الخادم. */
	UPROPERTY(Transient)
	FString ActiveBranch;

	UPROPERTY(Transient) UVerticalBox* TechList;
	UPROPERTY(Transient) UTextBlock* AcademyText;
	UPROPERTY(Transient) UButton* EconomyTab;
	UPROPERTY(Transient) UButton* MilitaryTab;
	UPROPERTY(Transient) UButton* DefenseTab;

	/** وسطاء الصفوف — UPROPERTY كي لا يجمعها GC قبل الضغط. */
	UPROPERTY(Transient)
	TArray<URok2ResearchRowProxy*> RowProxies;

	UFUNCTION()
	void OnResearchLoaded();

	/** المدينة تتغيّر بعد كل بحث (خصم + طابور)، وحالة «موارد غير كافية» تعتمد
	 *  عليها — فتوقيع الدالة يطابق FOnCityLoaded ويعيد الرسم. */
	UFUNCTION()
	void OnCityUpdated(const FRok2City& City);

	UFUNCTION()
	void OnEconomyTab();

	UFUNCTION()
	void OnMilitaryTab();

	UFUNCTION()
	void OnDefenseTab();

	UFUNCTION()
	void OnCloseClicked();

	void SelectBranch(const FString& Branch);
	void RebuildList();
};
