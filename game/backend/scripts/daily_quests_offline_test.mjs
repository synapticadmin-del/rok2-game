// P8-T6: حارس نقّي لنظام المهام اليومية/الأسبوعية — لا يحتاج Miniflare ولا D1.
// يتحقق من daily_quests.json + إعادة تنفيذ محلية مطابقة لمنطق src/do/sim/daily_quests.ts
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { chainRuns } from "../../../scripts/lib/npm_script_chain.mjs";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, "..");

let passed = 0;
let failed = 0;
function check(name, cond) {
  if (cond) { passed++; console.log(`  PASS: ${name}`); }
  else { failed++; console.log(`  FAIL: ${name}`); }
}

// قراءة JSON كـ raw object (نفس نمط import.meta في production)
const jsonPath = path.join(root, "src/data/daily_quests.json");
const questData = JSON.parse(fs.readFileSync(jsonPath, "utf8"));

console.log("P8-T6 Daily Quests offline guard");

// ── JSON structure ──────────────────────────────────────────────────────────
check("JSON version = 1", questData.version === 1);
check("5 مهام يومية", questData.constants.daily_quest_count === 5);
check("3 مهام أسبوعية", questData.constants.weekly_quest_count === 3);
check("سقف يومي 100", questData.constants.daily_points_limit === 100);
check("سقف أسبوعي 300", questData.constants.weekly_points_limit === 300);
check("refresh UTC 0", questData.constants.refresh_at_hour_utc === 0);

// ── types pools consistency ─────────────────────────────────────────────────
for (const typeId of questData.daily_order) {
  check(`daily pool موجود: ${typeId}`, Boolean(questData.daily_pools[typeId]));
  const pool = questData.daily_pools[typeId];
  check(`  goal_range سليم: ${typeId}`, Array.isArray(pool.goal_range) && pool.goal_range[0] <= pool.goal_range[1]);
  check(`  point_options سليم: ${typeId}`, Array.isArray(pool.point_options) && pool.point_options.every(p => p > 0));
}
for (const typeId of questData.weekly_order) {
  check(`weekly pool موجود: ${typeId}`, Boolean(questData.weekly_pools[typeId]));
}
check("8 أنواع مهام", Object.keys(questData.types).length === 8);
for (const [typeId, def] of Object.entries(questData.types)) {
  check(`  description يحتوي {goal}: ${typeId}`, def.description.includes("{goal}"));
  check(`  progress_sources غير فارغ: ${typeId}`, Array.isArray(def.progress_sources) && def.progress_sources.length > 0);
}

// ── deterministic distribution (reimplementation of seededRandom + hash) ────
function djb2(s) {
  let h = 5381;
  for (let i = 0; i < s.length; i++) h = ((h << 5) + h + s.charCodeAt(i)) >>> 0;
  return h;
}
function seededRandom(seed) {
  let s = seed >>> 0;
  return function () { s = (s * 1664525 + 1013904223) >>> 0; return s / 0x100000000; };
}
function questDay(nowMs, hourUtc) {
  const d = new Date(nowMs);
  const dayStart = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), d.getUTCDate(), hourUtc || 0, 0, 0));
  return Math.floor((d.getTime() - dayStart.getTime()) / 86400000);
}
function questWeek(nowMs, hourUtc) {
  const d = new Date(nowMs);
  const dayStart = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), d.getUTCDate(), hourUtc || 0, 0, 0));
  const day = Math.floor((d.getTime() - dayStart.getTime()) / 86400000);
  return Math.floor((dayStart.getTime() / 86400000 - 4) / 7);
}

const NOW = Date.UTC(2026, 7, 13, 12, 0, 0);
const day = questDay(NOW, questData.constants.refresh_at_hour_utc);
const week = questWeek(NOW, questData.constants.refresh_at_hour_utc);
check("questDay رقم صحيح", Number.isInteger(day));
check("questWeek رقم صحيح", Number.isInteger(week));

