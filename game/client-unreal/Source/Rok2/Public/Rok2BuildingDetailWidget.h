// Copyright ROK2. Building card — RoK-style Bottom Sheet (P5-T3).
//
// بطاقة مبنى تنزلق من أسفل الشاشة (Bottom Sheet) بدل النافذة الوسطية.
// المواصفة: 07-game-design/ui-ux-design-system.md §3.2.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BuildingDetailWidget.generated.h"

class URok2Api;
class UTextBlock;
class UButton;
class UVerticalBox;
class UBorder;
class UProgressBar;
class UImage;
class ARok2CityLayoutActor;

/** حدث زر ثانوي حسب نوع المبنى (تدريب/شفاء/بحث/صناديق) — يفوَّض للخارج */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildingAction, const FString&, BuildingId, const FString&, ActionKind);

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2BuildingDetailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	FString BuildingId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	int32 CurrentLevel = 1;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void SetupBuilding(URok2Api* InApi, const FString& InBuildingId, int32 InLevel);

	/** يُطلق عند ضغط الزر الثانوي (تدريب/شفاء/بحث/صناديق) */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnBuildingAction OnBuildingAction;

protected:
	virtual void NativeConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// تلاشي/انزلاق الدخول
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY() UBorder* SheetBorder;
	UPROPERTY() UTextBlock* TitleText;
	// P6-T1: أيقونة المبنى في الترويسة (إجرائية من URok2ArtAssets)
	UPROPERTY() UImage* HeaderIcon;
	UPROPERTY() UTextBlock* LevelText;
	UPROPERTY() UTextBlock* DescText;
	// P6-T1: التكلفة أصبحت زوجي أيقونة+رقم (طعام/خشب) + أيقونة ساعة للمدة
	UPROPERTY() UImage* CostFoodIcon;
	UPROPERTY() UTextBlock* CostFoodText;
	UPROPERTY() UImage* CostWoodIcon;
	UPROPERTY() UTextBlock* CostText;
	UPROPERTY() UImage* TimeIcon;
	UPROPERTY() UTextBlock* TimeText;
	UPROPERTY() UProgressBar* QueueBar;
	UPROPERTY() UTextBlock* QueueText;
	UPROPERTY() UButton* UpgradeButton;
	UPROPERTY() UTextBlock* UpgradeBtnText;
	UPROPERTY() UButton* ActionButton;
	UPROPERTY() UTextBlock* ActionBtnText;
	UPROPERTY() UButton* FacadeButton;
	UPROPERTY() UTextBlock* FacadeBtnText;
	// P6-T1: أيقونة الزر الثانوي حسب نوع الإجراء (تدريب/شفاء/بحث/صناديق)
	UPROPERTY() UImage* ActionBtnIcon;
	UPROPERTY() UButton* CloseButton;

	UFUNCTION()
	void OnUpgradeClicked();

	UFUNCTION()
	void OnActionClicked();

	/** يبدّل بين standard وceremonial وfortified للمباني المتحركة. */
	UFUNCTION()
	void OnFacadeClicked();

	UFUNCTION()
	void OnCloseClicked();

	// نوع الزر الثانوي حسب المبنى
	FString ActionKindForBuilding(const FString& Id) const;
	FString ActionLabelForBuilding(const FString& Id) const;
	// P6-T1: معرّف أيقونة الزر الثانوي الإجرائية
	FString ActionIconForBuilding(const FString& Id) const;
	FString FacadeLabel() const;
	ARok2CityLayoutActor* FindCityLayout() const;

	// أنيميشن الدخول (انزلاق من أسفل)
	// P6-T3: حالة الانزلاق المحلية أُزيلت — الحركة صارت في URok2MotionLibrary
};
