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
	ButtonClick,    // لمس زر قديم (متوافق مع الودجات السابقة)
	// P7-T1: أصوات الواجهة الموحدة من فهرس أصول P6-T8.
	UiButtonClick,  // ui_button_click.wav
	UiPanelOpen,    // ui_panel_open.wav
	UiPanelClose,   // ui_panel_close.wav
	UiError,        // ui_error.wav
	CivWhisper,     // ui_civ_whisper_<civ>.wav — يُشغَّل عند InitForCiv
	Notification,   // إشعار
	// P4-T4: أحداث اللعب
	GatherComplete, // عودة مسيرة جمع بالموارد
	ResearchComplete, // اكتمال بحث (tech_researched)
	HealComplete,   // بدء شفاء جرحى
	ZoneUnlock,     // فتح منطقة جديدة (zone_unlocked)
	RallyLaunch,    // انطلاق حملة rally (rally_launched)
	// P19-T4: صوتان كانا على القرص في Content/Audio/sfx بلا أي مستهلك —
	// `chest_open.wav` و`wheel_spin.wav` مولّدان في P10-T7 وغير مربوطين بنوع.
	ChestOpen,      // chest_open.wav — فتح صندوق في الحانة
	WheelSpin       // wheel_spin.wav — دوران عجلة/غاتشا
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

	// -----------------------------------------------------------------------
	// P18-T6: مستويان منفصلان — الموسيقى والمؤثرات.
	//
	// `MasterVolume` كان الرقم الوحيد ويضرب في كل شيء، فلا سبيل لخفض الموسيقى
	// وحدها؛ وهي أول ما يخفضه اللاعب. المستويان يُضربان في `MasterVolume` فيبقى
	// معناه «الحدّ الأعلى العام» وتبقى كل الاستدعاءات القائمة صحيحة.
	// -----------------------------------------------------------------------

	/** مستوى الموسيقى الخلفية [0..1] — يُضرب في MasterVolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Volume")
	float MusicVolume = 1.f;

	/** مستوى المؤثرات [0..1] — يُضرب في MasterVolume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rok2|Volume")
	float SfxVolume = 1.f;

	/**
	 * يضبط مستوى الموسيقى ويطبّقه **على الموسيقى العاملة الآن** عبر
	 * `SetVolumeMultiplier` — لا انتظار لأغنية تالية.
	 */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Volume")
	void SetMusicVolume(float NewVolume);

	/** يضبط مستوى المؤثرات (يسري على أول مؤثر بعده). */
	UFUNCTION(BlueprintCallable, Category = "Rok2|Volume")
	void SetSfxVolume(float NewVolume);

	/** المستوى الفعلي للموسيقى بعد الضرب في العام. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Volume")
	float GetEffectiveMusicVolume() const { return FMath::Clamp(MasterVolume * MusicVolume, 0.f, 1.f); }

	/** المستوى الفعلي للمؤثرات بعد الضرب في العام. */
	UFUNCTION(BlueprintPure, Category = "Rok2|Volume")
	float GetEffectiveSfxVolume() const { return FMath::Clamp(MasterVolume * SfxVolume, 0.f, 1.f); }

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
