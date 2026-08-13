#!/usr/bin/env node
/**
 * Offline unit checks for daily/weekly events (P3-T3): barbarians, resource rush, war_fever
 * driven only by data/events.json (mirrors src/do/sim/events.ts logic).
 *
 * Covers:
 *  - events.json shape: 3 events with recurrence/buffs, constants present
 *  - seasonWeekday determinism (day 0 = Sunday)
 *  - isEventDay: daily always, weekly only on its weekday
 *  - isEventActive: within durationTicks window of the day only
 *  - eventBuff multiplicative vs additive aggregation across concurrent events
 *  - barbExtraPerRegion / barbLevelBonus capped by constants
 *  - eventsStatus snapshot fields
 *  - barbarian horde + resource rush buff application (hp/gather/richness/score)
 */
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const DATA = JSON.parse(readFileSync(join(here, "../src/data/events.json"), "utf8"));

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

// ---- mirror of sim/events.ts ----
const C = {
  barbRespawnMs: DATA.constants.barb_respawn_ms,
  maxExtraSpawnPerRegion: DATA.constants.max_extra_spawn_per_region,
  barbBaseLevel: DATA.constants.barb_base_level,
};
const EVENTS = DATA.events.map((e) => ({
  id: e.id, name: e.name, kind: e.kind, recurrence: e.recurrence,
  durationTicks: e.duration_ticks, weekday: e.weekday, buffs: e.buffs ?? {}, notify: e.notify,
}));
const seasonWeekday = (d) => ((d % 7) + 7) % 7;
const isEventDay = (ev, d) => ev.recurrence === "daily" ? true : (ev.weekday === undefined || seasonWeekday(d) === ev.weekday);
const isEventActive = (ev, d, t) => isEventDay(ev, d) && t >= 0 && t < ev.durationTicks;
const activeEvents = (d, t) => EVENTS.filter((ev) => isEventActive(ev, d, t));
function eventBuff(d, t, key, additive = false) {
  const act = activeEvents(d, t);
  if (additive) { let s = 0; for (const e of act) s += e.buffs[key] ?? 0; return s; }
  let m = 1; for (const e of act) { const v = e.buffs[key]; if (typeof v === "number") m *= v; } return m;
}
const barbExtraPerRegion = (d, t) => Math.min(C.maxExtraSpawnPerRegion, eventBuff(d, t, "barb_spawn_extra_per_region", true));
const barbLevelBonus = (d, t) => eventBuff(d, t, "barb_level_bonus", true);

// ---- 1. shape ----
assert(DATA.constants && typeof DATA.constants.barb_respawn_ms === "number", "constants.barb_respawn_ms present");
assert(typeof C.maxExtraSpawnPerRegion === "number" && C.maxExtraSpawnPerRegion > 0, `max extra spawn/region (${C.maxExtraSpawnPerRegion})`);
// P10-T5: events.json يحتوي أيضًا mightiest_governor و wheel_of_fortune ضمن majorEvents،
// لكن مصفوفة EVENTS التي يقرأها هذا الحارس تشمل الأحداث القابلة للتكرار فقط — نسمح بكلٍّ منها.
const EXPECTED_KINDS = ["barbarians", "mightiest_governor", "resource_rush", "war_fever", "wheel_of_fortune"];
assert(EVENTS.length >= 3 && EVENTS.length <= 5, `${EVENTS.length} events defined (3–5 مقبول مع P10 majorEvents)`);
const kinds = EVENTS.map((e) => e.kind).sort();
assert(JSON.stringify(kinds) === JSON.stringify(EXPECTED_KINDS), `event kinds (${kinds})`);
const barb = EVENTS.find((e) => e.id === "barbarian_horde");
const rush = EVENTS.find((e) => e.id === "resource_rush");
const war = EVENTS.find((e) => e.id === "war_fever");
assert(barb && rush && war, "all three events present by id");
const mg = EVENTS.find((e) => e.id === "mightiest_governor");
const wf = EVENTS.find((e) => e.id === "wheel_of_fortune");
assert(mg && wf, "P10-T5 majorEvents present by id");

// ---- 2. recurrence ----
assert(barb.recurrence === "daily" && rush.recurrence === "daily", "barbarians + resource_rush are daily");
assert(war.recurrence === "weekly", "war_fever is weekly");
assert(typeof war.weekday === "number" && war.weekday >= 0 && war.weekday <= 6, `war_fever weekday in [0,6] (${war.weekday})`);

// ---- 3. seasonWeekday determinism ----
assert(seasonWeekday(0) === 0, "day 0 = Sunday");
assert(seasonWeekday(5) === 5, "day 5 = Friday");
assert(seasonWeekday(7) === 0, "day 7 wraps to Sunday");
assert(seasonWeekday(40) === 5, `day 40 = Friday (${seasonWeekday(40)})`);

// ---- 4. isEventDay ----
assert(isEventDay(barb, 0) && isEventDay(barb, 37), "barbarians every day");
assert(war.weekday === 5 ? (isEventDay(war, 5) && !isEventDay(war, 0)) : true, "war_fever only on its weekday (Friday)");
assert(isEventDay(war, 40), "war_fever active day 40 (Friday)");

