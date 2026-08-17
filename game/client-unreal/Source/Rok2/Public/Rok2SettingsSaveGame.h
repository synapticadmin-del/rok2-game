// Copyright ROK2. حفظ إعدادات اللاعب المحلية (P18-T6).
//
// الإعدادات تفضيلات **جهاز** لا حالة لعب: لا تُرسل للخادم ولا تُقرأ منه، ولا
// تُربط بمعرّف لاعب. من يشارك جهازاً يشارك مقياس الواجهة ومستوى الصوت — وهذا
// المتوقّع، بخلاف تخطيط المدينة الذي يُربط بمعرّف اللاعب في
// `URok2CityLayoutSaveGame` لأنه حالة لعب.
//
// AGENTS.md §3 «الخادم هو السلطة» لا يُخالف هنا: لا رقم توازن ولا مورد ولا نتيجة
// قتال في هذا الملف — صوت وحجم خط وتباين فقط.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Rok2SettingsSaveGame.generated.h"

UCLASS()
class ROK2_API URok2SettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** إصدار البنية — يسمح بترحيل واضح عند إضافة إعداد لاحقاً. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	int32 SchemaVersion = 1;

	/** مستوى الموسيقى الخلفية [0..1]. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	float MusicVolume = 0.8f;

	/** مستوى المؤثرات الصوتية [0..1]. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	float SfxVolume = 0.8f;

	/** مقياس الواجهة [0.85..1.6]. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	float UiScale = 1.f;

	/** وضع التباين العالي. */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "Rok2")
	bool bHighContrast = false;
};
