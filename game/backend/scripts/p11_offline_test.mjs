// P11-T3/T4 offline guard — يعيد تنفيذ منطق Lost Kingdom محليًا (لا يستورد TS) ويختبر:
// الهجرة + هيرون + قلاع/زيقورة + متجر عملات + حدود JSON + ثوابت زمنية anti-hardcode.
import fs from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";
import assert from "node:assert";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const DATA = path.resolve(HERE, "../src/data");
const SRC = path.resolve(HERE, "../src");
const lk = JSON.parse(fs.readFileSync(`${DATA}/lost_kingdom.json`, "utf8"));
const ac = JSON.parse(fs.readFileSync(`${DATA}/anticheat.json`, "utf8"));
const zones = JSON.parse(fs.readFileSync(`${DATA}/zones.json`, "utf8"));
const spec = JSON.parse(fs.readFileSync(`${DATA}/map_spec_coordinates.json`, "utf8"));

// ---- إعادة تنفيذ منطق sim محليًا ----
const MS_HOUR = 86400000 / 24;
function defaults(s) {
  const structures = [];
  for (let i = 0; i < s.hierons.count; i++) structures.push({ id: `HIER_${i}`, owner: "", hp: s.hierons.capture_points });
  for (let i = 0; i < s.sanctuaries.count; i++) structures.push({ id: `SANC_${i}`, owner: "", hp: s.hierons.capture_points });
  const citadels = [];
  for (let i = 0; i < s.citadels.count; i++) citadels.push({ id: `CIT_${i}`, hp: s.citadels.hp, destroyed: false });
  return { structures, citadels, ziggurat: { hp: s.great_ziggurat.hp, open: false, destroyed: false }, migration: { migrated: false, migrated_ms: 0, last_migrated_ms: 0 }, kvk_coins: 0, crown_points: 0, kingdom_points: 0 };
}
function canMigrate(state, hall, marches, now) {
  if (hall < lk.migration.min_city_hall_level) return "city_hall_too_low";
  if (marches) return "active_marches";
  if (state.migration.last_migrated_ms && now - state.migration.last_migrated_ms < lk.migration.cooldown_days * MS_HOUR * 24) return "migration_cooldown";
  return null;
}
function captureHieron(state, id, kingdom, now) {
  const idx = state.structures.findIndex(x => x.id === id && x.id.startsWith("HIER_"));
  if (idx < 0) return { error: "unknown_hieron", state };
  const x = state.structures[idx];
  if (x.owner === kingdom) return { error: "already_owned", state };
  state.structures[idx] = { ...x, owner: kingdom, captured_ms: now };
  state.kvk_coins += lk.structures.hierons.reward_coins;
  return { reward: lk.structures.hierons.reward_coins, state };
}
function destroyCitadel(state, id, damage, now) {
  const idx = state.citadels.findIndex(c => c.id === id);
  if (idx < 0) return { error: "unknown_citadel", state };
  const c = { ...state.citadels[idx], hp: Math.max(0, state.citadels[idx].hp - damage) };
  const destroyed = c.hp <= 0;
  state.citadels[idx] = { ...c, destroyed, destroyed_by: destroyed ? "K1" : "" };
  const d = state.citadels.filter(c => c.destroyed).length;
  if (destroyed) {
    state.kvk_coins += lk.constants.citadel_destroy_reward_coins;
    state.ziggurat.hp = Math.max(0, state.ziggurat.hp - Math.ceil(lk.structures.great_ziggurat.hp * lk.structures.citadels.ziggurat_damage_pct / 100));
    if (d >= lk.constants.ziggurat_open_after_citadels) state.ziggurat.open = true;
  }
  return { destroyed, state };
}
function attackZig(state, damage) {
  if (!state.ziggurat.open || state.ziggurat.destroyed) return { error: "not_open", state };
  state.ziggurat.hp -= damage;
  if (state.ziggurat.hp <= 0) {
    state.ziggurat.destroyed = true;
    state.crown_points += lk.constants.final_crown_points;
    state.kvk_coins += lk.constants.ziggurat_destroy_reward_coins;
    return { crowned: "K1", state };
  }
  return { state };
}
function buyItem(state, itemId) {
  const it = lk.constants.season_store.items.find(i => i.id === itemId);
  if (!it) return { error: "unknown_item", state };
  if (state.kvk_coins < it.cost) return { error: "insufficient_kvk_coins", state };
  state.kvk_coins -= it.cost;
  return { item: it, state };
}

