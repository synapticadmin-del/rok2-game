#!/usr/bin/env node
/**
 * ROK2 — P7-T1: اختبار E2E لتسجيل الضيف وتأسيس المدينة (Guest Onboarding).
 *
 * يثبت أن فلو "تسجيل الدخول → قياس الهوية → تأسيس المدينة → القادة → لقطة العالم"
 * يعمل من أول طلب حتى أول لقطة سلطة، على خادم D1/Durable Objects محلي نظيف،
 * وهو ما يمثله عميل UE5 في Rok2BootWidget → URok2Api::LoginAsGuest →
 * URok2Api::InitCity → URok2Api::FetchCommanders → world snapshot.
 *
 * الإدارة:
 *   - مع E2E_LIVE=1 يستهدف BASE_URL مباشرة (خادم تشغّله يدويًا):
 *     1. تأكد من وجود `.dev.vars` بـ AUTH_SECRET/ADMIN_KEY وأعد تشغيل `wrangler dev`
 *        (wrangler يقرأ الملف عند الإقلاع فقط).
 *     2. طبّق الهجرات على قاعدة الحالة الافتراضية:
 *        `wrangler d1 migrations apply rok2-db --local`
 *        أو على حالة معزولة عبر WRANGLER_D1_STATE_PATH=/مسار/جديد.
 *   - بدونها ينشئ بيئة معزولة مؤقتة: قواعد بيانات D1 جديدة، تطبيق الهجرات،
 *     خادم wrangler dev، ثم يزيل كل شيء عند الانتهاء.
 *
 * التشغيل:
 *   ADMIN_KEY=$(cat /tmp/admin_key.txt) E2E_LIVE=1 BASE_URL=http://127.0.0.1:8787 node scripts/e2e_p7_t1_guest_onboarding.mjs
 *   # أو بيئة معزولة كاملة:
 *   ADMIN_KEY=$(cat /tmp/admin_key.txt) node scripts/e2e_p7_t1_guest_onboarding.mjs
 */
import { spawn } from "node:child_process";
import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const BACKEND = path.resolve(__dirname, "..");
const PORT = 8791;
const BASE = process.env.E2E_LIVE === "1" && process.env.BASE_URL ? process.env.BASE_URL : `http://127.0.0.1:${PORT}`;
const ADMIN = process.env.ADMIN_KEY;
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

