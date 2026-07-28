#!/usr/bin/env python3
"""
ROK2 — Procedural audio asset generator (P4-T2).

يولّد ملفات WAV حقيقية للموسيقى الخلفية لكل حضارة + المؤثرات الصوتية،
بأسلوب توليف إجرائي (additive synthesis + noise shaping) بلا مكتبات خارجية
غير numpy. المخرجات: Content/Audio/<civ>/music.wav و Content/Audio/sfx/*.wav
بصيغة 16-bit PCM 44.1kHz Mono — الصيغة القياسية لاستيراد Unreal Engine.

كل حضارة لها بصمة صوتية مستمدة من هويتها الموسيقية:
  rome    — سلم خماسي رسمي (fanfare نحاسية، دقات طبول رباعية وقورة)
  china   — سلم خماسي صيني (C-D-E-G-A)، نغمات "كوتشين" هادئة متداخلة
  arabia  — سلم حجاز (D-Eb-F#-G-A-Bb-C)، إيقاع مقسوم صحراوي متمايل
  egypt   — سلم فريجي (E-F-G-A-B-C-D)، نغمات ناي طويلة + قرع إيقاعي
  vikings — درون باس ثابت + سلم قروسطي قاسٍ (war horns) وطبول ثقيلة
  japan   — سلم ياباني (insen scale)، نغمات "كوتو/شاكوهاشي" متباعدة
"""
import numpy as np
import os
import struct
import wave

SR = 44100
OUT = "/agent/workspace/audio_gen/Content/Audio"


def write_wav(path, samples):
    """يكتب مصفوفة float [-1,1] كـ WAV 16-bit PCM."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    samples = np.clip(samples, -1.0, 1.0)
    pcm = (samples * 32767).astype(np.int16)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    print(f"  wrote {path} ({len(samples)/SR:.1f}s, {os.path.getsize(path)/1024:.0f} KB)")


def env_ad(n, a=0.01, d=0.3):
    """غلاف Attack-Decay."""
    n_a = max(1, int(a * SR))
    n_d = max(1, n - n_a)
    attack = np.linspace(0, 1, n_a)
    decay = np.exp(-np.linspace(0, 6, n_d))
    return np.concatenate([attack, decay])[:n]


def pluck(freq, dur, harmonics=5, brightness=0.6):
    """نغمة ذات هارمونيات متحللة (تشبه الوتر/النحاس حسب السطوع)."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    sig = np.zeros(n)
    for h in range(1, harmonics + 1):
        amp = brightness / (h ** 1.5)
        sig += amp * np.sin(2 * np.pi * freq * h * t)
    sig += 0.02 * np.random.default_rng(7).standard_normal(n)  # حبيبات
    return sig * env_ad(n, 0.005, dur * 0.8)


def flute_tone(freq, dur, vib_rate=5.0, vib_depth=0.006):
    """نغمة هوائية ناعمة (ناي/شاكوهاشي) مع فيبراتو."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    vib = 1 + vib_depth * np.sin(2 * np.pi * vib_rate * t)
    sig = np.sin(2 * np.pi * freq * vib * t)
    sig += 0.15 * np.sin(2 * np.pi * freq * 2 * vib * t)
    # ضجيج نفخة خفيف
    rng = np.random.default_rng(11)
    sig += 0.03 * rng.standard_normal(n) * np.sin(np.pi * t / dur) ** 2
    e = np.minimum(1, np.linspace(0, 8, n)) * np.exp(-np.maximum(0, t - dur * 0.85) * 8)
    return sig * e * 0.8


def drum(freq, dur, punch=4.0):
    """طبلة: جيبية متناقصة التردد + ضجيج."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    sweep = freq * np.exp(-t * punch * 2)
    sig = np.sin(2 * np.pi * np.cumsum(sweep) / SR)
    rng = np.random.default_rng(13)
    sig += 0.35 * rng.standard_normal(n) * np.exp(-t * punch * 8)
    return sig * np.exp(-t * punch) * 0.9


def chord(freqs, dur, kind="pluck"):
    """طبقات عدة نغمات."""
    n = int(dur * SR)
    out = np.zeros(n)
    for f in freqs:
        src = pluck(f, dur) if kind == "pluck" else flute_tone(f, dur)
        out[: len(src)] += src * 0.5
    return out


def mix(*tracks):
    m = max(len(t) for t in tracks)
    out = np.zeros(m)
    for t in tracks:
        out[: len(t)] += t
    peak = np.max(np.abs(out))
    return out / peak * 0.85 if peak > 0 else out


def loop_fade(sig, fade_s=1.0):
    """تلاشي نهاية المقطع ليكرر بسلاسة."""
    n = int(fade_s * SR)
    sig[-n:] *= np.linspace(1, 0.05, n)
    return sig


