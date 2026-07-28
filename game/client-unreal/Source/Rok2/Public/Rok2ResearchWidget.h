#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Rok2ResearchWidget.generated.h"

class URok2Api;

UCLASS()
class URok2ResearchWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API")
    URok2Api* Api;

    UFUNCTION()
    void OnResearchButtonClicked();

private:
    FString CurrentSelectedTechId;
};
