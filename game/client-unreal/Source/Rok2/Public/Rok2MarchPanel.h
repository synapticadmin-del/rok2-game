// Copyright ROK2.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2MarchPanel.generated.h"

class URok2Api;
class USpinBox;
class UButton;
class UTextBlock;
class UComboBoxString;

UCLASS()
class URok2MarchPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	URok2Api* Api;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetType;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetId;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString TargetName;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	float ToX = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	float ToY = 0.f;

protected:
	UPROPERTY(Transient)
	UTextBlock* TargetNameText;

	UPROPERTY(Transient)
	UTextBlock* DistanceText;

	/** سعة القلعة وحالة طبقة الخريطة؛ تعرض قبل إرسال القوات أو الكشافة. */
	UPROPERTY(Transient)
	UTextBlock* MarchAvailabilityText;

	UPROPERTY(Transient)
	USpinBox* InfantrySpinBox;

	UPROPERTY(Transient)
	USpinBox* CavalrySpinBox;

	UPROPERTY(Transient)
	USpinBox* ArcherSpinBox;

	UPROPERTY(Transient)
	UComboBoxString* PrimaryCommanderBox;

	UPROPERTY(Transient)
	UComboBoxString* SecondaryCommanderBox;

	UPROPERTY(Transient)
	UButton* DispatchButton;

	/** يظهر فقط على الممر والعرش في الطبقة التكتيكية للاعب المنتمي لتحالف. */
	UPROPERTY(Transient)
	UButton* RallyButton;

	/** قائمة المسيرات الشخصية المتحركة فقط؛ لا تعرض مسيرات الرالي المشتركة. */
	UPROPERTY(Transient)
	UComboBoxString* RedirectMarchBox;

	UPROPERTY(Transient)
	UButton* RedirectButton;

	/** يربط التسمية المقروءة بمعرف المسيرة الذي يتعامل معه الخادم. */
	TMap<FString, FString> RedirectOptionIds;

	UFUNCTION()
	void OnDispatchClicked();

	UFUNCTION()
	void OnRedirectClicked();

	UFUNCTION()
	void OnRallyClicked();

	/** P5-T5: يرسل كشافة للنقطة المحددة */
	UFUNCTION()
	void OnScoutClicked();
};
