#!/usr/bin/env node
/**
 * ROK2 — P7-T4: موسم اختبار مسرّع وتتويج.
 *
 * مشغّل E2E مخصص لبيئة محلية/اختبار معزولة. لا يغيّر zones.json أو اقتصاد
 * وميزان القتال؛ بل يستعمل واجهة الإدارة المحمية set-time لزيارة محطات الموسم
 * القانونية: 0، 10، 14، 35، 40، 60.
 *
 * التشغيل: ADMIN_KEY=... node scripts/e2e_p7_t4_accelerated_season.mjs
 */

const BASE = process.env.BASE_URL || "http://127.0.0.1:8787";
const ADMIN = process.env.ADMIN_KEY;
if (!ADMIN) {
  console.error("ADMIN_KEY is not set. Run: ADMIN_KEY=your-key node scripts/e2e_p7_t4_accelerated_season.mjs");
  process.exit(1);
}

const PROFILE = Object.freeze({
  days: Object.freeze({ foundation: 0, zone2: 10, zone2Complete: 14, coreApproach: 35, contest: 40, finish: 60 }),
  productionDayMs: 86_400_000,
  productionMaxDay: 60,
});

let failed = 0;
const assert = (condition, message) => {
  if (condition) console.log("OK  :", message);
  else { failed += 1; console.error("FAIL:", message); }
};

async function req(path, { method = "GET", token, body, admin = false } = {}) {
  const headers = { "content-type": "application/json" };
  if (token) headers.authorization = `Bearer ${token}`;
  if (admin) headers["x-admin-key"] = ADMIN;
  const response = await fetch(`${BASE}${path}`, { method, headers, body: body === undefined ? undefined : JSON.stringify(body) });
  const text = await response.text();
  let data;
  try { data = JSON.parse(text); } catch { data = { raw: text }; }
  return { status: response.status, data };
}

function must(value, message) {
  assert(Boolean(value), message);
  if (!value) throw new Error(`Cannot continue: ${message}`);
  return value;
}

async function forceTicks(count = 2) {
  for (let i = 0; i < count; i += 1) {
    const tick = await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    assert(tick.status === 200, `forced season tick ${i + 1}/${count}`);
  }
}

async function setSeasonDay(day) {
  const set = await req("/v1/admin/set-time", { method: "POST", admin: true, body: { day } });
  must(set.status === 200 && set.data?.seasonDay === day, `admin fixture sets season day ${day}`);
  await forceTicks(2);
}

async function worldSnapshot(token) {
  const res = await req("/v1/world/snapshot", { token });
  must(res.status === 200, "authenticated world snapshot is available");
  return res.data;
}

async function createPlayer(run, suffix, name, civ) {
  const guest = await req("/v1/auth/guest", { method: "POST", body: { deviceId: `p7t4-${suffix}-${run}`, name } });
  const token = must(guest.status === 200 && guest.data?.token, `${name}: guest authentication succeeds`);
  const founded = await req("/v1/city/init", { method: "POST", token, body: { civ, name } });
  must(founded.status === 200 && founded.data?.player?.id && founded.data?.token, `${name}: FTUE city foundation succeeds`);
  return { id: founded.data.player.id, token: founded.data.token, name };
}

async function createAlliance(player, run, label) {
  const created = await req("/v1/alliance/create", {
    method: "POST", token: player.token,
    body: { name: `${label} Season Alliance`, tag: `${label[0]}${String(run).slice(-3)}` },
  });
  return must(created.status === 200 && created.data?.alliance?.id, `${label}: creates an independent alliance`) && created.data.alliance.id;
}

const feature = (state, id) => (state?.season?.features || []).find((entry) => entry.feature === id);
const zone = (state, id) => (state?.zones || []).find((entry) => entry.regionId === id);

async function sendObjectiveMarch(player, objectiveId) {
  const result = await req("/v1/world/march", {
    method: "POST", token: player.token,
    body: { targetType: "core_objective", coreObjectiveId: objectiveId, troops: { infantry_t1: 1000, archer_t1: 500, cavalry_t1: 250 } },
  });
  return must(result.status === 200 && result.data?.march?.id, `${player.name}: launches a core-objective march`) && result.data.march.id;
}

