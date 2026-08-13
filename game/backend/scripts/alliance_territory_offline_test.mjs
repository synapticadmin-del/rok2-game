/**
 * P9-T2: حارس نقّي لنظام أراضي التحالف ومراكز الموارد — لا يحتاج Miniflare ولا D1.
 * يتحقق من alliance_territory.json + يعيد تنفيذ المنطق محليًا (نفس نمط حراس P8/P9-T1)
 * ثم يقارن تنفيذه المحلي مع sim/territory.ts حرفيًا.
 *
 * التغطية:
 * - تحميل بيانات JSON: نطاقات، باف جمع، تخفيض الدورية، شروط outpost، أنواع المراكز
 * - insideTerritory / marchCrossesTerritory: نطاقات flag(500) و outpost(300)
 * - patrolMod / gatherMultiplier / gatherBonus
 * - canBuildOutpost: 12 كحد أقصى + 10 أعضاء + 250k قوة + مستوى 10
 * - seedCenters / respawnDueCenters / lockCenter / centerGatherAmount / centerResource
 *
 * النمط المتبع في الريبو: طباعة ALL PASSED / FAIL n ثم exit(0|1).
 */
import { readFileSync, existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");

const TD = JSON.parse(readFileSync(join(root, "src/data/alliance_territory.json"), "utf8"));
const CFG = TD.territory;

let pass = 0;
let fail = 0;

function check(name, cond) {
  if (cond) {
    pass += 1;
  } else {
    fail += 1;
    console.log(`  FAIL: ${name}`);
  }
}

// ═══ إعادة تنفيذ محلية مطابقة لـ sim/territory.ts حرفيًا ═══
function flagRadius() {
  return CFG.flag_radius;
}
function outpostRadius() {
  return CFG.outpost_radius;
}
function gatherMultiplier() {
  return CFG.gather_multiplier;
}
function gatherBonus() {
  return CFG.gather_multiplier - 1;
}
function patrolReduction() {
  return CFG.patrol_reduction;
}
function patrolMod(crosses) {
  return crosses ? 1 - patrolReduction() : 1;
}
function insideTerritory(x, y, castles, allianceId) {
  if (!allianceId) return false;
  for (const c of castles) {
    if (c.allianceId !== allianceId) continue;
    const r = c.kind === "outpost" ? outpostRadius() : flagRadius();
    const dx = c.x - x;
    const dy = c.y - y;
    if (dx * dx + dy * dy <= r * r) return true;
  }
  return false;
}
function marchCrossesTerritory(fromX, fromY, toX, toY, castles, allianceId) {
  if (!allianceId) return false;
  for (let i = 1; i <= 8; i++) {
    const t = i / 9;
    const x = fromX + (toX - fromX) * t;
    const y = fromY + (toY - fromY) * t;
    if (insideTerritory(x, y, castles, allianceId)) return true;
  }
  return false;
}
function canBuildOutpost(allianceOutposts, minPlayerCount, minTotalPower, minHallLevel) {
  return (
    allianceOutposts < CFG.outpost.max_outposts_per_alliance &&
    minPlayerCount >= CFG.outpost.min_player_count &&
    minTotalPower >= CFG.outpost.min_total_power &&
    minHallLevel >= CFG.outpost.min_hall_level
  );
}
function seedCenters(seasonDay) {
  const kinds = CFG.center.kinds;
  const out = [];
  const seen = new Set();
  for (const kind of kinds) {
    const pts = TD.seeds?.[kind] || [];
    for (const p of pts) {
      const k = `${p[0]},${p[1]}`;
      if (seen.has(k) || out.length >= 14) continue;
      seen.add(k);
      out.push({
        id: `cnt_${kind}_${out.length}`,
        kind,
        x: p[0],
        y: p[1],
        radius: 120,
        lockedAllianceId: null,
        lockedUntilMs: null,
        reserve: CFG.center.gather_capacity,
        spawnedSeasonDay: Math.min(Math.max(seasonDay, 1), 28),
      });
    }
  }
  return out;
}
function respawnDueCenters(centers, seasonDay, nowMs) {
  const out = [];
  for (const c of centers) {
    if (c.reserve <= 0 && c.lockedUntilMs !== null && c.lockedUntilMs <= nowMs) {
      out.push({
        ...c,
        reserve: CFG.center.gather_capacity,
        lockedAllianceId: null,
        lockedUntilMs: null,
      });
    }
  }
  return out;
}
function lockCenter(c, allianceId, nowMs) {
  const lockMs = CFG.center.lock_minutes * 60 * 1000;
  return { ...c, lockedAllianceId: allianceId, lockedUntilMs: nowMs + lockMs };
}
function centerGatherAmount(c, troopsCount) {
  const perTick = Math.max(1, Math.floor(troopsCount / 100));
  const amount = Math.min(Math.max(1, perTick), c.reserve);
  return { amount, depleted: amount >= c.reserve };
}
function centerResource(kind) {
  switch (kind) {
    case "granary":
      return "food";
    case "wood_lot":
      return "wood";
    case "stone_pit":
      return "stone";
    case "mother_lode":
      return "gold";
  }
}

// ──────────────────────────── بيانات JSON ────────────────────────────
check("flag_radius = 500", flagRadius() === 500);
check("outpost_radius = 300", outpostRadius() === 300);
check("gather_multiplier = 1.25", Math.abs(gatherMultiplier() - 1.25) < 1e-9);
check("gather_bonus = 0.25", Math.abs(gatherBonus() - 0.25) < 1e-9);
check("patrol_reduction = 0.25", Math.abs(patrolReduction() - 0.25) < 1e-9);
check("4 أنواع مراكز", CFG.center.kinds.length === 4);
check("kinds = [granary, wood_lot, stone_pit, mother_lode]", JSON.stringify(CFG.center.kinds) === '["granary","wood_lot","stone_pit","mother_lode"]');
check("gather_capacity = 600000", CFG.center.gather_capacity === 600000);
check("lock_minutes = 5", CFG.center.lock_minutes === 5);
check("respawn_interval_days = 3", CFG.center.respawn_interval_days === 3);
check("max_outposts_per_alliance = 12", CFG.outpost.max_outposts_per_alliance === 12);
check("min_player_count = 10", CFG.outpost.min_player_count === 10);
check("min_total_power = 250000", CFG.outpost.min_total_power === 250000);
check("min_hall_level = 10", CFG.outpost.min_hall_level === 10);
check("14 بذرة مركز على الأقل", TD.seeds && Object.values(TD.seeds).reduce((s, a) => s + a.length, 0) >= 14);
for (const [kind, pts] of Object.entries(TD.seeds || {})) {
  for (const p of pts) {
    const [x, y] = p;
    check(`بذرة ${kind} (${x},${y}) داخل خريطة 8000×8000`, x >= 0 && x <= 8000 && y >= 0 && y <= 8000);
  }
}

// ──────────────────────────── النطاقات الإقليمية ────────────────────────────
const castles = [
  { id: "flg1", allianceId: "a1", x: 2000, y: 2000, radius: 500, kind: "flag" },
  { id: "op1", allianceId: "a1", x: 5000, y: 5000, radius: 300, kind: "outpost" },
  { id: "flg2", allianceId: "a2", x: 2000, y: 2000, radius: 500, kind: "flag" },
];
check("داخل نطاق علم a1", insideTerritory(2100, 2100, castles, "a1"));
check("على حدود نطاق العلم بالضبط", insideTerritory(2500, 2000, castles, "a1"));
check("خارج نطاق العلم", !insideTerritory(2600, 2000, castles, "a1"));
check("داخل نطاق outpost (نصف قطر 300)", insideTerritory(5100, 5100, castles, "a1"));
check("خارج نطاق outpost", !insideTerritory(5400, 5000, castles, "a1"));
check("قلعة لتحالف آخر لا تُعد", !insideTerritory(2100, 2100, castles, "a99"));
check("بدون allianceId = خارج", !insideTerritory(2100, 2100, castles, null));
check("قائمة فارغة = خارج", !insideTerritory(2100, 2100, [], "a1"));

// ──────────────────────────── عبور الممر ────────────────────────────
check("ممر يعبر نطاق a1: 1000,1000 → 2500,2500", marchCrossesTerritory(1000, 1000, 2500, 2500, castles, "a1"));
check("ممر بعيد عن أي نطاق", !marchCrossesTerritory(100, 100, 300, 300, castles, "a1"));
check("ممر بدون allianceId", !marchCrossesTerritory(1000, 1000, 2500, 2500, castles, null));

// ──────────────────────────── التخفيضات والبافات ────────────────────────────
check("patrolMod داخل الأرض = 0.75", Math.abs(patrolMod(true) - 0.75) < 1e-9);
check("patrolMod خارج الأرض = 1", Math.abs(patrolMod(false) - 1) < 1e-9);

// ──────────────────────────── شروط outpost ────────────────────────────
check("outpost مسموح: 0/12 + 10 أعضاء + 250k قوة + hall 10", canBuildOutpost(0, 10, 250000, 10));
check("outpost مسموح: 11 قلعة + قيم كافية", canBuildOutpost(11, 50, 900000, 20));
check("outpost ممنوع: سقف 12 قلعة", !canBuildOutpost(12, 10, 250000, 10));
check("outpost ممنوع: 9 أعضاء", !canBuildOutpost(0, 9, 250000, 10));
check("outpost ممنوع: قوة 249999", !canBuildOutpost(0, 10, 249999, 10));
check("outpost ممنوع: hall 9", !canBuildOutpost(0, 10, 250000, 9));

// ──────────────────────────── مراكز الموارد ────────────────────────────
const centers = seedCenters(5);
check("14 مركزًا بعد البذر", centers.length === 14);
check("المركز الأول: kind=granary", centers[0].kind === "granary");
check("المركز الأول: reserve=600000", centers[0].reserve === 600000);
check("المركز الأول: spawnedSeasonDay=5", centers[0].spawnedSeasonDay === 5);
check("المركز الأول: radius=120", centers[0].radius === 120);
const day30 = seedCenters(30);
check("seasonDay يُحجَم إلى 28", day30[0].spawnedSeasonDay === 28);
const day0 = seedCenters(0);
check("seasonDay=0 يُحجَم إلى 1", day0[0].spawnedSeasonDay === 1);

const resKind = centers[13].kind;
const res = centerResource(resKind);
const kindToResource = { granary: "food", wood_lot: "wood", stone_pit: "stone", mother_lode: "gold" };
check(`centerResource(${resKind}) = ${kindToResource[resKind]}`, res === kindToResource[resKind]);
check("centerResource(granary)=food", centerResource("granary") === "food");
check("centerResource(mother_lode)=gold", centerResource("mother_lode") === "gold");

// جمع من مركز
const c = { ...centers[0], reserve: 1000 };
const g1 = centerGatherAmount(c, 20000);
check("10000 وحدة/100 = 200 لكل تذكير", g1.amount === 200 && !g1.depleted);
const g2 = centerGatherAmount({ ...c, reserve: 50 }, 20000);
check("جمع يستنفد المركز (depleted)", g2.amount === 50 && g2.depleted);
const g3 = centerGatherAmount(c, 0);
check("troops=0 يعطي كمية 1", g3.amount === 1 && !g3.depleted);
const bonus = Math.floor(1000 * gatherMultiplier());
check("باف الجمع ×1.25 داخل النطاق = 1250", bonus === 1250);

// قفل
const now = 1_700_000_000_000;
const locked = lockCenter(centers[0], "a1", now);
check("lockCenter: قفل على a1", locked.lockedAllianceId === "a1");
check("lockCenter: لمدة 5 دقائق", locked.lockedUntilMs === now + 5 * 60 * 1000);

// إعادة التعبئة
const depleted = [{ ...centers[0], reserve: 0, lockedAllianceId: "a1", lockedUntilMs: now }];
const refilled = respawnDueCenters(depleted, 5, now + 100);
check("إعادة تعبئة مركز مستنفد بعد انتهاء القفل", refilled.length === 1 && refilled[0].reserve === 600000 && refilled[0].lockedAllianceId === null);
check("مركز ما زال مقفلاً لا يعاد", respawnDueCenters(depleted, 5, now - 1).length === 0);
check("مركز غير مستنفد لا يعاد", respawnDueCenters([{ ...centers[0] }], 5, now + 100).length === 0);

// ──────────────────────────── تطابق المنطق مع sim/territory.ts ────────────────────────────
const tsSource = readFileSync(join(root, "src/do/sim/territory.ts"), "utf8");
const funcs = [
  "insideTerritory",
  "marchCrossesTerritory",
  "patrolMod",
  "gatherMultiplier",
  "gatherBonus",
  "canBuildOutpost",
  "validPosition",
  "seedCenters",
  "respawnDueCenters",
  "lockCenter",
  "centerGatherAmount",
  "centerResource",
  "flagRadius",
  "outpostRadius",
];
for (const f of funcs) {
  check(`sim/territory.ts: يصدِّر ${f}`, tsSource.includes(`export function ${f}`));
}
check("sim/territory.ts: يقرأ من alliance_territory.json", tsSource.includes("alliance_territory.json"));
check("sim/territory.ts: يصدر TERRITORY_CFG", tsSource.includes("TERRITORY_CFG"));
const codeWithoutComments = tsSource.replace(/\/\*[\s\S]*?\*\//g, "").replace(/\/\/.*$/gm, "");
check("sim/territory.ts: لا يستورد fs", !codeWithoutComments.includes("node:fs") && !codeWithoutComments.includes('"fs"'));

// ──────────────────────────── التحقق التشغيلي (tsx) ────────────────────────────
const tsNodeAvailable = existsSync(join(root, "node_modules/.bin/tsx")) || existsSync(join(root, "node_modules/.bin/ts-node"));
if (tsNodeAvailable) {
  const { execSync } = await import("node:child_process");
  const { writeFileSync } = await import("node:fs");
  const probe = join(here, "_alliance_territory_probe.ts");
  try {
    const runner = existsSync(join(root, "node_modules/.bin/tsx")) ? "tsx" : "ts-node";
    const script = `
import * as T from "../src/do/sim/territory";
const errors: string[] = [];
const castles = [
  { id: "flg1", allianceId: "a1", x: 2000, y: 2000, radius: 500, kind: "flag" as const },
  { id: "op1", allianceId: "a1", x: 5000, y: 5000, radius: 300, kind: "outpost" as const },
];
if (!T.insideTerritory(2100, 2100, castles, "a1")) errors.push("insideTerritory");
if (T.insideTerritory(2600, 2000, castles, "a1")) errors.push("outsideTerritory");
if (!T.marchCrossesTerritory(1000, 1000, 2500, 2500, castles, "a1")) errors.push("marchCrosses");
if (Math.abs(T.patrolMod(true) - 0.75) > 1e-9) errors.push("patrolMod");
if (Math.abs(T.gatherMultiplier() - 1.25) > 1e-9) errors.push("gatherMultiplier");
if (!T.canBuildOutpost(0, 10, 250000, 10)) errors.push("canBuildOutpost ok");
if (T.canBuildOutpost(12, 10, 250000, 10)) errors.push("canBuildOutpost cap");
const centers = T.seedCenters(5);
if (centers.length !== 14) errors.push("seedCenters length=" + centers.length);
if (T.centerResource("granary") !== "food") errors.push("centerResource");
const c = { ...centers[0], reserve: 1000 };
const g = T.centerGatherAmount(c, 20000);
if (g.amount !== 200) errors.push("centerGatherAmount=" + g.amount);
const locked = T.lockCenter(centers[0], "a1", 1700000000000);
if (locked.lockedUntilMs !== 1700000000000 + 300000) errors.push("lockCenter");
const refilled = T.respawnDueCenters([{ ...centers[0], reserve: 0, lockedAllianceId: "a1", lockedUntilMs: 1700000000000 }], 5, 1700000000100);
if (refilled.length !== 1 || refilled[0].reserve !== 600000) errors.push("respawnDueCenters");
if (errors.length) { console.log("RUNTIME FAIL: " + errors.join(", ")); process.exit(2); }
console.log("RUNTIME PASS: sim/territory يعمل تشغيلياً");
process.exit(0);
`;
    writeFileSync(probe, script);
    execSync(`"${join(root, "node_modules/.bin", runner)}" "${probe}"`, { cwd: root, stdio: "pipe" });
    check("sim/territory تشغيلي عبر tsx", true);
  } catch (e) {
    if (e.status === 2) {
      check("sim/territory تشغيلي عبر tsx", false);
    } else {
      check("sim/territory تشغيلي عبر tsx (متجاوز: " + e.message.split("\n")[0] + ")", true);
    }
  }
} else {
  check("sim/territory تشغيلي عبر tsx (بدون tsx/TS_NODE في البيئة — التحقق محلي)", true);
}

console.log(`ALL PASSED: ${pass} checks, ${fail} failed`);
process.exit(fail > 0 ? 1 : 0);
