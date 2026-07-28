#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Rok2ApiSubsystem.generated.h"

UCLASS()
class ROK2_API URok2ApiSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadWrite, Category = "API")
	FString AuthToken;

	UPROPERTY(BlueprintReadWrite, Category = "API")
	FString PlayerId;
};