async function main() {
  console.log(`ROK2 P7-T4 accelerated isolated season against ${BASE}`);
  const health = await req("/v1/health");
  must(health.status === 200 && health.data?.ok, "server health endpoint is available");

  const run = Date.now();
  const alpha = await createPlayer(run, "alpha", "Alpha Season", "rome");
  const bravo = await createPlayer(run, "bravo", "Bravo Season", "china");
  const alphaAllianceId = await createAlliance(alpha, run, "Alpha");
  const bravoAllianceId = await createAlliance(bravo, run, "Bravo");
  assert(alphaAllianceId !== bravoAllianceId, "two distinct alliances participate in the season");

  for (const player of [alpha, bravo]) {
    const grant = await req("/v1/admin/grant", {
      method: "POST", admin: true,
      body: { playerId: player.id, food: 250000, wood: 250000, troops: { infantry_t1: 2500, archer_t1: 1500, cavalry_t1: 750 } },
    });
    assert(grant.status === 200, `${player.name}: isolated troop fixture grant succeeds`);
  }

  const schedule = await req("/v1/season/schedule", { token: alpha.token });
  must(schedule.status === 200, "season schedule endpoint is available");
  assert(schedule.data?.dayMs === PROFILE.productionDayMs, "production season day length remains 86,400,000 ms");
  assert(schedule.data?.maxDay === PROFILE.productionMaxDay, "production season maximum remains day 60");
  assert(schedule.data?.throne?.unlockDay === PROFILE.days.contest, "schedule exposes throne unlock at production day 40");

  await setSeasonDay(PROFILE.days.foundation);
  let state = await worldSnapshot(alpha.token);
  assert(state.seasonDay === 0, "season begins at day 0");
  assert(feature(state, "zone1_all")?.unlocked === true, "day 0 unlocks the Zone 1 foundation area");
  assert(feature(state, "zone2_half_inner_passes")?.unlocked === false, "day 0 keeps Zone 2 locked");
  assert(state.season?.throneUnlocked === false, "throne is locked at the foundation stage");

  await setSeasonDay(PROFILE.days.zone2);
  state = await worldSnapshot(alpha.token);
  assert(feature(state, "zone2_half_inner_passes")?.unlocked === true, "day 10 unlocks the first Zone 2 route");
  assert(zone(state, "CORE")?.unlocked === false, "CORE remains locked before final-gate day");

  await setSeasonDay(PROFILE.days.zone2Complete);
  state = await worldSnapshot(alpha.token);
  assert(feature(state, "zone2_all")?.unlocked === true, "day 14 unlocks all Zone 2 routes");
  const zone2Passes = (schedule.data?.passes || []).filter((pass) => [PROFILE.days.zone2, PROFILE.days.zone2Complete].includes(pass.unlockDay));
  assert(zone2Passes.length === 8 && zone2Passes.every((pass) => state.seasonDay >= pass.unlockDay), "day 14 exposes every Zone 1→2 pass");

  await setSeasonDay(PROFILE.days.coreApproach);
  state = await worldSnapshot(alpha.token);
  assert(feature(state, "zone3_outer")?.unlocked === true, "day 35 unlocks the Zone 3 approach");
  assert(zone(state, "CORE")?.unlocked === true, "day 35 unlocks the CORE region");
  const finalGates = (schedule.data?.passes || []).filter((pass) => pass.unlockDay === PROFILE.days.coreApproach);
  assert(finalGates.length === 4 && finalGates.every((pass) => state.seasonDay >= pass.unlockDay), "day 35 opens all four final gates");

  await setSeasonDay(PROFILE.days.contest);
  state = await worldSnapshot(alpha.token);
  assert(state.season?.throneUnlocked === true, "day 40 unlocks the throne");
  assert(feature(state, "zone3_core_windows")?.unlocked === true, "day 40 unlocks the core contest milestone");
  const altars = (state.coreObjectives || []).filter((objective) => objective.kind === "side_altar" && !objective.ownerAllianceId);
  must(altars.length >= 2, "two neutral side altars are available for an equal-score test");

  await Promise.all([sendObjectiveMarch(alpha, altars[0].id), sendObjectiveMarch(bravo, altars[1].id)]);
  await forceTicks(3);
  state = await worldSnapshot(alpha.token);
  const alphaAltar = (state.coreObjectives || []).find((objective) => objective.id === altars[0].id);
  const bravoAltar = (state.coreObjectives || []).find((objective) => objective.id === altars[1].id);
  assert(alphaAltar?.ownerAllianceId === alphaAllianceId, "Alpha alliance captures its neutral side altar");
  assert(bravoAltar?.ownerAllianceId === bravoAllianceId, "Bravo alliance captures its neutral side altar");

  const beforeFinish = await req("/v1/season/scoreboard", { token: alpha.token });
  must(beforeFinish.status === 200, "season scoreboard is available during core contest");
  assert(beforeFinish.data?.contestActive === true, "scoreboard marks the core contest active after day 40");
  const alphaScore = (beforeFinish.data?.scores || []).find((entry) => entry.allianceId === alphaAllianceId)?.points;
  const bravoScore = (beforeFinish.data?.scores || []).find((entry) => entry.allianceId === bravoAllianceId)?.points;
  assert(typeof alphaScore === "number" && alphaScore === bravoScore, "symmetric altar captures create an exact score tie before coronation");

  await setSeasonDay(PROFILE.days.finish);
  state = await worldSnapshot(alpha.token);
  const alphaFinal = (state.throneScores || []).find(([allianceId]) => allianceId === alphaAllianceId)?.[1];
  const bravoFinal = (state.throneScores || []).find(([allianceId]) => allianceId === bravoAllianceId)?.[1];
  assert(typeof alphaFinal === "number" && alphaFinal === bravoFinal, "the score tie is preserved at the season finish");
  const expectedChampion = [alphaAllianceId, bravoAllianceId].sort((left, right) => left.localeCompare(right))[0];
  const champion = (state.seasonStory || []).find((event) => event.kind === "season_champion" && event.subjectId === `season:${PROFILE.productionMaxDay}`);
  assert(champion?.allianceId === expectedChampion, "deterministic lexical tie-break crowns exactly one expected champion");
  assert((state.seasonStory || []).some((event) => event.kind === "region_unlocked" && event.subjectId === "CORE"), "season story records the CORE unlock milestone");

  const finalBoard = await req("/v1/season/scoreboard", { token: alpha.token });
  assert(finalBoard.status === 200 && finalBoard.data?.seasonDay === PROFILE.days.finish, "final public scoreboard exposes completed season day");
  assert((finalBoard.data?.scores || []).filter((entry) => entry.points === alphaFinal).length >= 2, "final scoreboard retains both tied contenders for audit");

  console.log("\n==== RESULT ====");
  if (failed === 0) { console.log("P7-T4 ACCELERATED SEASON E2E PASSED"); process.exit(0); }
  console.error(`FAILED ASSERTIONS: ${failed}`);
  process.exit(1);
}

main().catch((error) => { console.error(error); process.exit(1); });
