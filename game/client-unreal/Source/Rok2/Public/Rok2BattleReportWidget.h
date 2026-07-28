// Copyright ROK2. Battle Report Widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2BattleReportWidget.generated.h"

UCLASS()
class URok2BattleReportWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rok2|Battle")
	void UpdateReport(bool bVictory, int32 Losses, int32 Remaining);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rok2|Battle")
	void OnReportUpdated(bool bVictory, int32 Losses, int32 Remaining);
};
