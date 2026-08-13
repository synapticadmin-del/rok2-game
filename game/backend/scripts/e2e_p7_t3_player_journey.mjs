#!/usr/bin/env node
/**
 * ROK2 — P7-T3: رحلة لاعبين E2E من البداية إلى الممر.
 *
 * يثبت أن لاعبين جديدين يمران بالمدينة وFTUE والتحالف والرالي/الممر وتقرير
 * القتال وقصة الموسم، مع إثبات الملكية والخصوصية وإعادة الاتصال:
 * تسجيل ضيفين → تأسيس مدينتين (روما/الصين) → نفس لقطة العالم المشتركة →
 * تحالف مشترك → منحة موارد إدارية → رالي على ممر مفتوح → انضمام بمساهمة
 * منفصلة → رؤية الرالي قبل وبعد الانضمام → إعادة تحميل الجلسة دون تكرار عضوية →
 * وضع WAIT_FOR_RALLY=1: إطلاق خادمي للمسير + وصول + تقرير قتال للاعبي الفريق
 * (خسائر الهجوم والدفاع) + معلم قصة الموسم → عودة المسير وتسوية بلا خطأ FK.
 *
 * الإدارة:
 *   - مع E2E_LIVE=1 يستهدف BASE_URL مباشرة (خادم تشغّله يدويًا):
 *     1. تأكد من وجود `.dev.vars` بـ AUTH_SECRET/ADMIN_KEY وأعد تشغيل `wrangler dev`.
 *     2. طبّق الهجرات على قاعدة الحالة الافتراضية:
 *        `wrangler d1 migrations apply rok2-db --local`
 *        أو على حالة معزولة عبر WRANGLER_D1_STATE_PATH=/مسار/جديد.
 *   - بدونها ينشئ بيئة معزولة مؤقتة: قواعد بيانات D1/DO جديدة، تطبيق الهجرات،
 *     خادم wrangler dev، ثم يزيل كل شيء عند الانتهاء.
 *
 * التشغيل:
 *   ADMIN_KEY=... E2E_LIVE=1 BASE_URL=http://127.0.0.1:8787 node scripts/e2e_p7_t3_player_journey.mjs
 *   المسار الكامل (إطلاق + تقارير + قصة موسم): WAIT_FOR_RALLY=1 فوق.
 *   # أو بيئة معزولة كاملة (المسار الكامل تلقائيًا):
 *   ADMIN_KEY=... WAIT_FOR_RALLY=1 node scripts/e2e_p7_t3_player_journey.mjs
 */
import { spawn } from "node:child_process";
import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const BACKEND = path.resolve(__dirname, "..");
const PORT = 8793;
const BASE = process.env.E2E_LIVE === "1" && process.env.BASE_URL ? process.env.BASE_URL : `http://127.0.0.1:${PORT}`;
const ADMIN = process.env.ADMIN_KEY;
const WAIT_FOR_RALLY = process.env.WAIT_FOR_RALLY === "1" || process.env.E2E_FULL === "1";

if (!ADMIN) {
  console.error("ADMIN_KEY is not set. Generate one with: node -e \"console.log(require('crypto').randomBytes(32).toString('hex'))\" and export it as ADMIN_KEY.");
  process.exit(1);
}

let failed = 0;
let teardowns = [];

const assert = (condition, message) => {
  if (condition) console.log("OK  :", message);
  else { failed += 1; console.error("FAIL:", message); }
};
const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

async function req(reqPath, { method = "GET", token, body, admin = false } = {}) {
  const headers = { "content-type": "application/json" };
  if (token) headers.authorization = `Bearer ${token}`;
  if (admin) headers["x-admin-key"] = ADMIN;
  let lastErr = null;
  for (let attempt = 0; attempt < 6; attempt += 1) {
    try {
      const response = await fetch(`${BASE}${reqPath}`, {
        method,
        headers,
        body: body ? JSON.stringify(body) : undefined,
      });
      const text = await response.text();
      let data;
      try { data = JSON.parse(text); } catch { data = { raw: text }; }
      return { status: response.status, data };
    } catch (err) { lastErr = err; }
    await sleep(1500);
  }
  if (lastErr) fs.appendFileSync(DEVLOG, `REQ_UNREACHABLE ${method} ${reqPath}: ${lastErr?.message || lastErr}
`);
  console.error("FAIL:", `${method} ${reqPath} unreachable: ${lastErr?.message || lastErr}`);
  failed += 1;
  return { status: 0, data: {} };
}

function must(value, message) {
  assert(Boolean(value), message);
  if (!value) throw new Error(`Cannot continue: ${message}`);
  return value;
}

