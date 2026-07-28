#!/usr/bin/env node
/**
 * ROK2 hospital system E2E test (P2-T2) against a live API.
 *
 * Validates:
 *  1. GET /v1/city exposes hospital block (level/capacity/used/free) + wounded map.
 *  2. After a pass battle, severely wounded troops appear in 'severely_wounded'
 *     capped by hospital capacity (overflow dies), and the battle report carries
 *     a hospital summary (admitted/died).
 *  3. /v1/city/heal rejects healing more than wounded, heals with resource cost,
 *     and after queue completion troops return home.
 *
 * Usage:
 *   BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/hospital_test.mjs
 */

const BASE = process.env.BASE_URL || "http://127.0.0.1:8787";
const ADMIN = process.env.ADMIN_KEY || "rok2-dev-admin";

let failed = 0;
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}

async function req(path, { method = "GET", token, body, admin = false } = {}) {
  const headers = { "content-type": "application/json" };
  if (token) headers.authorization = `Bearer ${token}`;
  if (admin) headers["x-admin-key"] = ADMIN;
  const res = await fetch(`${BASE}${path}`, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });
  const text = await res.text();
  let data;
  try { data = JSON.parse(text); } catch { data = { raw: text }; }
  return { status: res.status, data };
}

function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

async function main() {
  console.log("ROK2 hospital test against", BASE);

  // fresh player
  const guest = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: `hosp-${Date.now()}-${Math.floor(Math.random() * 1e6)}`, name: "Medic" },
  });
  let token = guest.data.token;
  const init = await req("/v1/city/init", { method: "POST", token, body: { civ: "egypt", name: "Medic" } });
  token = init.data.token || token;
  const playerId = init.data.player.id;

  // 1) city exposes hospital block (starter hospital L1 → capacity 200)
  let city = await req("/v1/city", { token });
  assert(city.status === 200 && city.data.hospital, `city exposes hospital block: ${JSON.stringify(city.data.hospital)}`);
  assert(city.data.hospital.capacity === 200 && city.data.hospital.used === 0, "starter hospital capacity 200, empty");
  assert(city.data.wounded && Object.keys(city.data.wounded).length === 0, "no wounded initially");

  // troops for a battle that will produce wounded
  const grant = await req("/v1/admin/grant", {
    method: "POST",
    admin: true,
    body: { playerId, food: 100000, wood: 100000, stone: 50000, gold: 50000, troops: { infantry_t1: 500, archer_t1: 300, cavalry_t1: 200 } },
  });
  assert(grant.status === 200, "admin grant");

  const tag = `H${String(Date.now()).slice(-3)}`;
  const alliance = await req("/v1/alliance/create", { method: "POST", token, body: { name: "Medic Corps", tag } });
  assert(alliance.status === 200, `alliance created (${tag})`);

  const snap = await req("/v1/world/snapshot");
  const passPick = (snap.data.passes || []).find((p) => p.id.startsWith("P_R") && (p.unlockDay || 0) === 0) || { id: "P_R2_R3" };

  // small attack → guarantees losses → severely wounded
  const atk = await req("/v1/world/pass/attack", {
    method: "POST",
    token,
    body: { passId: passPick.id, troops: { infantry_t1: 120, archer_t1: 60 } },
  });
  assert(atk.status === 200 && atk.data.march?.id, "attack march created");

  for (let i = 0; i < 10; i++) {
    await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    await sleep(150);
  }

  // 2) wounded recorded, capped by capacity; report has hospital summary
  city = await req("/v1/city", { token });
  const woundedTotal = Object.values(city.data.wounded || {}).reduce((s, n) => s + n, 0);
  console.log("wounded after battle:", city.data.wounded, "hospital:", city.data.hospital);
  assert(woundedTotal > 0, `severely wounded recorded after battle (${woundedTotal})`);
  assert(city.data.hospital.used === woundedTotal, "hospital.used matches wounded total");
  assert(city.data.hospital.used <= city.data.hospital.capacity, "wounded within capacity (overflow dies)");

  const snap2 = await req("/v1/world/snapshot");
  const report = (snap2.data.reports || []).find((r) => r.kind === "pass_attack" && r.attackerPlayerId === playerId);
  assert(report && report.hospital, `battle report carries hospital summary: ${JSON.stringify(report && report.hospital)}`);

  // 3) heal flow — reject over-heal, then heal all with cost, queue completes
  const over = await req("/v1/city/heal", {
    method: "POST",
    token,
    body: { troops: { infantry_t1: (city.data.wounded.infantry_t1 || 0) + 999 } },
  });
  assert(over.status === 400, "over-heal rejected (400)");

  const healAll = await req("/v1/city/heal", {
    method: "POST",
    token,
    body: { troops: city.data.wounded },
  });
  assert(healAll.status === 200 && healAll.data.queueId, `heal started: queue ${healAll.data.queueId} cost ${JSON.stringify(healAll.data.cost)}`);
  assert(healAll.data.healSeconds > 0, `heal duration from data (${healAll.data.healSeconds}s)`);

  city = await req("/v1/city", { token });
  const woundedAfterQueue = Object.values(city.data.wounded || {}).reduce((s, n) => s + n, 0);
  assert(woundedAfterQueue === 0, "wounded moved to heal queue (0 still wounded)");

  const homeBefore = Object.values(city.data.troops || {}).reduce((s, n) => s + n, 0);
  for (let i = 0; i < 8; i++) {
    await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    await sleep(150);
  }
  city = await req("/v1/city", { token });
  const homeAfter = Object.values(city.data.troops || {}).reduce((s, n) => s + n, 0);
  assert(homeAfter > homeBefore, `healed troops returned home (${homeBefore} → ${homeAfter})`);

  console.log("\n==== RESULT ====");
  if (failed === 0) { console.log("ALL HOSPITAL TESTS PASSED"); process.exit(0); }
  console.error(`FAILED ASSERTIONS: ${failed}`);
  process.exit(1);
}

main().catch((e) => { console.error(e); process.exit(1); });
