// Copyright ROK2.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2Types.h"
#include "Rok2AllianceRosterWidget.generated.h"

class URok2Api;
class UVerticalBox;
class UButton;
class UTextBlock;
class URok2BattleReportWidget;

USTRUCT(BlueprintType)
struct FRok2AllianceMemberData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	int32 Power = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	FString Rank;
};

UCLASS()
class URok2AllianceRosterWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	URok2Api* Api;

	UPROPERTY(BlueprintReadWrite, Category = "Rok2")
	TArray<FRok2AllianceMemberData> Members;

protected:
	UPROPERTY(Transient)
	UVerticalBox* RosterVBox;

	/** بطاقات الراليات الحية؛ مصدرها GET /v1/alliance/rallies. */
	UPROPERTY(Transient)
	UVerticalBox* RallyVBox;

	UPROPERTY(Transient)
	UButton* HelpButton;

	UPROPERTY(Transient)
	UButton* RallyReportsButton;

	UPROPERTY(Transient)
	URok2BattleReportWidget* RallyReportsWidget;

	UFUNCTION()
	void OnHelpClicked();

	UFUNCTION()
	void OnRallyReportsClicked();

	UFUNCTION()
	void OnPromoteClicked();

	UFUNCTION()
	void OnKickClicked();

	UFUNCTION()
	void OnInviteClicked();

	void PopulateRoster();
	void PopulateRallies(const TArray<FRok2AllianceRally>& Rallies);

	UFUNCTION()
	void OnRalliesUpdated(const TArray<FRok2AllianceRally>& Rallies);
};