function cleanup() {
  for (const fn of teardowns) {
    try { fn(); } catch { /* ignore teardown errors */ }
  }
}
process.on("exit", cleanup);
process.on("SIGINT", () => { cleanup(); process.exit(1); });

const DEVLOG = path.join(BACKEND, ".wrangler-e2e-dev.log");
async function withSandboxedServer(body) {
  if (process.env.E2E_LIVE === "1") return body();
  const sandboxDb = path.join(BACKEND, `d1-sandbox-p7t3-${Date.now()}`);
  fs.mkdirSync(sandboxDb, { recursive: true });
  teardowns.push(() => { try { fs.rmSync(sandboxDb, { recursive: true, force: true }); } catch { /* ignore */ } });
  const devVars = path.join(BACKEND, ".dev.vars");
  const devVarsBackup = fs.existsSync(devVars) ? fs.readFileSync(devVars) : null;
  const devVarsBakPath = devVarsBackup ? `${devVars}.e2e-bak-${Date.now()}` : null;
  if (devVarsBackup) {
    fs.renameSync(devVars, devVarsBakPath);
    teardowns.push(() => { try { fs.renameSync(devVarsBakPath, devVars); } catch { /* ignore */ } });
  }
  fs.writeFileSync(devVars, `AUTH_SECRET="${crypto.randomBytes(32).toString("hex")}"\nADMIN_KEY="${ADMIN}"\n`);
  teardowns.push(() => { try { fs.unlinkSync(devVars); } catch { /* ignore */ } });
  // في مسار الإطلاق الكامل، مدة التجمع الرسمية (rally.prep_seconds في
  // zones.json) 300 ثانية — طويلة جدًا لاختبار آلي. نختصرها إلى 8 ثوانٍ مؤقتًا
  // في البيئة المعزولة فقط ونستعيدها عند التنظيف. يجب التعديل قبل إقلاع
  // wrangler dev لأن zones.json يُجمَّع داخل عامل التشغيل.
  let zonesBackup = null;
  if (WAIT_FOR_RALLY) {
    const zonesPath = path.join(BACKEND, "src", "data", "zones.json");
    const zones = JSON.parse(fs.readFileSync(zonesPath, "utf8"));
    zonesBackup = zones.alliance.rally.prep_seconds;
    zones.alliance.rally.prep_seconds = 8;
    fs.writeFileSync(zonesPath, JSON.stringify(zones, null, 2) + "\n");
    teardowns.push(() => {
      try {
        const z2 = JSON.parse(fs.readFileSync(zonesPath, "utf8"));
        z2.alliance.rally.prep_seconds = zonesBackup;
        fs.writeFileSync(zonesPath, JSON.stringify(z2, null, 2) + "\n");
      } catch { /* ignore restore errors */ }
    });
  }
  const dev = spawn("npx", ["wrangler", "dev", "--port", String(PORT), "--inspector-port", "0", "--persist-to", sandboxDb], {
    cwd: BACKEND,
    env: process.env,
    stdio: ["ignore", "pipe", "pipe"],
  });
  teardowns.push(() => { dev.kill(); });
  let devOutput = "";
  dev.stdout.on("data", (chunk) => { devOutput += chunk.toString(); });
  dev.stderr.on("data", (chunk) => { devOutput += chunk.toString(); });
  let started = false;
  for (let i = 0; i < 40; i += 1) {
    await sleep(1000);
    if (devOutput.includes("Ready on")) { started = true; break; }
    if (dev.exitCode !== null) { fs.writeFileSync(DEVLOG, devOutput); throw new Error(`wrangler dev exited early: ${devOutput.slice(-800)}`); }
  }
  if (!started) { fs.writeFileSync(DEVLOG, devOutput); throw new Error(`wrangler dev did not start: ${devOutput.slice(-1200)}`); }
  const apply = spawn("npx", ["wrangler", "d1", "migrations", "apply", "rok2-db", "--local", "--persist-to", sandboxDb], {
    cwd: BACKEND,
    env: process.env,
    stdio: ["ignore", "pipe", "pipe"],
  });
  await new Promise((resolve, reject) => {
    let out = "";
    apply.stdout.on("data", (c) => { out += c.toString(); });
    apply.stderr.on("data", (c) => { out += c.toString(); });
    apply.on("exit", (code) => { code === 0 ? resolve() : reject(new Error(out.slice(-600))); });
  });
  try {
    return await body();
  } finally {
    try { fs.writeFileSync(DEVLOG, devOutput); } catch { /* ignore */ }
    cleanup();
  }
}

