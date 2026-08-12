#!/usr/bin/env node
import assert from "node:assert/strict";
import { existsSync, readFileSync, statSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");
const sfxDir = join(root, "Content", "Audio", "sfx");
const artHeader = readFileSync(join(root, "Source", "Rok2", "Public", "Rok2ArtAssets.h"), "utf8");
const artSource = readFileSync(join(root, "Source", "Rok2", "Private", "Rok2ArtAssets.cpp"), "utf8");

const shared = ["ui_button_click.wav", "ui_panel_open.wav", "ui_panel_close.wav", "ui_error.wav"];
const civilizations = ["arabia", "china", "egypt", "japan", "rome", "vikings"];
const expected = [...shared, ...civilizations.map((civ) => `ui_civ_whisper_${civ}.wav`)];

for (const name of expected) {
  const path = join(sfxDir, name);
  assert.ok(existsSync(path), `missing P6-T8 asset: ${name}`);
  const wav = readFileSync(path);
  assert.ok(wav.length > 44, `${name} must contain PCM audio beyond its WAV header`);
  assert.equal(wav.subarray(0, 4).toString("ascii"), "RIFF", `${name} must be RIFF`);
  assert.equal(wav.subarray(8, 12).toString("ascii"), "WAVE", `${name} must be WAVE`);
  assert.ok(statSync(path).size < 300_000, `${name} must remain a short UI effect`);
}

for (const id of ["button_click", "panel_open", "panel_close", "error"]) {
  assert.match(artHeader, new RegExp(`GetUiSfxAssetPath|HasUiSfx`), "P6-T8 must publish a UI SFX resolver");
  assert.match(artSource, new RegExp(`Audio/sfx/ui_${id}`), `catalog missing ${id}`);
}
for (const civ of civilizations) {
  assert.match(artSource, new RegExp(`Audio/sfx/ui_civ_whisper_${civ}`), `catalog missing civilization whisper ${civ}`);
}
assert.match(artHeader, /GetCivilizationWhisperAssetPath/, "civilization whisper resolver must be public");
console.log(`P6-T8 verified: ${shared.length} shared UI SFX and ${civilizations.length} civilization whispers.`);