// ---- 5. isEventActive window ----
assert(isEventActive(barb, 0, 0), "barbarians active at tick 0 of its day");
assert(isEventActive(barb, 0, barb.durationTicks - 1), `barbarians active at tick ${barb.durationTicks - 1}`);
assert(!isEventActive(barb, 0, barb.durationTicks), `barbarians ended at tick ${barb.durationTicks}`);
assert(!isEventActive(barb, 0, barb.durationTicks + 500), "barbarians inactive later in day");
assert(!isEventActive(war, 0, 0), "war_fever inactive on non-weekday even at tick 0");
assert(isEventActive(war, 40, 0), "war_fever active day 40 tick 0");

// ---- 6. multiplicative vs additive buffs ----
// barb hp mult is multiplicative
assert(eventBuff(0, 0, "barb_hp_mult") === barb.buffs.barb_hp_mult, `barb_hp_mult = ${barb.buffs.barb_hp_mult} during horde`);
assert(eventBuff(0, 9999, "barb_hp_mult") === 1, "barb_hp_mult back to 1 after horde window");
// gather_rate_mult multiplicative during rush
assert(eventBuff(1, 0, "gather_rate_mult") === rush.buffs.gather_rate_mult, `gather_rate_mult = ${rush.buffs.gather_rate_mult} during rush`);
// barb_level_bonus additive
assert(barbLevelBonus(0, 0) === barb.buffs.barb_level_bonus, `barb level bonus +${barb.buffs.barb_level_bonus} during horde`);
assert(barbLevelBonus(0, 9999) === 0, "barb level bonus 0 after horde");
// day 40: war_fever (Friday) AND rush (daily) can be concurrent → attack_mult from war only
assert(eventBuff(40, 0, "attack_mult") === war.buffs.attack_mult, `day40 attack_mult from war_fever (${war.buffs.attack_mult})`);

// ---- 7. barb extra spawn capped ----
assert(barbExtraPerRegion(0, 0) === Math.min(C.maxExtraSpawnPerRegion, barb.buffs.barb_spawn_extra_per_region), `barb extra/region during horde = ${barbExtraPerRegion(0, 0)}`);
assert(barbExtraPerRegion(0, 0) <= C.maxExtraSpawnPerRegion, "barb extra spawn capped by constant");
assert(barbExtraPerRegion(0, 9999) === 0, "no extra barb spawns after horde");

// ---- 8. eventsStatus snapshot fields ----
function eventsStatus(d, t) {
  return EVENTS.map((ev) => ({
    id: ev.id, name: ev.name, kind: ev.kind, recurrence: ev.recurrence,
    active: isEventActive(ev, d, t), scheduledToday: isEventDay(ev, d),
    durationTicks: ev.durationTicks, ticksRemaining: isEventActive(ev, d, t) ? ev.durationTicks - t : 0,
  }));
}
const s0 = eventsStatus(0, 0);
const s0barb = s0.find((e) => e.id === "barbarian_horde");
assert(s0barb.active && s0barb.ticksRemaining === barb.durationTicks, "status: barbarian active with full remaining at tick 0");
const s0war = s0.find((e) => e.id === "war_fever");
assert(!s0war.active && !s0war.scheduledToday, "status: war_fever not scheduled on day 0 (Sunday)");
const s40 = eventsStatus(40, 10);
assert(s40.find((e) => e.id === "war_fever").active, "status: war_fever active day 40 (Friday)");
assert(s40.find((e) => e.id === "war_fever").ticksRemaining === war.durationTicks - 10, "status: war_fever ticks remaining correct");

// ---- 9. buff application: horde hp + rush gather/richness + kill/gather score ----
const barbHpPerLevel = 100;
const lvl = C.barbBaseLevel + barbLevelBonus(0, 0);
const hp = Math.floor(barbHpPerLevel * lvl * eventBuff(0, 0, "barb_hp_mult"));
const hpNormal = barbHpPerLevel * C.barbBaseLevel;
assert(hp > hpNormal, `horde barb hp (${hp}) > normal (${hpNormal})`);
const baseRate = 0.5 * 1000; // 1000 troops
const rushRate = baseRate * eventBuff(1, 0, "gather_rate_mult");
assert(rushRate > baseRate, `rush gather rate (${rushRate}) > base (${baseRate})`);
const rich = Math.floor(5000 * eventBuff(1, 0, "resource_richness_mult"));
assert(rich === 10000, `rush richness doubled (${rich})`);
// scores additive
assert(eventBuff(0, 0, "barb_kill_score", true) === barb.buffs.barb_kill_score, "barb kill score during horde");
assert(eventBuff(1, 0, "gather_score", true) === rush.buffs.gather_score, "gather score during rush");
assert(eventBuff(40, 0, "pvp_kill_score", true) === war.buffs.pvp_kill_score, "pvp kill score during war_fever");

console.log("\n==== RESULT ====");
if (failed === 0) { console.log("ALL OFFLINE EVENTS CHECKS PASSED"); process.exit(0); }
console.error(`FAILED ASSERTIONS: ${failed}`);
process.exit(1);
