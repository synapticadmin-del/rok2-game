#!/usr/bin/env node
/**
 * ROK2 API smoke test against wrangler dev (default http://127.0.0.1:8787)
 *
 * Usage:
 *   ADMIN_KEY=... node scripts/smoke.mjs
 *   BASE_URL=http://127.0.0.1:8787 ADMIN_KEY=... node scripts/smoke.mjs
 *
 * ملاحظة أمنية: لا قيمة افتراضية لـ ADMIN_KEY — كانت "rok2-dev-admin" مكتوبة
 * هنا وهي قيمة مسرّبة (موجودة في قائمة الرفض في secrets.ts). اضبط المفتاح
 * الحقيقي في البيئة.
 */

const BASE = process.env.BASE_URL || "http://127.0.0.1:8787";
const ADMIN = process.env.ADMIN_KEY;
if (!ADMIN) {
  console.error("ADMIN_KEY is not set. Run: ADMIN_KEY=your-key node scripts/smoke.mjs");
  process.exit(1);
}

let failed = 0;

function assert(cond, msg) {
  if (!cond) {
    failed++;
    console.error("FAIL:", msg);
  } else {
    console.log("OK  :", msg);
  }
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
  try {
    data = JSON.parse(text);
  } catch {
    data = { raw: text };
  }
  return { status: res.status, data };
}

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