def paste(dst, seg, start_s, gain=1.0):
    """يضيف مقطعاً داخل المسار عند زمن معين مع قصّ آمن للحواف."""
    i = int(start_s * SR)
    if i >= len(dst):
        return
    end = min(len(dst), i + len(seg))
    dst[i:end] += seg[: end - i] * gain


# ----------------------------------------------------------------------------
# موسيقى الحضارات — حلقات خلفية 20-24 ثانية
# ----------------------------------------------------------------------------

def music_rome():
    """فنفار روماني: طبول رباعية + نغمات نحاسية خماسية وقورة."""
    bpm, dur = 96, 22.0
    n = int(dur * SR)
    beat = 60.0 / bpm
    base = np.zeros(n)
    # طبول في كل نبضة + أوف-بيت خفيف
    t = 0.0
    while t < dur:
        paste(base, drum(85, 0.35, 5), t, 0.8)
        paste(base, drum(160, 0.15, 8), t + beat / 2, 0.25)
        t += beat
    # لحن نحاسي خماسي (A-C-D-E-G) — جمل وقورة كل ثانيتين
    melody = np.zeros(n)
    notes = [220.0, 261.63, 293.66, 329.63, 392.0, 329.63, 293.66, 261.63]
    for k, f in enumerate(notes * 2):
        paste(melody, chord([f, f * 1.5], beat * 1.8, "flute") * 0.55, k * beat * 2)
    return loop_fade(mix(base, melody), 1.5)


def music_china():
    """كوتشين صيني: سلم خماسي C-D-E-G-A، نغمات متداخلة هادئة."""
    dur = 24.0
    n = int(dur * SR)
    base = np.zeros(n)
    scale = [261.63, 293.66, 329.63, 392.0, 440.0]
    rng = np.random.default_rng(42)
    t = 0.0
    while t < dur - 1:
        f = scale[rng.integers(0, 5)]
        octv = 2.0 if rng.random() > 0.6 else 1.0
        paste(base, pluck(f * octv, 1.6, 7, 0.8), t, 0.5)
        t += 0.5 + rng.random() * 0.5
    # طبقة درون خفيفة
    drone = flute_tone(130.81, dur, 3.0, 0.003) * 0.12
    return loop_fade(mix(base, drone), 2.0)


def music_arabia():
    """حجاز عربي: سلم D-Eb-F#-G، إيقاع مقسوم متمايل + ناي."""
    dur = 22.0
    n = int(dur * SR)
    base = np.zeros(n)
    beat = 60.0 / 110
    # إيقاع مقسوم: دوم-تك-تك (دربوكة تقريبية)
    t = 0.0
    while t < dur:
        for off, amp, fr in [(0, 0.7, 100), (beat * 0.5, 0.3, 200), (beat * 0.75, 0.3, 200)]:
            paste(base, drum(fr, 0.2, 7), t + off, amp)
        t += beat
    # ناي: سلم حجاز D-Eb-F#-G-A-Bb
    hijaz = [293.66, 311.13, 369.99, 392.0, 440.0, 466.16]
    mel = np.zeros(n)
    phrase = [0, 1, 2, 3, 2, 1, 2, 3, 4, 5, 4, 3]
    for k, idx in enumerate(phrase * 2):
        f = hijaz[idx % len(hijaz)]
        paste(mel, flute_tone(f, beat * 1.7), k * beat * 1.5, 0.45)
    return loop_fade(mix(base, mel), 1.5)


def music_egypt():
    """فريجي مصري: ناي طويل + قرع إيقاعي هيبطي."""
    dur = 23.0
    n = int(dur * SR)
    base = np.zeros(n)
    beat = 60.0 / 88
    t = 0.0
    while t < dur:
        paste(base, drum(70, 0.4, 4), t, 0.7)
        paste(base, drum(180, 0.12, 9), t + beat * 0.75, 0.2)
        t += beat * 2
    phrygian = [329.63, 349.23, 392.0, 440.0, 493.88, 523.25]
    mel = np.zeros(n)
    seq = [0, 1, 0, 2, 3, 2, 1, 0, 4, 3, 2, 1]
    for k, idx in enumerate(seq * 2):
        paste(mel, flute_tone(phrygian[idx % 6], beat * 2.5, 4.5, 0.008), k * beat * 2, 0.5)
    return loop_fade(mix(base, mel), 2.0)


def music_vikings():
    """فايكنج: درون باس قاسٍ + أبواق حرب + طبول ثقيلة بطيئة."""
    dur = 22.0
    n = int(dur * SR)
    t_arr = np.arange(n) / SR
    # درون E2 + خامسة
    drone = np.sin(2 * np.pi * 82.41 * t_arr) * 0.3 + np.sin(2 * np.pi * 123.47 * t_arr) * 0.15
    drone += 0.02 * np.random.default_rng(5).standard_normal(n)
    base = np.zeros(n)
    beat = 60.0 / 70
    t = 0.0
    while t < dur:
        paste(base, drum(60, 0.5, 3.5), t, 0.85)
        t += beat * 2
    # بوق حرب: نغمة طويلة قاسية كل 5.5 ثانية
    mel = np.zeros(n)
    for k, f in enumerate([164.81, 196.0, 220.0, 196.0]):
        paste(mel, pluck(f, beat * 6, 8, 0.9), k * 5.5, 0.55)
    return loop_fade(mix(drone, base, mel), 1.5)


