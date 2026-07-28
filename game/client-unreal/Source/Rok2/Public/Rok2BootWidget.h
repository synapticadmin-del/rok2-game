// Copyright ROK2. Boot UI widget (login + civ select).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BootWidget.generated.h"

class URok2Api;
class UComboBoxString;
class UEditableTextBox;
class UButton;

UCLASS()
class URok2BootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* EnterButton;

	UPROPERTY(meta = (BindWidgetOptional))
	UEditableTextBox* NameInput;

	UPROPERTY(meta = (BindWidgetOptional))
	UComboBoxString* CivCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* StartButton;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnEnterClicked();

	UFUNCTION()
	void OnStartClicked();

	UFUNCTION()
	void OnLoginComplete(const FString& Token);

	UFUNCTION()
	void OnApiError(const FString& Message);

	UFUNCTION()
	void OnPlayerLoaded(const FRok2Player& Player);
};
