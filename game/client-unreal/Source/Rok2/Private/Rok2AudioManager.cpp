// Copyright ROK2. Audio manager for civ-themed music + SFX (P5-T6) — implementation.

#include "Rok2AudioManager.h"
#include "Rok2CivThemes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogRok2Audio, Log, All);

URok2AudioManager* URok2AudioManager::Get()
{
	static URok2AudioManager* Instance = nullptr;
	if (!Instance || !IsValid(Instance))
	{
		Instance = NewObject<URok2AudioManager>();
		Instance->AddToRoot();
	}
	return Instance;
}

void URok2AudioManager::InitForCiv(const FString& CivId)
{
	if (bInitialized && CurrentCivId == CivId) return;
	bInitialized = true;
	CurrentCivId = CivId;

	BuildAudioPaths();

	UE_LOG(LogRok2Audio, Log, TEXT("AudioManager initialized for civ: %s"), *CivId);
}

void URok2AudioManager::BuildAudioPaths()
{
	// مسارات الموسيقى لكل حضارة (Content/Audio/<civ>/music.wav)
	MusicPaths.Empty();
	MusicPaths.Add(TEXT("rome"),    TEXT("Audio/rome/music"));
	MusicPaths.Add(TEXT("china"),   TEXT("Audio/china/music"));
	MusicPaths.Add(TEXT("arabia"),  TEXT("Audio/arabia/music"));
	MusicPaths.Add(TEXT("egypt"),   TEXT("Audio/egypt/music"));
	MusicPaths.Add(TEXT("vikings"), TEXT("Audio/vikings/music"));
	MusicPaths.Add(TEXT("japan"),   TEXT("Audio/japan/music"));

	// مسارات موسيقى القتال لكل حضارة (Content/Audio/<civ>/battle.wav) — P4-T3
	BattleMusicPaths.Empty();
	BattleMusicPaths.Add(TEXT("rome"),    TEXT("Audio/rome/battle"));
	BattleMusicPaths.Add(TEXT("china"),   TEXT("Audio/china/battle"));
	BattleMusicPaths.Add(TEXT("arabia"),  TEXT("Audio/arabia/battle"));
	BattleMusicPaths.Add(TEXT("egypt"),   TEXT("Audio/egypt/battle"));
	BattleMusicPaths.Add(TEXT("vikings"), TEXT("Audio/vikings/battle"));
	BattleMusicPaths.Add(TEXT("japan"),   TEXT("Audio/japan/battle"));

	// مسارات المؤثرات (مشتركة لكل الحضارات، أو مخصصة لاحقاً)
	SfxPaths.Empty();
	SfxPaths.Add(ERok2AudioType::BuildComplete,  TEXT("Audio/sfx/build_complete"));
	SfxPaths.Add(ERok2AudioType::Upgrade,        TEXT("Audio/sfx/upgrade"));
	SfxPaths.Add(ERok2AudioType::BattleVictory,  TEXT("Audio/sfx/victory"));
	SfxPaths.Add(ERok2AudioType::BattleDefeat,   TEXT("Audio/sfx/defeat"));
	SfxPaths.Add(ERok2AudioType::MarchStart,     TEXT("Audio/sfx/march_start"));
	SfxPaths.Add(ERok2AudioType::ButtonClick,    TEXT("Audio/sfx/button_click"));
	SfxPaths.Add(ERok2AudioType::Notification,   TEXT("Audio/sfx/notification"));
	// P4-T4: أحداث اللعب
	SfxPaths.Add(ERok2AudioType::GatherComplete,   TEXT("Audio/sfx/gather_complete"));
	SfxPaths.Add(ERok2AudioType::ResearchComplete, TEXT("Audio/sfx/research_complete"));
	SfxPaths.Add(ERok2AudioType::HealComplete,     TEXT("Audio/sfx/heal_complete"));
	SfxPaths.Add(ERok2AudioType::ZoneUnlock,       TEXT("Audio/sfx/zone_unlock"));
	SfxPaths.Add(ERok2AudioType::RallyLaunch,      TEXT("Audio/sfx/rally_launch"));
}

USoundWave* URok2AudioManager::LoadSound(const FString& Path) const
{
	// يحاول تحميل من /Game/Audio/... (محتوى المشروع)
	// ملاحظة: يتطلب ملفات .wav حقيقية في Content/Audio/ — غير موجودة حالياً (placeholder صامت)
	const FString FullPath = FString::Printf(TEXT("/Game/%s"), *Path);
	USoundWave* Sound = LoadObject<USoundWave>(nullptr, *FullPath);
	if (!Sound)
	{
		UE_LOG(LogRok2Audio, Verbose, TEXT("Sound not found (placeholder silent): %s"), *FullPath);
	}
	return Sound;
}