let fails = 0, checks = 0;
function check(name, cond) { checks++; if (!cond) { console.log("FAIL:", name); fails++; } }

// ---- بيانات lost_kingdom.json ----
check("ملف lost_kingdom.json موجود", !!lk.constants);
check("constants.ziggurat_total_hp=1000000", lk.constants.ziggurat_total_hp === 1000000);
check("citadels.count=4", lk.structures.citadels.count === 4);
check("ziggurat_damage_pct=25 لكل قلعة", lk.structures.citadels.ziggurat_damage_pct === 25);
check("hierons.count=16", lk.structures.hierons.count === 16);
check("sanctuaries.count=4", lk.structures.sanctuaries.count === 4);
check("ancient_ruins.count=8", lk.structures.ancient_ruins.count === 8);
check("circles.count=12", lk.structures.circles.count === 12);
check("altars.count=4", lk.structures.altars_of_darkness.count === 4);
check("great_ziggurat id=great_ziggurat", lk.structures.great_ziggurat.id === "great_ziggurat");
check("final_battle 72 ساعة", lk.constants.ziggurat_final_battle_duration_hours === 72);
check("crown_points=5000", lk.constants.final_crown_points === 5000);
check("KVك symbol=KC", lk.constants.kvk_coin_symbol === "KC");

// ---- هجرة ----
let st = defaults(lk.structures);
check("هجرة CH15 مرفوضة", canMigrate(st, 15, false, 100) === "city_hall_too_low");
check("هجرة مع مسيرات مرفوضة", canMigrate(st, 16, true, 100) === "active_marches");
const res = canMigrate(st, 16, false, 1000);
check("هجرة CH16 بلا مسيرات مقبولة", res === null);
const now = 1_000_000;
st.migration = { migrated: true, migrated_ms: now, last_migrated_ms: now };
check("cooldown 30 يومًا يُرفض", canMigrate(st, 16, false, now + MS_HOUR * 24 * 10) === "migration_cooldown");
check("بعد 30 يومًا تُقبل", canMigrate(st, 16, false, now + lk.migration.cooldown_days * MS_HOUR * 24 + 1) === null);

// ---- هيرون ----
st = defaults(lk.structures);
let r = captureHieron(st, "HIER_0", "K1", 500);
check("استيلاء هيرون يمنح 50 KC", r.reward === lk.structures.hierons.reward_coins && st.kvk_coins === 50);
r = captureHieron(st, "HIER_0", "K1", 600);
check("هيرون مملوك يُرفض", r.error === "already_owned");
r = captureHieron(st, "HIER_X", "K2", 600);
check("هيرون مجهول يُرفض", r.error === "unknown_hieron");

// ---- قلاع + زيقورة ----
st = defaults(lk.structures);
r = destroyCitadel(st, "CIT_0", 500000, 700);
check("قلعة HP تنقص (500000)", r.state.citadels[0].hp === 0 && r.destroyed);
check("زيقورة HP نقصت 25%", r.state.ziggurat.hp === 750000);
check("زيقورة مغلقة بعد قلعة واحدة", !r.state.ziggurat.open);
for (let i = 1; i < 4; i++) r = destroyCitadel(r.state, `CIT_${i}`, 500000, 700);
check("تدمير 4 قلاع يفتح الزيقورة", r.state.ziggurat.open && r.state.ziggurat.hp === 0 && r.state.ziggurat.destroyed === false);
check("مكافأة قلعة 500 KC", r.state.kvk_coins === 2000);
check("مجموع damage = 100% HP الزيقورة", r.state.ziggurat.hp === 0);

// ---- معركة نهائية ----
let zState = defaults(lk.structures);
zState.ziggurat.hp = 100; zState.ziggurat.open = true;
zState.citadels.forEach(c => { c.destroyed = true; c.destroyed_by = "K1"; });
r = attackZig(zState, 100);
check("تدمير الزيقورة تتويج", r.crowned === "K1");
check("crown_points=5000", r.state.crown_points === 5000);
check("مكافأة تدمير 2000 KC", r.state.kvk_coins === 2000);
r = attackZig(r.state, 10);
check("زيقورة مدمرة تُرفض", r.error === "not_open");

