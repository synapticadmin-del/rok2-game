//
// تخزين محلي لتخطيط المدينة في انتظار مزامنة الخادم السلطوية.
// يحفظ المواضع، الدوران، والواجهات التجميلية لكل مبنى باسم لاعب منفصل.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Rok2CityLayoutActor.h"
#include "Rok2CityLayoutSaveGame.generated.h"

UCLASS()
class ROK2_API URok2CityLayoutSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** إصدار البنية كي يمكن ترحيل التخطيط بوضوح عند تغيير القواعد مستقبلاً. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	int32 SchemaVersion = 1;

	/** معرّف اللاعب الذي ينتمي إليه الحفظ؛ يمنع خلط حسابين على الجهاز نفسه. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	FString PlayerId;

	/** مواضع ودوران وواجهات مباني المدينة. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	TArray<FRok2BuildingPlacement> Placements;
};