async function withSandboxedServer(body) {
  if (process.env.E2E_LIVE === "1") return body();
  const sandboxDb = path.join(BACKEND, `d1-sandbox-${Date.now()}`);
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

  const dev = spawn("npx", ["wrangler", "dev", "--port", String(PORT), "--inspector-port", "0"], {
    cwd: BACKEND,
    env: { ...process.env, WRANGLER_D1_STATE_PATH: sandboxDb, WRANGLER_DO_STATE_PATH: sandboxDb },
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
    if (dev.exitCode !== null) throw new Error(`wrangler dev exited early: ${devOutput.slice(-800)}`);
  }
  if (!started) throw new Error(`wrangler dev did not start: ${devOutput.slice(-1200)}`);

  // قاعدة بيانات محلية جديدة نظيفة + تطبيق كل الهجرات
  const apply = spawn("npx", ["wrangler", "d1", "migrations", "apply", "rok2-db", "--local"], {
    cwd: BACKEND,
    env: { ...process.env, WRANGLER_D1_STATE_PATH: sandboxDb },
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
    cleanup();
  }
}

async function main() {
  console.log("ROK2 P7-T1 E2E guest onboarding against", BASE);
  const run = Date.now();

  // 0 — الخادم حي، ولقطة العالم مرفوضة قبل المصادقة (عزل غير الضيوف).
  const health = await req("/v1/health");
  must(health.status === 200 && health.data?.ok === true, "server health endpoint is available");
  const anonymous = await req("/v1/world/snapshot");
  assert(anonymous.status === 401 || anonymous.status === 403, "world snapshot rejects unauthenticated access");

  // 1 — تسجيل ضيف: حساب جديد + رمز موقع بأمان، بلا لاعب قبل تأسيس المدينة.
  const guest = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: `p7t1-${crypto.randomBytes(6).toString("hex")}` },
  });
  let token = must(guest.status === 200 && guest.data?.token, "guest authentication returns a signed token");
  const accountId = must(guest.data?.accountId, "guest authentication returns the account id");
  assert(guest.data?.player === null, "new guest has no player until city foundation (FTUE gating)");

  // 2 — قياس الهوية قبل تأسيس المدينة: حساب موجود بلا لاعب.
  const meBefore = await req("/v1/me", { token });
  assert(meBefore.status === 200 && meBefore.data?.account?.id === accountId && meBefore.data?.player === null,
    "/v1/me exposes the account identity and a null player before onboarding");

  // 3 — تأسيس المدينة (FTUE): اختيار حضارة وMatchmaking سلطوي ورمز محدّث وقائد بداية.
  const founded = await req("/v1/city/init", {
    method: "POST",
    token,
    body: { civ: "egypt", name: "T1-Governor" },
  });
  const newToken = founded.data?.token;
  if (newToken) token = newToken;
  must(founded.status === 200 && founded.data?.player?.id, "city foundation succeeds with a server-assigned player");
  const player = founded.data.player;
  assert(player.civ === "egypt" && player.name === "T1-Governor", "server honors the chosen civilization and governor name");
  const starter = founded.data?.starterCommander;
  must(starter?.commanderId && starter?.level === 1, "city foundation grants the starter commander (FTUE roster)");
  assert(typeof founded.data?.kingdom === "string", "server assigns the player to a kingdom (matchmaking)");
  assert(typeof player.x === "number" && typeof player.y === "number", "server places the city at authoritative coordinates");

  // 4 — الهوية بعد التأسيس: القياس يظهر اللاعب الجديد.
  const meAfter = await req("/v1/me", { token });
  assert(meAfter.status === 200 && meAfter.data?.player?.id === player.id && meAfter.data?.player?.civ === "egypt",
    "/v1/me reflects the newly founded player with its civilization");

  // 5 — القادة: قائمة الروستر + المملوكين (بما في ذلك قائد البداية) من البيانات المركزية.
  const commanders = await req("/v1/commanders", { token });
  must(commanders.status === 200 && Array.isArray(commanders.data?.commanders), "commander list is available to the onboarded player");
  assert(commanders.data.commanders.some((c) => c.commanderId === starter.commanderId), "starter commander appears in the owned list");
  const roster = commanders.data?.roster;
  const rosterCount = roster ? (Array.isArray(roster) ? roster.length : (roster.commanders?.length || Object.keys(roster).length)) : 0;
  assert(rosterCount >= 18, `central commander roster (data/commanders.json, 18 commanders) is served alongside owned list (got ${rosterCount})`);

  // 6 — لقطة العالم: اللاعب الجديد يرى مملكته من أول طلب.
  const snapshot = await req("/v1/world/snapshot", { token });
  must(snapshot.status === 200 && Array.isArray(snapshot.data?.cities), "authenticated world snapshot returns the kingdom state");
  assert(snapshot.data.cities.some((city) => city.playerId === player.id), "the new city is visible in the world snapshot");
  assert(typeof snapshot.data?.seasonDay === "number", "season day is part of the snapshot (story readiness)");

  // 7 — العودة اللاحقة: تسجيل جديد بجهاز موجود يستعيد الحساب نفسه (بدون City إعادة إنشاء).
  const returning = await req("/v1/auth/guest", {
    method: "POST",
    body: { deviceId: guest.data.deviceId || "reuse" },
  });
  if (returning.status === 200 && returning.data?.accountId === accountId) {
    assert(true, "returning guest re-authenticates to the same account (session persistence)");
    const meAgain = await req("/v1/me", { token: returning.data.token });
    assert(meAgain.status === 200 && meAgain.data?.player?.id === player.id, "returning session sees the same player after onboarding");
  } else {
    // إعادة التسجيل بجهاز محفوظ ليست شرطًا حاسمًا إذا تغير سلوك الـ deviceId؛ وثّق الفحص فقط.
    assert(false, "returning guest re-authenticates to the same account (documented, not blocking)");
  }

  console.log(failed === 0 ? `\nE2E P7-T1 PASSED: guest onboarding journey verified (${BASE}).` : `\nE2E P7-T1 FAILED (${failed} failures).`);
  process.exit(failed === 0 ? 0 : 1);
}

withSandboxedServer(main).catch((err) => {
  console.error("Fatal:", err.message);
  cleanup();
  process.exit(2);
});