function buildDailyQuests(playerId, dayN, rng) {
  const order = questData.daily_order;
  const count = questData.constants.daily_quest_count;
  const picked = [];
  const available = order.slice();
  for (let i = 0; i < count; i++) {
    const idx = Math.floor(rng() * available.length);
    picked.push(available.splice(idx, 1)[0]);
  }
  let pointsSum = 0;
  const quests = picked.map((typeId) => {
    const pool = questData.daily_pools[typeId];
    const options = pool.point_options;
    const points = options[Math.floor(rng() * options.length)] ?? options[0];
    pointsSum += points;
    return { typeId, goal: pool.goal_range[0] + Math.floor(rng() * (pool.goal_range[1] - pool.goal_range[0] + 1)), points };
  });
  const limit = questData.constants.daily_points_limit;
  while (pointsSum > limit && quests.length > 0) {
    const biggest = quests.reduce((a, b) => (b.points > a.points ? b : a), quests[0]);
    const reduction = Math.min(biggest.points, pointsSum - limit);
    biggest.points -= reduction;
    pointsSum -= reduction;
  }
  return quests.map((q, i) => ({ ...q, id: `daily_${dayN}_${i}` }));
}

// حتمية: نفس اللاعب + نفس اليوم → نفس المهام
const rng1a = seededRandom(djb2(`p1|daily|${day}`));
const rng1b = seededRandom(djb2(`p1|daily|${day}`));
const q1a = buildDailyQuests("p1", day, rng1a);
const q1b = buildDailyQuests("p1", day, rng1b);
check("حتمية التوزيع: نفس اليوم نفس اللاعب", JSON.stringify(q1a) === JSON.stringify(q1b));

// تغيّر اليوم → توزيع مختلف (مع احتمال نظري ضئيل متطابق)
const rng2 = seededRandom(djb2(`p1|daily|${day - 1}`));
const q2 = buildDailyQuests("p1", day - 1, rng2);
check("تغيّر اليوم يغيّر التوزيع", JSON.stringify(q1a) !== JSON.stringify(q2));

// مختلف لاعبين في نفس اليوم → توزيع مختلف
const rng3 = seededRandom(djb2(`p2|daily|${day}`));
const q3 = buildDailyQuests("p2", day, rng3);
check("مختلف لاعبين → توزيع مختلف", JSON.stringify(q1a) !== JSON.stringify(q3));

// سقف النقاط: مجموع نقاط الخمس <= 100
const pointsSum = q1a.reduce((s, q) => s + q.points, 0);
check(`سقف النقاط اليومية محترم: ${pointsSum} <= 100`, pointsSum <= questData.constants.daily_points_limit);
check("5 مهام يوميًا", q1a.length === 5);

// weekly نفس الخصائص
function buildWeeklyQuests(playerId, weekN, rng) {
  const order = questData.weekly_order;
  const count = questData.constants.weekly_quest_count;
  const picked = [];
  const available = order.slice();
  for (let i = 0; i < count; i++) {
    const idx = Math.floor(rng() * available.length);
    picked.push(available.splice(idx, 1)[0]);
  }
  return picked.map((typeId, i) => {
    const pool = questData.weekly_pools[typeId];
    const options = pool.point_options;
    const points = options[Math.floor(rng() * options.length)] ?? options[0];
    return { id: `weekly_${weekN}_${i}`, typeId, goal: pool.goal_range[0] + Math.floor(rng() * (pool.goal_range[1] - pool.goal_range[0] + 1)), points };
  });
}

const rngW = seededRandom(djb2(`p1|weekly|${week}`));
const qw = buildWeeklyQuests("p1", week, rngW);
check("3 مهام أسبوعيًا", qw.length === 3);
for (const q of qw) {
  check(`  weekly pool سليم: ${q.typeId}`, Boolean(questData.weekly_pools[q.typeId]));
}

// ── progress / claim logic ──────────────────────────────────────────────────
function applyProgress(quest, source, amount) {
  const def = questData.types[quest.typeId];
  if (!def || !def.progress_sources.includes(source)) return { progress: quest.progress, pointsEarned: 0 };
  if (quest.claimed) return { progress: quest.progress, pointsEarned: 0 };
  const before = quest.progress;
  const after = Math.min(quest.goal, before + amount);
  return { progress: after, pointsEarned: after >= quest.goal && before < quest.goal ? quest.points : 0 };
}

