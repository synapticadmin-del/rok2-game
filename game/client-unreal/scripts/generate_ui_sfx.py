#!/usr/bin/env python3
"""Generate original, deterministic UI SFX for ROK2 P6-T8.

The generator uses only elementary synthesis; no sampled or third-party audio is
embedded.  Run from any directory: output is always Content/Audio/sfx.
"""
from __future__ import annotations

import math
import random
import wave
from pathlib import Path

SAMPLE_RATE = 48_000
OUT_DIR = Path(__file__).resolve().parents[1] / "Content" / "Audio" / "sfx"


def clamp(value: float) -> float:
    return max(-0.96, min(0.96, value))


def write_wav(name: str, duration: float, signal) -> None:
    frames = int(duration * SAMPLE_RATE)
    samples = bytearray()
    for frame in range(frames):
        t = frame / SAMPLE_RATE
        value = clamp(signal(t, duration))
        integer = int(value * 32767.0)
        samples.extend(integer.to_bytes(2, byteorder="little", signed=True))
    path = OUT_DIR / name
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        output.writeframes(samples)


def sine(freq: float, t: float) -> float:
    return math.sin(2.0 * math.pi * freq * t)


def bell(t: float, start: float, freq: float, gain: float = 1.0) -> float:
    local = t - start
    if local < 0.0:
        return 0.0
    envelope = math.exp(-local * 5.6)
    partials = (
        0.72 * sine(freq, local)
        + 0.26 * sine(freq * 2.73, local)
        + 0.12 * sine(freq * 4.08, local)
    )
    return gain * envelope * partials


def pluck(t: float, start: float, freq: float, gain: float = 1.0) -> float:
    local = t - start
    if local < 0.0:
        return 0.0
    envelope = math.exp(-local * 7.8)
    return gain * envelope * (0.78 * sine(freq, local) + 0.17 * sine(freq * 2.0, local))


def wood_click(t: float, duration: float) -> float:
    random.seed(int(t * SAMPLE_RATE) + 17)
    envelope = math.exp(-t * 49.0)
    noise = (random.random() * 2.0 - 1.0) * 0.22
    resonance = 0.54 * sine(218.0, t) + 0.20 * sine(436.0, t)
    return envelope * (noise + resonance)


def panel_open(t: float, duration: float) -> float:
    return bell(t, 0.00, 523.25, 0.52) + bell(t, 0.10, 659.25, 0.42) + bell(t, 0.20, 783.99, 0.38)


def panel_close(t: float, duration: float) -> float:
    return pluck(t, 0.00, 659.25, 0.45) + pluck(t, 0.10, 523.25, 0.37) + pluck(t, 0.19, 392.00, 0.32)


def error(t: float, duration: float) -> float:
    def pulse(start: float) -> float:
        local = t - start
        if local < 0.0:
            return 0.0
        return math.exp(-local * 9.5) * (0.48 * sine(146.83, local) + 0.26 * sine(155.56, local))
    return pulse(0.0) + pulse(0.19)


def china_whisper(t: float, duration: float) -> float:
    return bell(t, 0.00, 659.25, 0.38) + bell(t, 0.18, 783.99, 0.34) + bell(t, 0.36, 987.77, 0.30)


def rome_whisper(t: float, duration: float) -> float:
    local = t
    attack = min(1.0, local * 16.0)
    envelope = attack * math.exp(-local * 2.8)
    brass = sine(220.0, local) + 0.46 * sine(440.0, local) + 0.19 * sine(660.0, local)
    return 0.31 * envelope * brass


def arabia_whisper(t: float, duration: float) -> float:
    bend = 440.0 + 18.0 * math.sin(2.0 * math.pi * 1.6 * t)
    envelope = min(1.0, t * 13.0) * math.exp(-t * 2.25)
    flute = 0.80 * sine(bend, t) + 0.16 * sine(bend * 2.0, t) + 0.08 * sine(bend * 3.0, t)
    return 0.31 * envelope * flute + pluck(t, 0.40, 523.25, 0.18)


def egypt_whisper(t: float, duration: float) -> float:
    return pluck(t, 0.00, 329.63, 0.32) + pluck(t, 0.16, 440.00, 0.29) + pluck(t, 0.31, 493.88, 0.24)


def vikings_whisper(t: float, duration: float) -> float:
    local = t
    envelope = min(1.0, local * 10.0) * math.exp(-local * 2.25)
    horn = sine(164.81, local) + 0.34 * sine(329.63, local) + 0.12 * sine(494.44, local)
    return 0.32 * envelope * horn + 0.12 * pluck(t, 0.36, 246.94)


def japan_whisper(t: float, duration: float) -> float:
    local = t
    freq = 587.33 - 72.0 * min(1.0, local / 0.62)
    envelope = min(1.0, local * 14.0) * math.exp(-local * 2.75)
    breath = 0.76 * sine(freq, local) + 0.12 * sine(freq * 2.0, local)
    return 0.29 * envelope * breath + bell(t, 0.45, 783.99, 0.16)


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    assets = (
        ("ui_button_click.wav", 0.16, wood_click),
        ("ui_panel_open.wav", 0.68, panel_open),
        ("ui_panel_close.wav", 0.52, panel_close),
        ("ui_error.wav", 0.52, error),
        ("ui_civ_whisper_china.wav", 0.92, china_whisper),
        ("ui_civ_whisper_rome.wav", 0.92, rome_whisper),
        ("ui_civ_whisper_arabia.wav", 0.92, arabia_whisper),
        ("ui_civ_whisper_egypt.wav", 0.92, egypt_whisper),
        ("ui_civ_whisper_vikings.wav", 0.92, vikings_whisper),
        ("ui_civ_whisper_japan.wav", 0.92, japan_whisper),
    )
    for name, duration, signal in assets:
        write_wav(name, duration, signal)
        print(f"wrote {name}")


if __name__ == "__main__":
    main()
