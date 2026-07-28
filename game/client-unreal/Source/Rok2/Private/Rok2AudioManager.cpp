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

	// مسارات المؤثرات (مشتركة لكل الحضارات، أو مخصصة لاحقاً)
	SfxPaths.Empty();
	SfxPaths.Add(ERok2AudioType::BuildComplete,  TEXT("Audio/sfx/build_complete"));
	SfxPaths.Add(ERok2AudioType::Upgrade,        TEXT("Audio/sfx/upgrade"));
	SfxPaths.Add(ERok2AudioType::BattleVictory,  TEXT("Audio/sfx/victory"));
	SfxPaths.Add(ERok2AudioType::BattleDefeat,   TEXT("Audio/sfx/defeat"));
	SfxPaths.Add(ERok2AudioType::MarchStart,     TEXT("Audio/sfx/march_start"));
	SfxPaths.Add(ERok2AudioType::ButtonClick,    TEXT("Audio/sfx/button_click"));
	SfxPaths.Add(ERok2AudioType::Notification,   TEXT("Audio/sfx/notification"));
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

	const FString* Path = MusicPaths.Find(CurrentCivId);
	if (!Path)
	{
		UE_LOG(LogRok2Audio, Warning, TEXT("No music path for civ: %s"), *CurrentCivId);
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

	// تشغيل الموسيقى (تتكرر)
	if (UWorld* World = GetWorld())
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(World, Music, MasterVolume, 1.f, 0.f, nullptr, true);
		if (MusicComponent)
		{
			MusicState = ERok2MusicState::Playing;
			OnMusicStateChanged.Broadcast(MusicState);
			UE_LOG(LogRok2Audio, Log, TEXT("Playing music for civ: %s"), *CurrentCivId);
		}
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