// ---- متجر ----
st = defaults(lk.structures);
st.kvk_coins = 150;
r = buyItem(st, "unknown_item_x");
check("عنصر مجهول يُرفض", r.error === "unknown_item");
r = buyItem(st, lk.constants.season_store.items[0].id);
check("شراء ناجح يخصم العملات", r.error === undefined && st.kvk_coins === 150 - lk.constants.season_store.items[0].cost);
st.kvk_coins = 5;
r = buyItem(st, lk.constants.season_store.items[0].id);
check("عملات غير كافية", r.error === "insufficient_kvk_coins");

// ---- خرائط ----
check("7 zones (Z1-Z7)", zones.zones.length === 7);
const lkZones = zones.zones.filter(z => z.key && z.key.startsWith("lost_"));
check("4 zones لـ Lost Kingdom", lkZones.length === 4);
check("Z7 دور kvk_ziggurat", zones.zones[6].role === "kvk_ziggurat");
check("region LK_CORE موجودة", spec.regions.some(r => r.id === "LK_CORE"));
check("26 region إجماليًا", spec.regions.length === 26);
check("GZ_1 داخل العالم", lk.structures.great_ziggurat.pos[0] <= 2400);
check("lk gates في passes", lk.structures && JSON.parse(fs.readFileSync(`${DATA}/passes.json`, "utf8")).passes.some(p => p.id.startsWith("LK_")));

// ---- anti-hardcode ----
const shardSrc = fs.readFileSync(`${SRC}/do/KingdomShard.ts`, "utf8");
const lkSrc = fs.readFileSync(`${SRC}/do/sim/lost_kingdom.ts`, "utf8");
check("لا 86400000 حرفي في KingdomShard", !shardSrc.includes("86400000"));
check("لا 3600000 حرفي في KingdomShard", !shardSrc.includes("3640000") && !shardSrc.includes("3600000"));
check("لا literals في sim/lost_kingdom.ts", !lkSrc.includes("86400000") && !lkSrc.includes("3600000"));
check("sim يستورد MS_PER_HOUR من timeConstants", lkSrc.includes('MS_PER_HOUR') && lkSrc.includes("timeConstants"));

// ---- anticheat ----
for (const k of ["lk_migrate", "lk_hieron_capture", "lk_citadel_attack", "lk_ziggurat_attack", "lk_season_buy"]) {
  check(`حد ${k} في anticheat.json`, !!ac.rate_limits[k]);
  check(`حد ${k} في anticheat.ts fallback`, fs.readFileSync(`${SRC}/do/sim/anticheat.ts`, "utf8").includes(k));
}

// ---- router + shard endpoints ----
const routerSrc = fs.readFileSync(`${SRC}/http/router.ts`, "utf8");
check("endpoint /v1/lk/state", routerSrc.includes('"/v1/lk/state"'));
check("endpoint /v1/lk/migrate", routerSrc.includes('"/v1/lk/migrate"'));
check("endpoint /v1/lk/hieron", routerSrc.includes('"/v1/lk/hieron"'));
check("endpoint /v1/lk/citadel", routerSrc.includes('"/v1/lk/citadel"'));
check("endpoint /v1/lk/ziggurat", routerSrc.includes('"/v1/lk/ziggurat"'));
check("endpoint /v1/lk/season-buy", routerSrc.includes('"/v1/lk/season-buy"'));
check("getLostKingdomJson في gameData.ts", fs.readFileSync(`${SRC}/lib/gameData.ts`, "utf8").includes("getLostKingdomJson"));
check("migration 18 في KingdomShard", shardSrc.includes("VALUES (18)"));
check("lk_state جدول", shardSrc.includes("lk_state"));
check("loadLKState", shardSrc.includes("loadLKState"));
check("persistLK", shardSrc.includes("persistLK"));
check("handlers lk-migrate/.../lk-state في shard", ["lk-migrate", "lk-hieron", "lk-citadel", "lk-ziggurat", "lk-season-buy", "lk-state"].every(e => shardSrc.includes(`/${e}`)));

if (fails > 0) { console.log(`\nFAILED ASSERTIONS: ${fails}`); process.exit(1); }
console.log(`ALL PASSED: ${checks} checks, ${fails} failed`);
process.exit(0);
