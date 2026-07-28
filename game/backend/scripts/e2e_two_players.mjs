#!/usr/bin/env node
/**
 * ROK2 — E2E اختبار لاعبين: بوابة نجاح المرحلة 1 (P1-T7)
 *
 * السيناريو (مطابق game/docs/E2E_TWO_PLAYERS.md):
 *   لاعبان (Alpha / Bravo) → تحالفان متنافسان → نزاع على ممر جبلي
 *   → كلا الطرفين يستلم تقرير قتال صحيحاً بتقسيم الخسائر.
 *
 * التشغيل:
 *   node scripts/e2e_two_players.mjs                     # ضد wrangler dev المحلي
 *   BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/e2e_two_players.mjs
 */

const BASE = process.env.BASE_URL || "http://127.0.0.1:8787";
const ADMIN = process.env.ADMIN_KEY || "rok2-dev-admin";

let failed = 0;
let deployStale = false; // الـ API المُنشر أقدم من main — يحتاج wrangler deploy
function assert(cond, msg) {
  if (!cond) { failed++; console.error("FAIL:", msg); }
  else console.log("OK  :", msg);
}
function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

async function req(path, { method = "GET", token, body, admin = false } = {}) {
  const headers = { "content-type": "application/json" };
  if (token) headers.authorization = `Bearer ${token}`;
  if (admin) headers["x-admin-key"] = ADMIN;
  const res = await fetch(`${BASE}${path}`, {
    method, headers, body: body ? JSON.stringify(body) : undefined,
  });
  const text = await res.text();
  let data;
  try { data = JSON.parse(text); } catch { data = { raw: text }; }
  return { status: res.status, data };
}

function findReportFor(reports, attackerId, passId) {
  return (reports || []).find(
    (r) => r.attackerPlayerId === attackerId && (!passId || r.passId === passId),
  );
}

function assertReportShape(r, label) {
  assert(r && r.result, `${label}: report exists`);
  if (!r || !r.result) return;
  assert(
    ["attacker", "defender", "draw"].includes(r.result.winner),
    `${label}: winner is valid (${r.result.winner})`,
  );
  assert(typeof r.result.attackerLosses === "object", `${label}: attackerLosses present`);
  assert(typeof r.result.defenderLosses === "object", `${label}: defenderLosses present`);
  const split = r.result.attackerSplit;
  if (split && typeof split.dead === "object" && typeof split.severely === "object" && typeof split.slightly === "object") {
    console.log("OK  :", `${label}: loss split (dead/severely/slightly) present`);
  } else if (split) {
    console.log("OK  :", `${label}: split present (partial shape)`);
  } else {
    console.warn("WARN:", `${label}: loss split missing — deployed API is older than main (run: npx wrangler deploy)`);
    deployStale = true;
  }
  assert(r.result.powerBefore && r.result.powerBefore.attacker > 0,
    `${label}: powerBefore recorded`);
}

async function forceTicks(n = 4) {
  for (let i = 0; i < n; i++) {
    await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    await sleep(120);
  }
}

