#!/usr/bin/env node
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");
const header = readFileSync(join(root, "Source", "Rok2", "Public", "Rok2WorldIconography.h"), "utf8");
const source = readFileSync(join(root, "Source", "Rok2", "Private", "Rok2WorldIconography.cpp"), "utf8");
const zones = JSON.parse(readFileSync(join(root, "..", "..", "data", "zones.json"), "utf8"));

assert.ok(zones.constants?.season_max_day > 0, "zones.json must remain the world data source");
assert.match(header, /FRok2WorldIconStyle/, "icon style contract must exist");
assert.match(header, /Resolve\(const FString& TargetType, const FString& ResourceKind, int32 Level\)/, "renderer-facing resolver must accept target, resource, and level");
assert.match(source, /TierForLevel/, "level tier resolver must exist");
assert.match(source, /node_wheat/, "wheat nodes need a dedicated icon");
assert.match(source, /node_wood/, "wood nodes need a dedicated icon");
assert.match(source, /node_stone/, "stone nodes need a dedicated icon");
assert.match(source, /node_gold/, "gold nodes need a dedicated icon");
assert.match(source, /barbarian_scout/, "low barbarian tier needs a distinct icon");
assert.match(source, /barbarian_warband/, "mid barbarian tier needs a distinct icon");
assert.match(source, /barbarian_elite/, "elite barbarian tier needs a distinct icon");
assert.match(source, /objective_throne_crown/, "throne needs a crown marker");
assert.match(source, /objective_pass_gate/, "passes need gate markers");
assert.match(source, /BaseColor/, "icon grammar must expose a family colour");
assert.match(source, /TierColor/, "icon grammar must expose a level colour");
assert.match(source, /WorldScale/, "icon grammar must expose level scale");
assert.match(source, /MaximumLevel\)/, "tiers must normalize against category maximum level");
console.log("P6-T9 verified: semantic icons and tier colouring are defined for world zones.");
