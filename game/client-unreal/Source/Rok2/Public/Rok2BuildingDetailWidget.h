// Copyright ROK2. Building Detail & Upgrade Popup Widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BuildingDetailWidget.generated.h"

class URok2Api;
class UTextBlock;
class UButton;
class UVerticalBox;

UCLASS()
class URok2BuildingDetailWidget : public UUserWidget
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

protected:
	virtual void NativeConstruct() override;

	UPROPERTY()
	UTextBlock* TitleText;

	UPROPERTY()
	UTextBlock* LevelText;

	UPROPERTY()
	UTextBlock* CostText;

	UPROPERTY()
	UButton* UpgradeButton;

	UPROPERTY()
	UButton* ActionButton;

	UPROPERTY()
	UButton* CloseButton;

	UFUNCTION()
	void OnUpgradeClicked();

	UFUNCTION()
	void OnActionClicked();

	UFUNCTION()
	void OnCloseClicked();
};
