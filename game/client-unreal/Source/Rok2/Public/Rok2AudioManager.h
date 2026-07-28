// Copyright ROK2. Audio manager for civ-themed music + SFX (P5-T6).
//
// يدير الموسيقى الخلفية والمؤثرات الصوتية حسب حضارة اللاعب (من URok2CivThemes).
// يقرأ ملفات .wav من Content/Audio/<civ>/ إن وُجدت، وإلا يبقى صامتاً (placeholder).
// المرجع: 07-game-design/ui-ux-design-system.md §10 + civilizations-visual-design.md (صوتيات).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimerManager.h"
#include "Rok2AudioManager.generated.h"

class USoundWave;
class UAudioComponent;

/** نوع الصوت (موسيقى خلفية أو مؤثر). */
UENUM(BlueprintType)
enum class ERok2AudioType : uint8
{
	Music = 0,      // موسيقى خلفية (تتكرر)
	BuildComplete,  // بناء يكتمل
	Upgrade,        // ترقية مبنى
	BattleVictory,  // نصر في قتال
	BattleDefeat,   // هزيمة
	MarchStart,     // انطلاق مسيرة
	ButtonClick,    // لمس زر
	Notification    // إشعار
};

/** حالة الموسيقى الحالية. */
UENUM(BlueprintType)
enum class ERok2MusicState : uint8
{
	Stopped = 0,
	Playing,
	Paused
};

/** نمط الموسيقى: سلام (مدينة) أو قتال — P4-T3. */
UENUM(BlueprintType)
enum class ERok2MusicMode : uint8
{
	Peace = 0,  // music.wav (الوضع الافتراضي)
	Battle      // battle.wav (أثناء/بعد تقرير قتال)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMusicStateChanged, ERok2MusicState, NewState);

UCLASS(BlueprintType)
class ROK2_API URok2AudioManager : public UObject
{
	GENERATED_BODY()

public:
	/** نسخة مشتركة (AddToRoot) */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	static URok2AudioManager* Get();

	/** يهيئ المدير بحضارة اللاعب (تُستدعى عند Login/InitCity). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void InitForCiv(const FString& CivId);

	/** يشغّل الموسيقى الخلفية للحضارة (تتكرر). */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void PlayMusic();

	/** يوقف الموسيقى. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void StopMusic();

	/** يدخل وضع القتال: يبدّل الموسيقى إلى battle.wav ويعود للسلام تلقائياً بعد المهلة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void EnterBattleMode();

	/** يخرج من وضع القتال فوراً ويعود لموسيقى السلام. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void ExitBattleMode();

	/** هل الموسيقى الحالية في وضع قتال؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsInBattleMode() const { return MusicMode == ERok2MusicMode::Battle; }

	/** مهلة العودة التلقائية لموسيقى السلام بعد آخر قتال (ثوانٍ). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	float BattleModeTimeout = 30.f;

	/** يشغّل مؤثراً صوتياً لمرة واحدة. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void PlaySfx(ERok2AudioType Type);

	/** يشغّل مؤثراً صوتياً لمرة واحدة بحجم مخصص. */
	UFUNCTION(BlueprintCallable, Category = "Rok2")
	void PlaySfxWithVolume(ERok2AudioType Type, float Volume);

	/** هل الموسيقى تعمل حالياً؟ */
	UFUNCTION(BlueprintPure, Category = "Rok2")
	bool IsMusicPlaying() const { return MusicState == ERok2MusicState::Playing; }

	/** يُبث عند تغيّر حالة الموسيقى. */
	UPROPERTY(BlueprintAssignable, Category = "Rok2")
	FOnMusicStateChanged OnMusicStateChanged;

	/** مستوى الصوت العام (0-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	float MasterVolume = 0.8f;

	/** هل الصوت مفعّل؟ (إيقاف كلي) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2")
	bool bAudioEnabled = true;

protected:
	/** حضارة اللاعب الحالية. */
	FString CurrentCivId = TEXT("rome");

	/** حالة الموسيقى. */
	ERok2MusicState MusicState = ERok2MusicState::Stopped;

	/** نمط الموسيقى الحالي (سلام/قتال) — P4-T3. */
	ERok2MusicMode MusicMode = ERok2MusicMode::Peace;

	/** مؤقت العودة التلقائية لموسيقى السلام بعد القتال. */
	FTimerHandle BattleModeTimer;

	/** مكون الموسيقى (إن وُجدت ملفات). */
	UPROPERTY(Transient)
	UAudioComponent* MusicComponent;

	/** خريطة نوع الصوت → مسار الملف. */
	TMap<ERok2AudioType, FString> SfxPaths;

	/** خريطة حضارة → مسار ملف الموسيقى (سلام). */
	TMap<FString, FString> MusicPaths;

	/** خريطة حضارة → مسار ملف موسيقى القتال — P4-T3. */
	TMap<FString, FString> BattleMusicPaths;

	/** يبني خريطة المسارات الافتراضية من Content/Audio/. */
	void BuildAudioPaths();

	/** يحاول تحميل USoundWave من مسار (يعيد nullptr إن لم يوجد). */
	USoundWave* LoadSound(const FString& Path) const;

	/** يشغّل صوتاً من مسار (إن وُجد). */
	void PlaySoundAtPath(const FString& Path, float Volume);

	/** يشغّل الموسيقى حسب النمط الحالي (سلام/قتال) — P4-T3. */
	void PlayCurrentModeMusic();

	bool bInitialized = false;
};