async function forceTicks(count = 6) {
  for (let i = 0; i < count; i += 1) {
    const tick = await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } });
    assert(tick.status === 200, `forced world tick ${i + 1}/${count}`);
    await sleep(120);
  }
}

function selectOpenPass(snapshot) {
  const passes = snapshot?.passes || [];
  return passes.find((pass) => pass.id.startsWith("P_R") && !pass.ownerAllianceId && (pass.unlockDay || 0) === 0)
    || passes.find((pass) => !pass.ownerAllianceId && (pass.unlockDay || 0) === 0)
    || passes[0];
}

function reportForRally(snapshot, rallyId) {
  return (snapshot?.reports || []).find((report) => report?.rally?.rallyId === rallyId);
}

async function createPlayer(run, suffix, name, civ) {
  const guest = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: `p7t3-${suffix}-${run}`, name },
  });
  const token = must(guest.status === 200 && guest.data?.token, `${name}: guest authentication succeeds`);
  const founded = await req("/v1/city/init", { method: "POST", token, body: { civ, name } });
  if (founded.status !== 200) {
    try { fs.appendFileSync(DEVLOG, `CITY_INIT_RESPONSE ${name}: status=${founded.status} body=${JSON.stringify(founded.data)}
`); } catch { /* ignore */ }
    console.error(`CITY_INIT_DEBUG ${name}: status=${founded.status} body=${JSON.stringify(founded.data)}`);
  }
  must(founded.status === 200 && founded.data?.player?.id && founded.data?.token, `${name}: FTUE city foundation succeeds`);
  return { id: founded.data.player.id, token: founded.data.token, name };
}