async function main() {
  console.log("ROK2 smoke against", BASE);

  // wait for server
  let serverUp = false;
  for (let i = 0; i < 30; i++) {
    try {
      const h = await req("/v1/health");
      if (h.status === 200 && h.data.ok) {
        serverUp = true;
        break;
      }
    } catch {
      // retry
    }
    await sleep(500);
  }
  assert(serverUp, "health endpoint up");
  if (!serverUp) {
    console.error("Server not reachable. Start: npx wrangler dev");
    process.exit(1);
  }

  const meta = await req("/v1/meta/map");
  assert(meta.status === 200 && meta.data.width === 1200, "map meta width 1200");
  assert(Array.isArray(meta.data.regions) && meta.data.regions.length >= 8, "map has regions");
  assert(Array.isArray(meta.data.passes) && meta.data.passes.length >= 8, "map has passes");

  const civs = await req("/v1/meta/civilizations");
  assert(civs.status === 200 && civs.data.civilizations?.length >= 6, "civilizations meta");

  // Guest A
  const aGuest = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: `smoke-a-${Date.now()}`, name: "Alpha" },
  });
  assert(aGuest.status === 200 && aGuest.data.token, "guest A token");
  let aToken = aGuest.data.token;

  const aInit = await req("/v1/city/init", {
    method: "POST",
    token: aToken,
    body: { civ: "rome", name: "Alpha" },
  });
  assert(aInit.status === 200 && aInit.data.token, "city init A returns new token");
  aToken = aInit.data.token;
  const aPlayerId = aInit.data.player.id;

  // Guest B
  const bGuest = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: `smoke-b-${Date.now()}`, name: "Bravo" },
  });
  let bToken = bGuest.data.token;
  const bInit = await req("/v1/city/init", {
    method: "POST",
    token: bToken,
    body: { civ: "china", name: "Bravo" },
  });
  bToken = bInit.data.token;
  assert(bInit.status === 200 && bInit.data.player?.id, "city init B");

  const aCity = await req("/v1/city", { token: aToken });
  assert(aCity.status === 200 && aCity.data.city.food >= 0, "get city A");
  assert(aCity.data.troops?.infantry_t1 >= 100, "starter infantry A");

  // upgrade farm
  const up = await req("/v1/city/upgrade", {
    method: "POST",
    token: aToken,
    body: { buildingId: "farm" },
  });
  assert(up.status === 200 && up.data.level === 2, "upgrade farm to 2");

  // train
  const tr = await req("/v1/city/train", {
    method: "POST",
    token: aToken,
    body: { unit: "infantry_t1", count: 20 },
  });
  assert(tr.status === 200 && tr.data.count >= 120, "train infantry");

  // grant more troops for war
  const grantA = await req("/v1/admin/grant", {
    method: "POST",
    admin: true,
    body: {
      playerId: aPlayerId,
      food: 100000,
      wood: 100000,
      troops: { infantry_t1: 500, archer_t1: 300, cavalry_t1: 200 },
    },
  });
  assert(grantA.status === 200, "admin grant A");

  const grantB = await req("/v1/admin/grant", {
    method: "POST",
    admin: true,
    body: {
      playerId: bInit.data.player.id,
      food: 100000,
      wood: 100000,
      troops: { infantry_t1: 500, archer_t1: 300, cavalry_t1: 200 },
    },
  });
  assert(grantB.status === 200, "admin grant B");

  // alliances (unique tags each run — D1 persists locally)
  const tagA = `A${String(Date.now()).slice(-3)}`;
  const tagB = `B${String(Date.now()).slice(-3)}`;
  const allA = await req("/v1/alliance/create", {
    method: "POST",
    token: aToken,
    body: { name: "Legion Alpha", tag: tagA },
  });
  assert(allA.status === 200 && allA.data.alliance?.id, `create alliance A (${tagA}) ${JSON.stringify(allA.data)}`);

  const allB = await req("/v1/alliance/create", {
    method: "POST",
    token: bToken,
    body: { name: "Dynasty Bravo", tag: tagB },
  });
  assert(allB.status === 200 && allB.data.alliance?.id, `create alliance B (${tagB}) ${JSON.stringify(allB.data)}`);
  if (!allA.data.alliance?.id || !allB.data.alliance?.id) {
    console.error("Cannot continue without alliances");
    process.exit(1);
  }

  // snapshot يتطلب مصادقة الآن — نمرر رمز A (نسخة كاملة للاعبين)
  let snap = await req("/v1/world/snapshot", { token: aToken });
  assert(snap.status === 200, "world snapshot (authed)");
  const passCandidates = (snap.data.passes || []).filter(
    (p) => p.id.startsWith("P_R") && (p.unlockDay || 0) === 0,
  );
  let passPick =
    passCandidates.find((p) => !p.ownerAllianceId) ||
    passCandidates[0] ||
    { id: "P_R2_R3" };
  const passId = passPick.id;
  console.log("Using pass", passId, "owner=", passPick.ownerAllianceId);

  // A attacks pass
  const atk1 = await req("/v1/world/pass/attack", {
    method: "POST",
    token: aToken,
    body: { passId, troops: { infantry_t1: 300, archer_t1: 150, cavalry_t1: 100 } },
  });
  assert(atk1.status === 200 && atk1.data.march?.id, `A march on ${passId}`);

  // force ticks until march arrives (force=complete marches immediately)
  for (let i = 0; i < 5; i++) {
    await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    await sleep(100);
  }

  snap = await req("/v1/world/snapshot", { token: aToken });
  assert(snap.status === 200, "world snapshot");
  let pass = (snap.data.passes || []).find((p) => p.id === passId);
  console.log("Pass state after A:", pass);

  // If not captured yet (progress), attack again
  for (let round = 0; round < 4 && (!pass?.ownerAllianceId || pass.captureProgress < 100); round++) {
    // re-grant troops
    await req("/v1/admin/grant", {
      method: "POST",
      admin: true,
      body: {
        playerId: aPlayerId,
        troops: { infantry_t1: 400, archer_t1: 200, cavalry_t1: 150 },
      },
    });
    const atk = await req("/v1/world/pass/attack", {
      method: "POST",
      token: aToken,
      body: { passId, troops: { infantry_t1: 300, archer_t1: 150, cavalry_t1: 100 } },
    });
    assert(atk.status === 200, `A re-attack round ${round + 1}`);
    for (let i = 0; i < 4; i++) {
      await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
      await sleep(100);
    }
    snap = await req("/v1/world/snapshot", { token: aToken });
    pass = (snap.data.passes || []).find((p) => p.id === passId);
    console.log(`Pass after round ${round + 1}:`, {
      owner: pass?.ownerAllianceId,
      progress: pass?.captureProgress,
    });
  }

  assert(!!pass?.ownerAllianceId, "pass has owner after attacks");
  assert(pass.ownerAllianceId === allA.data.alliance.id, "pass owned by alliance A");

  // B contests
  await req("/v1/admin/grant", {
    method: "POST",
    admin: true,
    body: {
      playerId: bInit.data.player.id,
      troops: { infantry_t1: 600, archer_t1: 300, cavalry_t1: 200 },
    },
  });
  const atkB = await req("/v1/world/pass/attack", {
    method: "POST",
    token: bToken,
    body: { passId, troops: { infantry_t1: 400, archer_t1: 200, cavalry_t1: 150 } },
  });
  assert(atkB.status === 200, "B attacks contested pass");
  for (let i = 0; i < 12; i++) {
    await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    await sleep(100);
  }
  snap = await req("/v1/world/snapshot", { token: aToken });
  pass = (snap.data.passes || []).find((p) => p.id === passId);
  console.log("Pass after B contest:", pass);
  assert(Array.isArray(snap.data.reports), "battle reports present");

  // set season day unlock
  const day = await req("/v1/admin/set-time", {
    method: "POST",
    admin: true,
    body: { day: 14 },
  });
  assert(day.status === 200 && day.data.seasonDay === 14, "admin set season day");

  console.log("\n==== RESULT ====");
  if (failed === 0) {
    console.log("ALL SMOKE TESTS PASSED");
    process.exit(0);
  } else {
    console.error(`FAILED ASSERTIONS: ${failed}`);
    process.exit(1);
  }
}

main().catch((e) => {
  console.error(e);
  process.exit(1);
});
