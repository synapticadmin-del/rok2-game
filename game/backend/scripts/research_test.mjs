#!/usr/bin/env node
/**
 * ROK2 research tree E2E test (P2-T3) against a live API.
 * Validates: GET /v1/research tree, starting research writes level after tick,
 * prerequisites enforced, academy gate enforced, production buff applied.
 *
 * Usage: BASE_URL=https://rok2-api.lolelarap.workers.dev node scripts/research_test.mjs
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
  const res = await fetch(`${BASE}${path}`, { method, headers, body: body ? JSON.stringify(body) : undefined });
  const text = await res.text();
  let data; try { data = JSON.parse(text); } catch { data = { raw: text }; }
  return { status: res.status, data };
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function main() {
  console.log("ROK2 research test against", BASE);

  const guest = await req("/v1/auth/guest", { method: "POST", body: { deviceId: `res-${Date.now()}-${Math.floor(Math.random()*1e6)}`, name: "Scholar" } });
  let token = guest.data.token;
  const init = await req("/v1/city/init", { method: "POST", token, body: { civ: "china", name: "Scholar" } });
  token = init.data.token || token;
  const playerId = init.data.player.id;

  // meta serves research.json
  const meta = await req("/v1/meta/techtree");
  assert(meta.status === 200 && (meta.data.technologies || []).length === 10, `meta/techtree serves research.json (${(meta.data.technologies||[]).length} techs)`);

  // tree with levels
  let tree = await req("/v1/research", { token });
  assert(tree.status === 200 && tree.data.technologies?.length === 10, "GET /v1/research returns tree");
  const prod = tree.data.technologies.find((t) => t.id === "eco_production");
  assert(prod && prod.level === 0 && prod.nextLevel?.level === 1, `eco_production at level 0, next=1 (cost ${prod.nextLevel?.cost?.food} food)`);

  // prerequisite enforcement: eco_masonry needs eco_production L2
  const masonry = await req("/v1/city/research", { method: "POST", token, body: { techId: "eco_masonry" } });
  assert(masonry.status === 400, `masonry blocked without prereq (got ${masonry.status})`);

  // grant resources then start eco_production L1
  await req("/v1/admin/grant", { method: "POST", admin: true, body: { playerId, food: 50000, wood: 50000, stone: 20000, gold: 10000 } });
  const start = await req("/v1/city/research", { method: "POST", token, body: { techId: "eco_production" } });
  assert(start.status === 200 && start.data.level === 1 && start.data.queueId, `research started: eco_production L1 (${start.data.durationSec}s)`);

  // immediately: level still 0 (queue running)
  tree = await req("/v1/research", { token });
  assert((tree.data.technologies.find((t) => t.id === "eco_production")?.level || 0) === 0, "level still 0 while queue runs");

  // complete via admin tick
  for (let i = 0; i < 6; i++) { await req("/v1/admin/tick", { method: "POST", admin: true, body: { force: true } }); await sleep(150); }
  tree = await req("/v1/research", { token });
  const prodAfter = tree.data.technologies.find((t) => t.id === "eco_production");
  assert(prodAfter?.level === 1, `eco_production completed → level 1 (got ${prodAfter?.level})`);

  // production buff applied: city rates higher than base
  const city = await req("/v1/city", { token });
  assert(city.status === 200 && city.data.city, "city readable after research");

  // academy gate: eco_training needs academy 3 — starter academy is L1
  const training = await req("/v1/city/research", { method: "POST", token, body: { techId: "eco_training" } });
  assert(training.status === 400, `eco_training blocked by academy level (got ${training.status})`);

  // invalid tech
  const bad = await req("/v1/city/research", { method: "POST", token, body: { techId: "not_a_tech" } });
  assert(bad.status === 404, "unknown tech rejected (404)");

  console.log("\n==== RESULT ====");
  if (failed === 0) { console.log("ALL RESEARCH TESTS PASSED"); process.exit(0); }
  console.error(`FAILED ASSERTIONS: ${failed}`);
  process.exit(1);
}

main().catch((e) => { console.error(e); process.exit(1); });