void URok2AudioManager::PlayMusic()
{
	if (!bAudioEnabled) return;
	if (MusicState == ERok2MusicState::Playing) return;
	PlayCurrentModeMusic();
}

void URok2AudioManager::PlayCurrentModeMusic()
{
	// P4-T3: يختار مسار الموسيقى حسب النمط (سلام/قتال)
	const TMap<FString, FString>& Paths = (MusicMode == ERok2MusicMode::Battle) ? BattleMusicPaths : MusicPaths;
	const FString* Path = Paths.Find(CurrentCivId);
	if (!Path)
	{
		UE_LOG(LogRok2Audio, Warning, TEXT("No music path for civ: %s (mode %d)"), *CurrentCivId, (int32)MusicMode);
		return;
	}

	USoundWave* Music = LoadSound(*Path);
	if (!Music)
	{
		// placeholder صامت — لا مشكلة، نستمر بدون موسيقى
		MusicState = ERok2MusicState::Playing;
		OnMusicStateChanged.Broadcast(MusicState);
		return;
	}

	// إيقاف أي موسيقى سابقة قبل التبديل
	if (MusicComponent && MusicComponent->IsPlaying())
	{
		MusicComponent->Stop();
	}

	// تشغيل الموسيقى (تتكرر)
	if (UWorld* World = GetWorld())
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(World, Music, MasterVolume, 1.f, 0.f, nullptr, true);
		if (MusicComponent)
		{
			MusicState = ERok2MusicState::Playing;
			OnMusicStateChanged.Broadcast(MusicState);
			UE_LOG(LogRok2Audio, Log, TEXT("Playing %s music for civ: %s"),
				MusicMode == ERok2MusicMode::Battle ? TEXT("battle") : TEXT("peace"), *CurrentCivId);
		}
	}
}

void URok2AudioManager::EnterBattleMode()
{
	if (MusicMode == ERok2MusicMode::Battle)
	{
		// مدّد مهلة العودة فقط
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BattleModeTimer);
			World->GetTimerManager().SetTimer(BattleModeTimer, this,
				&URok2AudioManager::ExitBattleMode, BattleModeTimeout, false);
		}
		return;
	}
	MusicMode = ERok2MusicMode::Battle;
	UE_LOG(LogRok2Audio, Log, TEXT("Entering battle music mode"));

	if (MusicState == ERok2MusicState::Playing)
	{
		PlayCurrentModeMusic();
	}

	// جدولة العودة التلقائية للسلام
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BattleModeTimer, this,
			&URok2AudioManager::ExitBattleMode, BattleModeTimeout, false);
	}
}

void URok2AudioManager::ExitBattleMode()
{
	if (MusicMode == ERok2MusicMode::Peace) return;
	MusicMode = ERok2MusicMode::Peace;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BattleModeTimer);
	}
	UE_LOG(LogRok2Audio, Log, TEXT("Exiting battle music mode — back to peace"));

	if (MusicState == ERok2MusicState::Playing)
	{
		PlayCurrentModeMusic();
	}
}

void URok2AudioManager::StopMusic()
{
	if (MusicComponent && MusicComponent->IsPlaying())
	{
		MusicComponent->Stop();
	}
	MusicState = ERok2MusicState::Stopped;
	OnMusicStateChanged.Broadcast(MusicState);
}

void URok2AudioManager::PlaySfx(ERok2AudioType Type)
{
	PlaySfxWithVolume(Type, MasterVolume);
}

void URok2AudioManager::PlaySfxWithVolume(ERok2AudioType Type, float Volume)
{
	if (!bAudioEnabled) return;

	const FString* Path = SfxPaths.Find(Type);
	if (!Path)
	{
		UE_LOG(LogRok2Audio, Verbose, TEXT("No SFX path for type: %d"), (int32)Type);
		return;
	}

	PlaySoundAtPath(*Path, Volume);
}

void URok2AudioManager::PlaySoundAtPath(const FString& Path, float Volume)
{
	USoundWave* Sound = LoadSound(Path);
	if (!Sound) return; // placeholder صامت

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::PlaySound2D(World, Sound, Volume);
	}
}
