// Copyright ROK2. Build menu widget (P5-T3) — RoK-style categorized build grid.
//
// قائمة البناء: شبكة أيقونات مبانٍ بثلاث فئات (اقتصاد/عسكري/زخرفة) تنزلق من أسفل.
// تُفتح بزر البناء الكبير في HUD. المواصفة: ui-ux-design-system.md §3.3.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BuildMenuWidget.generated.h"

class URok2Api;
class UTextBlock;
class UButton;
class UVerticalBox;
class UHorizontalBox;
class UBorder;
class UUniformGridPanel;
class URok2BuildMenuWidget;

/** يُطلق عند اختيار مبنى من القائمة (للبناء/الوضع على الشبكة) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildMenuPick, const FString&, BuildingId);

/** وسيط لكل زر مبنى — يخزن الـ id ويعيد بثّ الضغط معه. */
UCLASS()
class URok2BuildButtonProxy : public UObject
{
	GENERATED_BODY()
public:
	FString Id;
	FOnBuildMenuPick OnPick;

	UFUNCTION()
	void HandleClick() { OnPick.Broadcast(Id); }
};

UCLASS(BlueprintType, Blueprintable)
class ROK2_API URok2BuildMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnBuildMenuPick OnBuildMenuPick;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(Transient)
	URok2Api* Api;

	UPROPERTY() UUniformGridPanel* Grid;
	UPROPERTY() UTextBlock* TabEconText;
	UPROPERTY() UTextBlock* TabMilText;
	UPROPERTY() UTextBlock* TabDecorText;

	UFUNCTION()
	void OnTabEcon();

	UFUNCTION()
	void OnTabMil();

	UFUNCTION()
	void OnTabDecor();

	UFUNCTION()
	void OnCloseClicked();

	void FillGrid(const FString& Category);

private:
	// كتالوج المباني (id, أيقونة, اسم عربي, فئة)
	struct FBuildEntry { FString Id; FString Icon; FString Name; FString Cat; };
	TArray<FBuildEntry> Catalog;
	void BuildCatalog();

	FString CurrentCategory = TEXT("economic");

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void HandleBuildingPicked(const FString& BuildingId);

	/** وسائط أزرار المباني — تُبقى حية للـ GC طوال عرض القائمة. */
	UPROPERTY(Transient)
	TArray<URok2BuildButtonProxy*> Proxies;
};