const baseQuest = { typeId: "train_troops", goal: 300, points: 15, progress: 0, claimed: false };
let r = applyProgress(baseQuest, "train", 200);
check("progress يتراكم: 200/300", r.progress === 200 && r.pointsEarned === 0);
r = applyProgress({ ...baseQuest, progress: 200 }, "train", 150);
check("اكتمال يمنح النقاط: 350→capped عند 300", r.progress === 300 && r.pointsEarned === 15);
r = applyProgress({ ...baseQuest, progress: 290 }, "gather", 999);
check("مصدر خاطئ لا يقدّم", r.progress === 290 && r.pointsEarned === 0);
r = applyProgress({ ...baseQuest, goal: 100, progress: 0, claimed: true }, "train", 999);
check("مهمة مستردة لا تقدّم", r.progress === 0 && r.pointsEarned === 0);
r = applyProgress({ ...baseQuest, progress: 299 }, "train", 1);
check("اكتمال عند المساواة بالضبط", r.progress === 300 && r.pointsEarned === 15);
r = applyProgress({ ...baseQuest, progress: 301, claimed: false }, "train", 1);
check("تقدم مفرط لا يمنح نقاطًا مضاعفة", r.pointsEarned === 0);

// ── golden key / weekly chest eligibility ───────────────────────────────────
function goldenKeyEligible(points) { return points >= questData.rewards.golden_key_cost_points; }
check("مفتاح ذهبي عند 100", goldenKeyEligible(100) === true);
check("لا مفتاح عند 99", goldenKeyEligible(99) === false);
check("golden_key_gems = 200", questData.rewards.golden_key_gems === 200);
check("weekly chest = 500 gems + 2 speedup",
  questData.rewards.weekly_chest_gems === 500 &&
  questData.rewards.weekly_chest_speedups === 2 &&
  questData.rewards.weekly_chest_cost_points === 300);

// ── migration file ──────────────────────────────────────────────────────────
const migPath = path.join(root, "migrations/0012_daily_quests.sql");
check("ملف migration 0012 موجود", fs.existsSync(migPath));
const sql = fs.readFileSync(migPath, "utf8");
check("migration يحتوي player_quests", sql.includes("player_quests"));
check("migration يحتوي player_quest_points", sql.includes("player_quest_points"));
check("migration يحتوي player_quest_rewards", sql.includes("player_quest_rewards"));
check("PK على player_id+cycle+slot للمهام", sql.includes("PRIMARY KEY (player_id, cycle, slot)"));
check("PK على player_id+cycle+cycle_day للنقاط", sql.includes("PRIMARY KEY (player_id, cycle, cycle_day)"));

// ── guard wiring ────────────────────────────────────────────────────────────
const runPath = path.join(root, "scripts/run_offline_tests.mjs");
const runner = fs.readFileSync(runPath, "utf8");
check("run_offline_tests يشغّل daily_quests_offline_test", runner.includes("daily_quests_offline_test.mjs"));

const pkg = JSON.parse(fs.readFileSync(path.join(root, "package.json"), "utf8"));
check("job test:p8-t6-daily-quests موجود", Boolean(pkg.scripts["test:p8-t6-daily-quests"]));
// البوابة صارت مركّبة (check → check:fast/check:e2e/check:ue-contracts)، فالبحث
// الحرفي في سطر check وحده يبلّغ غياباً وهمياً — chainRuns يوسّع المراجع تعدياً.
check("check chain يشمل test:p8-t6-daily-quests", chainRuns(pkg.scripts, "test:p8-t6-daily-quests"));

// ── doc ─────────────────────────────────────────────────────────────────────
const docPath = path.join(root, "../docs/P8_T6_DAILY_QUESTS.md");
check("وثيقة P8_T6_DAILY_QUESTS.md موجودة", fs.existsSync(docPath));

console.log(`\n${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
console.log("ALL PASSED");
process.exit(0);
