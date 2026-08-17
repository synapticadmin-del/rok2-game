// Copyright ROK2. شاشة الحقيبة (P19-T5) — header.
//
// قبل هذا البند: `ARok2GameMode::HandleItemsAction` سطرٌ واحد يبثّ توست
// «الحقيبة قيد التجهيز» — وزر «حقيبة» في عنقود الـHUD موجود منذ P5-T3. أي أن
// اللاعب يملك عناصر في `player_inventory` (من المتجر والمهام وBattle Pass)
// ولا سبيل له إلى رؤيتها ولا استخدامها.
//
// كل بيانات الشاشة من `GET /v1/items/bag`: الاسم والوصف والفئة والنُدرة من
// `data/items.json`، والعدد من الجدول. لا اسم عنصر ولا رقم توازن في العميل.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2DismissibleLayer.h"
#include "Rok2Types.h"
#include "Rok2BagWidget.generated.h"

class URok2Api;
class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class URok2BagWidget;

/**
 * وسيط صف عنصر — يحمل معرّفه حتى تصل ضغطة الزر.
 * نفس نمط `URok2ResearchRowProxy` و`URok2TrainUnitProxy`: `UFUNCTION` لا تقبل
 * معاملات ملتقطة.
 */
UCLASS()
class URok2BagItemProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FString ItemId;

	/** فئة تُختار (فارغة = زر استخدام لا زر تبويب). */
	UPROPERTY() FString CategoryId;

	UPROPERTY(Transient)
	URok2BagWidget* Owner = nullptr;

	UFUNCTION()
	void HandleClick();
};

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2BagWidget : public UUserWidget, public IRok2DismissibleLayer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|Items")
	void Setup(URok2Api* InApi);

	/** يُنادى من وسيط الصف: استخدام عنصر أو تبديل تبويب. */
	void HandleProxyClick(const FString& ItemId, const FString& CategoryId);

	virtual void DismissLayer() override { OnCloseClicked(); }

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	URok2Api* Api;

	/** التبويب المعروض؛ فارغ = «الكل». */
	UPROPERTY(Transient)
	FString ActiveCategory;

	UPROPERTY(Transient) UHorizontalBox* TabsBox;
	UPROPERTY(Transient) UVerticalBox* ItemsList;
	UPROPERTY(Transient) UTextBlock* GemsText;
	UPROPERTY(Transient) UTextBlock* EmptyText;

	/**
	 * وسائط الصفوف والتبويبات، منفصلتان عن قصد: `RebuildItems` تُنادى وحدها عند
	 * تبديل التبويب، ولو تشاركتا مصفوفة لأتلف تفريغُها أزرار التبويبات القائمة.
	 */
	UPROPERTY(Transient)
	TArray<URok2BagItemProxy*> ItemProxies;

	UPROPERTY(Transient)
	TArray<URok2BagItemProxy*> TabProxies;

	UFUNCTION()
	void OnCloseClicked();

	/** تُنادى عند وصول لقطة الحقيبة من الخادم. */
	UFUNCTION()
	void OnBagUpdated(const FRok2BagState& State);

	void RebuildTabs();
	void RebuildItems();

	/** صف عنصر واحد: أيقونة + اسم ووصف + عدد + زر استخدام إن أمكن. */
	void BuildItemRow(const FRok2BagItem& Item);

	/**
	 * أول طابور نشط يصلح للتسريع، أو فارغ.
	 *
	 * التسريع يحتاج هدفاً، وطلبه من اللاعب باختيار طابور من قائمة ثانية يجعل
	 * الفعل ثلاث لمسات — والقاعدة #1 في `ui-ux-design-system.md` §8 «كل إجراء
	 * ≤ 3 لمسات من شاشة المدينة». فالهدف هو الطابور الأول، والخادم يرفض إن لم
	 * يوجد.
	 */
	FString FirstActiveQueueId() const;
};