def music_japan():
    """ياباني: كوتو متباعد + شاكوهاشي طويل (insen scale)."""
    dur = 24.0
    n = int(dur * SR)
    base = np.zeros(n)
    insen = [246.94, 261.63, 329.63, 349.23, 392.0]  # B-C-E-F-G
    rng = np.random.default_rng(3)
    t = 0.0
    while t < dur - 2:
        f = insen[rng.integers(0, 5)]
        paste(base, pluck(f, 2.2, 6, 0.7), t, 0.45)
        t += 1.2 + rng.random() * 1.3
    mel = np.zeros(n)
    for k, f in enumerate([493.88, 440.0, 392.0, 523.25, 440.0]):
        paste(mel, flute_tone(f, 3.5, 4.0, 0.007), k * 4.4, 0.3)
    return loop_fade(mix(base, mel), 2.0)


# ----------------------------------------------------------------------------
# المؤثرات الصوتية
# ----------------------------------------------------------------------------

def sfx_build_complete():
    """طرقتان + رنين إنجاز صاعد."""
    n = int(1.2 * SR)
    out = np.zeros(n)
    paste(out, drum(140, 0.15, 9), 0.0, 0.7)
    paste(out, drum(140, 0.15, 9), 0.12, 0.7)
    paste(out, chord([523.25, 659.25, 783.99], 0.9), 0.25, 0.5)
    return out


def sfx_upgrade():
    """تأرجح صاعد ذهبي (احتفال قصير)."""
    notes = [392.0, 493.88, 587.33, 783.99]
    n = int(1.4 * SR)
    out = np.zeros(n)
    for k, f in enumerate(notes):
        paste(out, pluck(f, 0.5, 6, 0.9), k * 0.16, 0.6)
    return out


def sfx_victory():
    """فنفار نصر: 4 نغمات صاعدة قوية."""
    n = int(2.0 * SR)
    out = np.zeros(n)
    for k, f in enumerate([261.63, 329.63, 392.0, 523.25]):
        paste(out, chord([f, f * 1.25], 0.7, "flute"), k * 0.3, 0.7)
    paste(out, drum(90, 0.4, 5), 0.0, 0.6)
    return out


def sfx_defeat():
    """هزيمة: نغمتان هابطتان + طبلة خافتة."""
    n = int(1.8 * SR)
    out = np.zeros(n)
    paste(out, flute_tone(392.0, 1.1, 3.0, 0.004), 0.0, 0.6)
    paste(out, flute_tone(311.13, 1.1, 3.0, 0.004), 0.5, 0.6)
    paste(out, drum(70, 0.6, 3), 1.0, 0.5)
    return out


def sfx_march_start():
    """بوق انطلاق + وقع خطوات سريع."""
    n = int(1.0 * SR)
    out = np.zeros(n)
    paste(out, pluck(220.0, 0.8, 8, 0.95), 0.0, 0.7)
    for k in range(3):
        paste(out, drum(120, 0.08, 12), 0.3 + k * 0.18, 0.35)
    return out


def sfx_button_click():
    """نقرة خشبية قصيرة."""
    n = int(0.12 * SR)
    t = np.arange(n) / SR
    sig = np.sin(2 * np.pi * 800 * t) * np.exp(-t * 40)
    rng = np.random.default_rng(2)
    sig += 0.3 * rng.standard_normal(n) * np.exp(-t * 60)
    return sig * 0.6


def sfx_notification():
    """جرس ناعم ثنائي."""
    n = int(0.9 * SR)
    out = np.zeros(n)
    for k, f in enumerate([880.0, 1174.66]):
        seg = pluck(f, 0.6, 4, 0.5) * 0.5
        i = int(k * 0.22 * SR); out[i:i+len(seg)] += seg
    return out


if __name__ == "__main__":
    print("== ROK2 audio generation ==")
    civs = {
        "rome": music_rome, "china": music_china, "arabia": music_arabia,
        "egypt": music_egypt, "vikings": music_vikings, "japan": music_japan,
    }
    for civ, fn in civs.items():
        print(f"[{civ}]")
        write_wav(f"{OUT}/{civ}/music.wav", fn())
    sfx = {
        "build_complete": sfx_build_complete, "upgrade": sfx_upgrade,
        "victory": sfx_victory, "defeat": sfx_defeat,
        "march_start": sfx_march_start, "button_click": sfx_button_click,
        "notification": sfx_notification,
    }
    for name, fn in sfx.items():
        print(f"[sfx/{name}]")
        write_wav(f"{OUT}/sfx/{name}.wav", fn())
    print("done.")