async function main() {
  console.log("ROK2 P7-T3 E2E player journey against", BASE);
  console.log("Mode:", WAIT_FOR_RALLY ? "full rally launch + combat + season story" : "formation + ownership + resume");

  const health = await req("/v1/health");
  must(health.status === 200 && health.data?.ok, "server health endpoint is available");
  const anonymousSnapshot = await req("/v1/world/snapshot");
  assert(anonymousSnapshot.status === 401 || anonymousSnapshot.status === 403, "world snapshot rejects unauthenticated access (privacy)");

  const run = Date.now();
  const alpha = await createPlayer(run, "alpha", "Alpha Journey", "rome");
  const bravo = await createPlayer(run, "bravo", "Bravo Journey", "china");

  let snapshot = await req("/v1/world/snapshot", { token: alpha.token });
  must(snapshot.status === 200, "authenticated world snapshot is available after FTUE");
  const cityIds = (snapshot.data?.cities || []).map((city) => city.playerId);
  assert(cityIds.includes(alpha.id) && cityIds.includes(bravo.id), "both newly founded cities share the same world snapshot");

  const passes = snapshot.data?.passes || [];
  assert(passes.length >= 1, "world snapshot exposes passes for rally targeting");

  const tag = `J${String(run).slice(-3)}`;
  const created = await req("/v1/alliance/create", {
    method: "POST",
    token: alpha.token,
    body: { name: "Journey Alliance", tag },
  });
  const alliance = must(created.status === 200 && created.data?.alliance?.id, "Alpha creates the shared alliance");
  const allianceId = created.data.alliance.id;

  const joined = await req("/v1/alliance/join", { method: "POST", token: bravo.token, body: { allianceId } });
  assert(joined.status === 200 && joined.data?.allianceId === allianceId, "Bravo joins Alpha's alliance");
  const allianceState = await req(`/v1/alliance/${allianceId}`, { token: alpha.token });
  const memberIds = (allianceState.data?.members || []).map((member) => member.id);
  assert(allianceState.status === 200 && memberIds.includes(alpha.id) && memberIds.includes(bravo.id), "alliance membership is visible to its members");

  for (const player of [alpha, bravo]) {
    const grant = await req("/v1/admin/grant", {
      method: "POST",
      admin: true,
      body: { playerId: player.id, food: 250000, wood: 250000, troops: { infantry_t1: 1500, archer_t1: 800, cavalry_t1: 400 } },
    });
    assert(grant.status === 200, `${player.name}: administrative E2E fixture grant succeeds`);
  }

  snapshot = await req("/v1/world/snapshot", { token: alpha.token });
  const pass = must(selectOpenPass(snapshot.data), "an unlocked, unowned pass is available for the rally");
  console.log("   rally target pass:", pass.id);

  const createdRally = await req("/v1/alliance/rally", {
    method: "POST",
    token: alpha.token,
    body: { targetType: "pass", targetId: pass.id, troops: { infantry_t1: 500, archer_t1: 250, cavalry_t1: 100 } },
  });
  const rally = must(createdRally.status === 200 && createdRally.data?.rally?.id, "Alpha creates a forming rally for the pass");
  const rallyId = createdRally.data.rally.id;
  assert(createdRally.data.rally.status === "forming", "new rally begins in the forming state");

  const visibleBeforeJoin = await req("/v1/alliance/rallies", { token: bravo.token });
  const listedBeforeJoin = (visibleBeforeJoin.data?.rallies || []).find((entry) => entry.id === rallyId);
  assert(visibleBeforeJoin.status === 200 && listedBeforeJoin?.participants === 1 && !listedBeforeJoin?.isJoined, "Bravo sees the forming rally before joining");

  const joinedRally = await req("/v1/alliance/rally/join", {
    method: "POST",
    token: bravo.token,
    body: { rallyId, troops: { infantry_t1: 450, archer_t1: 225, cavalry_t1: 90 } },
  });
  assert(joinedRally.status === 200 && joinedRally.data?.participants === 2, "Bravo joins the rally with a separate troop contribution");

  const detail = await req(`/v1/alliance/rally/${rallyId}`, { token: alpha.token });
  const participantIds = (detail.data?.participants || []).map((entry) => entry.playerId);
  assert(detail.status === 200 && participantIds.includes(alpha.id) && participantIds.includes(bravo.id), "rally detail preserves both participant identities");

  await sleep(250);
  const resumed = await req("/v1/alliance/rallies", { token: bravo.token });
  const resumedRally = (resumed.data?.rallies || []).find((entry) => entry.id === rallyId);
  assert(resumed.status === 200 && resumedRally?.isJoined && resumedRally?.participants === 2, "reloaded session restores joined-rally state without duplicate membership");

  let marchId = null;
  if (WAIT_FOR_RALLY) {
    const launchMs = Number(createdRally.data.rally.launchMs || 0);
    const waitMs = Math.max(0, launchMs - Date.now() + 1500);
    console.log(`   waiting ${(waitMs / 1000).toFixed(1)}s for server-authoritative rally launch...`);
    await sleep(waitMs);
    await forceTicks(8);
    const afterLaunch = await req(`/v1/alliance/rally/${rallyId}`, { token: alpha.token });
    assert(afterLaunch.status === 200 && afterLaunch.data?.rally?.status === "launched", "forming rally becomes a server-launched march");
    assert(Boolean(afterLaunch.data?.rally?.march_id || afterLaunch.data?.rally?.marchId), "launched rally records its unified march");
    marchId = afterLaunch.data?.rally?.march_id || afterLaunch.data?.rally?.marchId;
    const arrivalWaitMs = 9500;
    console.log(`   waiting ${(arrivalWaitMs / 1000).toFixed(1)}s for rally arrival...`);
    await sleep(arrivalWaitMs);
    await forceTicks(4);
    const alphaSnapshot = await req("/v1/world/snapshot", { token: alpha.token });
    const bravoSnapshot = await req("/v1/world/snapshot", { token: bravo.token });
    const alphaReport = reportForRally(alphaSnapshot.data, rallyId);
    const bravoReport = reportForRally(bravoSnapshot.data, rallyId);
    assert(Boolean(alphaReport?.result), "rally leader receives the resulting battle report");
    assert(Boolean(bravoReport?.result), "rally participant receives the resulting battle report");
    assert(alphaReport?.result?.attackerLosses && alphaReport?.result?.defenderLosses, "rally report contains both combat-loss sides");
    assert((alphaSnapshot.data?.seasonStory || []).some((entry) => entry.kind === "first_pass_capture" || entry.kind === "pass_conquered"), "pass resolution leaves a season-story milestone in the snapshot");
    await sleep(9500);
    await forceTicks(3);
    const afterReturn = await req("/v1/world/snapshot", { token: alpha.token });
    assert(!(afterReturn.data?.marches || []).some((march) => march.id === marchId), "rally return settles without leaving a moving march or a database error");
  } else {
    console.log("NOTE: formation, ownership, and rehydration are verified. Re-run with WAIT_FOR_RALLY=1 for launch, combat reports, and season-story verification.");
  }

  console.log("\n==== RESULT ====");
  if (failed === 0) {
    console.log(WAIT_FOR_RALLY ? "P7-T3 FULL E2E PASSED" : "P7-T3 RALLY FORMATION E2E PASSED");
    process.exit(0);
  }
  console.error(`FAILED ASSERTIONS: ${failed}`);
  process.exit(1);
}

withSandboxedServer(main).catch((error) => { console.error(error); process.exit(1); });
