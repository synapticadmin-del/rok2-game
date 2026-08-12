#!/usr/bin/env node
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");
const shard = readFileSync(join(root, "..", "backend", "src", "do", "KingdomShard.ts"), "utf8");
const header = readFileSync(join(root, "Source", "Rok2", "Public", "Rok2SeasonStoryWidget.h"), "utf8");
const widget = readFileSync(join(root, "Source", "Rok2", "Private", "Rok2SeasonStoryWidget.cpp"), "utf8");

assert.match(shard, /CREATE TABLE IF NOT EXISTS season_story_events/, "season story needs durable storage");
assert.match(shard, /INSERT OR IGNORE INTO _sql_schema_migrations \(id\) VALUES \(8\)/, "season story needs a forward-only migration");
assert.match(shard, /seasonStory: this\.seasonStory/, "public snapshot must include the story timeline");
assert.match(shard, /type: "season_story_event"/, "new milestones must be broadcast live");
assert.match(shard, /kind: "region_unlocked"/, "zone unlock must become a story event");
assert.match(shard, /kind: previousPassOwnerAllianceId \? "pass_conquered" : "first_pass_capture"/, "pass ownership must distinguish first capture from conquest");
assert.match(shard, /kind: "throne_captured"/, "throne conquest must become a story event");
assert.match(shard, /kind: "season_champion"/, "season winner must be recorded");
assert.match(shard, /rightScore - leftScore \|\| leftId\.localeCompare\(rightId\)/, "champion ties must resolve deterministically");
assert.match(shard, /this\.seasonStory = this\.seasonStory\.slice\(-120\)/, "timeline must be bounded");

assert.match(header, /class ROK2_API URok2SeasonStoryWidget/, "dedicated season story widget must exist");
assert.match(header, /SetStoryEvents/, "widget must receive a snapshot timeline");
assert.match(header, /AddStoryEvent/, "widget must accept live story events");
assert.match(widget, /حكاية المملكة/, "widget needs the narrative title");
assert.match(widget, /تتويج بطل الموسم/, "widget needs the visual champion crown");
assert.match(widget, /RebuildTimeline/, "widget needs a visual timeline renderer");
assert.match(widget, /pass_conquered/, "widget must label alliance war events");
assert.match(widget, /throne_captured/, "widget must label throne events");
console.log("P6-T10 verified: public season milestones, timeline, and winner coronation are wired by contract.");
