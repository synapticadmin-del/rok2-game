// Copyright ROK2. Boot UI widget (login + civ select + loading/connection status).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BootWidget.generated.h"

class URok2Api;
class UComboBoxString;
class UEditableTextBox;
class UButton;
class UTextBlock;
class UBorder;

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

	/** شاشة التحميل: نص الحالة أسفل البطاقة (جاري الاتصال / إعادة المحاولة / فشل) */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StatusText;

	/** حاوية مؤشر التحميل — تظهر أثناء أي عملية شبكة */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* LoadingPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* LoadingText;

	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void Setup(URok2Api* InApi);

protected:
	UPROPERTY(Transient)
	URok2Api* Api;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

	UFUNCTION()
	void OnConnectionState(bool bOnline, const FString& StatusMessage);

	/** إظهار/إخفاء مؤشر التحميل مع نص اختياري */
	void SetLoading(bool bShow, const FString& Message = TEXT(""));

	/** نقاط متحركة لمؤشر التحميل */
	float LoadingDotsTimer = 0.f;
	FString LoadingBaseMessage;
	bool bLoadingVisible = false;
};
