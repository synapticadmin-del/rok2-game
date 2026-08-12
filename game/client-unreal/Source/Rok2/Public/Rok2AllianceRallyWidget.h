// Copyright ROK2.
// بطاقة رالي تحالف سلطوية: العرض محلي، أما الأهلية والقوات والحالة فتُحسم بالخادم.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2AllianceRallyWidget.generated.h"

class URok2Api;
class UBorder;
class UButton;
class UTextBlock;

UCLASS()
class ROK2_API URok2AllianceRallyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** يضبط البطاقة قبل إضافتها إلى قائمة التحالف. */
	void Setup(URok2Api* InApi, const FRok2AllianceRally& InRally);

protected:
	UPROPERTY(Transient) URok2Api* Api;
	FRok2AllianceRally Rally;
	UPROPERTY(Transient) UBorder* CardBorder;
	UPROPERTY(Transient) UTextBlock* TargetText;
	UPROPERTY(Transient) UTextBlock* StatusText;
	UPROPERTY(Transient) UTextBlock* CountdownText;
	UPROPERTY(Transient) UButton* JoinButton;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildCard();
	void RefreshDisplay();
	TMap<FString, int32> BuildHomeContribution() const;

	UFUNCTION()
	void OnJoinClicked();
};
