// Copyright ROK2. City HUD widget - resources, buildings, train, alliance.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2CityWidget.generated.h"

class URok2Api;
class UTextBlock;
class UVerticalBox;
class UButton;
class UEditableTextBox;
class UComboBoxString;
class USpinBox;

UCLASS()
class URok2CityWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ResourcesText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PlayerInfoText;

	/** شارة حالة الاتصال في الشريط العلوي (متصل / إعادة اتصال / غير متصل) — P1-T2 */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ConnectionText;

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* BuildingsList;

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* ActiveQueuesList;

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* TroopsList;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* TrainUnitCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	USpinBox* TrainCountSpin;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* TrainButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* AllianceNameInput;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* AllianceTagInput;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* CreateAllianceButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* MapButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* RefreshButton;

	/** زر فتح تقارير القتال (P1-T4) */
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* ReportsButton;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Refresh();

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnCityLoaded(const FRok2City& City);

	UFUNCTION()
	void OnTrainClicked();

	UFUNCTION()
	void OnCreateAllianceClicked();

	UFUNCTION()
	void OnMapClicked();

	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnReportsClicked();

	UFUNCTION()
	void OnToast(const FString& Message);

	UFUNCTION()
	void OnConnectionState(bool bOnline, const FString& StatusMessage);

	UPROPERTY(Transient)
	TArray<URok2QueueBtnHandler*> QueueHandlers;
};

UCLASS()
class URok2QueueBtnHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString QueueId;

	UPROPERTY()
	URok2Api* Api;

	UFUNCTION()
	void OnClick();
};