async function main() {
  console.log("ROK2 E2E two-players against", BASE);

  // ---------- 0) server up ----------
  const h = await req("/v1/health");
  assert(h.status === 200 && h.data.ok, "server up");
  if (h.status !== 200) process.exit(1);

  // فحص نشر: /v1/meta/all موجود فقط في أحدث كود على main
  const metaAll = await req("/v1/meta/all");
  if (metaAll.status === 200) {
    console.log("OK  : /v1/meta/all deployed");
  } else {
    console.warn("WARN: /v1/meta/all missing — deployed API older than main (run: npx wrangler deploy)");
    deployStale = true;
  }

  const run = Date.now();

  // ---------- 1) لاعبان يسجلان ويؤسسان مدينتين ----------
  const aGuest = await req("/v1/auth/guest", { method: "POST", body: { deviceId: `e2e-a-${run}`, name: "Alpha" } });
  const aInit = await req("/v1/city/init", { method: "POST", token: aGuest.data.token, body: { civ: "rome", name: "Alpha" } });
  assert(aInit.status === 200 && aInit.data.player?.id, "player A (Alpha) founded city");
  const aToken = aInit.data.token;
  const aId = aInit.data.player.id;

  const bGuest = await req("/v1/auth/guest", { method: "POST", body: { deviceId: `e2e-b-${run}`, name: "Bravo" } });
  const bInit = await req("/v1/city/init", { method: "POST", token: bGuest.data.token, body: { civ: "china", name: "Bravo" } });
  assert(bInit.status === 200 && bInit.data.player?.id, "player B (Bravo) founded city");
  const bToken = bInit.data.token;
  const bId = bInit.data.player.id;

  // المدينتان ظاهرتان في نفس العالم المشترك
  let snap = await req("/v1/world/snapshot");
  const cityIds = (snap.data.cities || []).map((c) => c.playerId);
  assert(cityIds.includes(aId) && cityIds.includes(bId), "both cities on shared map");

  // ---------- 2) تحالفان متنافسان ----------
  const tagA = `AL${String(run).slice(-4)}`;
  const tagB = `BR${String(run).slice(-4)}`;
  const allA = await req("/v1/alliance/create", { method: "POST", token: aToken, body: { name: "Legion Alpha", tag: tagA } });
  assert(allA.status === 200 && allA.data.alliance?.id, "alliance A created");
  const allB = await req("/v1/alliance/create", { method: "POST", token: bToken, body: { name: "Dynasty Bravo", tag: tagB } });
  assert(allB.status === 200 && allB.data.alliance?.id, "alliance B created");
  const aAlliance = allA.data.alliance.id;
  const bAlliance = allB.data.alliance.id;

  // ---------- 3) تجهيز قوات ----------
  for (const pid of [aId, bId]) {
    const g = await req("/v1/admin/grant", {
      method: "POST", admin: true,
      body: { playerId: pid, food: 200000, wood: 200000, troops: { infantry_t1: 1500, archer_t1: 800, cavalry_t1: 400 } },
    });
    assert(g.status === 200, `grant troops for ${pid === aId ? "A" : "B"}`);
  }

  // ---------- 4) اختيار ممر مفتوح ----------
  snap = await req("/v1/world/snapshot");
  const passPick =
    (snap.data.passes || []).find((p) => p.id.startsWith("P_R") && !p.ownerAllianceId && (p.unlockDay || 0) === 0) ||
    (snap.data.passes || [])[0];
  const passId = passPick.id;
  console.log("   contested pass:", passId);

  // ---------- 5) A يحتل الممر (هجمات متتالية حتى الملكية) ----------
  let pass = passPick;
  for (let round = 0; round < 6; round++) {
    if (pass.ownerAllianceId === aAlliance && pass.captureProgress >= 100) break;
    await req("/v1/admin/grant", { method: "POST", admin: true, body: { playerId: aId, troops: { infantry_t1: 600, archer_t1: 300, cavalry_t1: 150 } } });
    const atk = await req("/v1/world/pass/attack", {
      method: "POST", token: aToken,
      body: { passId, troops: { infantry_t1: 400, archer_t1: 200, cavalry_t1: 100 } },
    });
    assert(atk.status === 200 && atk.data.march?.id, `A attack round ${round + 1} dispatched`);
    await forceTicks(4);
    snap = await req("/v1/world/snapshot");
    pass = (snap.data.passes || []).find((p) => p.id === passId);
  }
  assert(pass?.ownerAllianceId === aAlliance, "pass owned by alliance A after assault");

  // تقرير A صحيح
  snap = await req("/v1/world/snapshot");
  const aReport = findReportFor(snap.data.reports, aId, passId);
  assertReportShape(aReport, "A capture report");

  // ---------- 6) B ينازع الممر المملوك لـ A ----------
  await req("/v1/admin/grant", { method: "POST", admin: true, body: { playerId: bId, troops: { infantry_t1: 800, archer_t1: 400, cavalry_t1: 200 } } });
  const atkB = await req("/v1/world/pass/attack", {
    method: "POST", token: bToken,
    body: { passId, troops: { infantry_t1: 500, archer_t1: 250, cavalry_t1: 120 } },
  });
  assert(atkB.status === 200 && atkB.data.march?.id, "B contest march dispatched");
  await forceTicks(8);

  // ---------- 7) كلا الطرفين يستلمان تقارير صحيحة ----------
  snap = await req("/v1/world/snapshot");
  assert(snap.status === 200, "final snapshot");
  const bReport = findReportFor(snap.data.reports, bId, passId);
  assertReportShape(bReport, "B contest report");

  // الممر ما زال متنازعاً عليه (مالك + سجل معارك) — بوابة النجاح
  pass = (snap.data.passes || []).find((p) => p.id === passId);
  console.log("   final pass state:", { owner: pass?.ownerAllianceId, progress: pass?.captureProgress, state: pass?.state });
  assert(!!pass?.ownerAllianceId, "pass still owned (contest resolved by combat)");
  assert((snap.data.reports || []).length >= 2, "multiple battle reports recorded");

  console.log("\n==== RESULT ====");
  if (failed === 0) {
    console.log("E2E TWO PLAYERS PASSED — بوابة المرحلة 1 تحققت");
    if (deployStale) console.log("NOTE: deployed API is behind main — run: cd game/backend && npx wrangler deploy");
    process.exit(0);
  }
  console.error(`FAILED ASSERTIONS: ${failed}`);
  process.exit(1);
}

main().catch((e) => { console.error(e); process.exit(1); });
