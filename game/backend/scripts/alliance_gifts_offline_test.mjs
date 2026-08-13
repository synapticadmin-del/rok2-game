#!/usr/bin/env node
// P9-T6: حارس جودة offline لصناديق هدايا التحالف — يعيد تنفيذ المنطق النقي محليًا (Node عادي).
// لا يستورد TypeScript؛ يعيد تنفيذ القواعد من alliance_gifts.json مباشرة.
// كل فحص يطبع PASS/FAIL، والنهاية ALL PASSED مع process.exit(0) أو 1.

import { readFileSync, writeFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const specPath = join(__dirname, "..", "src", "data", "alliance_gifts.json");

let failed = 0;
let checks = 0;
function check(label, cond, extra = "") {
  checks += 1;
  if (cond) console.log(`  PASS: ${label}`);
  else { failed += 1; console.log(`  FAIL: ${label} ${extra}`); }
}

const spec = JSON.parse(readFileSync(specPath, "utf8"));
const CONST = spec.constants;

// --- إعادة تنفيذ القواعد محليًا ---
function pickItem(pool, rand) {
  const total = pool.reduce((s, e) => s + Math.max(0, e.weight), 0);
  if (total <= 0) return null;
  let roll = rand() * total;
  for (const e of pool) {
    roll -= Math.max(0, e.weight);
    if (roll <= 0) return e;
  }
  return pool[pool.length - 1];
}
function makeGift(type, hallLevel, memberCount, activeCount, now, rand) {
  const t = spec.gift_types.find((g) => g.id === type);
  if (!t) return { ok: false, reason: "gift_type_unknown" };
  if (hallLevel < t.min_hall_level) return { ok: false, reason: "hall_level_low" };
  if (memberCount < CONST.min_members_for_gift) return { ok: false, reason: "no_members" };
  if (activeCount >= CONST.max_active_gifts_per_alliance) return { ok: false, reason: "max_active_gifts" };
  const item = pickItem(t.pool, rand);
  if (!item) return { ok: false, reason: "empty_pool" };
  return { ok: true, gift: { id: `gift:a:${type}:${now}`, items: [item], createdMs: now, expiresMs: now + CONST.gift_open_window_ms, openedBy: [], maxOpeners: memberCount } };
}
function claimGift(gift, playerId, memberIds, dailyOpens, now) {
  if (now >= gift.expiresMs) return { ok: false, reason: "gift_expired" };
  if (!memberIds.includes(playerId)) return { ok: false, reason: "not_member" };
  if (gift.openedBy.includes(playerId)) return { ok: false, reason: "already_opened" };
  // سقف الفتحات = عدد الأعضاء وقت الإنشاء (maxOpeners) — لا يتسع الصندوق لعدد أكبر من ذلك.
  if (gift.openedBy.length >= gift.maxOpeners) return { ok: false, reason: "gift_full" };
  if (memberIds.length - gift.openedBy.length <= 0) return { ok: false, reason: "gift_full" };
  if (dailyOpens >= CONST.max_daily_opens_per_member) return { ok: false, reason: "daily_cap" };
  const item = gift.items[0];
  const reward = item.kind === "resource" ? { resource: { resource: item.resource, amount: item.amount } }
    : item.kind === "speedup" ? { speedup: { speedup_id: item.speedup_id, amount: item.amount } }
    : { gems: item.amount };
  // تسجيل الفاتح (كما يفعّل الشارد: gift.openedBy = gift.openedBy.concat) ثم إرجاع الحالة المحدثة.
  gift.openedBy = gift.openedBy.concat(playerId);
  return { ok: true, reward, openedBy: gift.openedBy };
}

console.log("P9-T6 alliance gifts offline guard");

// --- 1) صحة ملف البيانات ---
check("ملف alliance_gifts.json موجود", spec && spec.gift_types && spec.gift_types.length > 0);
check(`constants.max_active_gifts_per_alliance = ${CONST.max_active_gifts_per_alliance} > 0`, CONST.max_active_gifts_per_alliance > 0);
check(`gift_open_window_ms = ${CONST.gift_open_window_ms} (24 ساعة)`, CONST.gift_open_window_ms === 24 * 3600 * 1000);
check(`max_daily_opens_per_member = ${CONST.max_daily_opens_per_member} (1/ساعة × 30)`, CONST.max_daily_opens_per_member === 30);
check(`max_opens_per_member_per_gift = ${CONST.max_opens_per_member_per_gift}`, CONST.max_opens_per_member_per_gift === 1);

// --- 2) كل نوع صندوق له pool صالح ---
const validKinds = new Set(["resource", "speedup", "gems"]);
for (const t of spec.gift_types) {
  check(`${t.id}: pool مجموع أوزانه > 0`, t.pool.reduce((s, e) => s + e.weight, 0) > 0, t.id);
  check(`${t.id}: كل العناصر بـ kind معرف`, t.pool.every((e) => validKinds.has(e.kind)), t.id);
  check(`${t.id}: موارد resource من الأربع المعتمدة`, t.pool.filter((e) => e.kind === "resource").every((e) => ["food", "wood", "stone", "gold"].includes(e.resource)), t.id);
  check(`${t.id}: min_hall_level >= 1`, t.min_hall_level >= 1, t.id);
  check(`${t.id}: open_weight > 0`, t.open_weight > 0, t.id);
  check(`${t.id}: source معرف`, typeof t.source === "string" && t.source.length > 0, t.id);
}
check("يوجد نوع victory_gift (صندوق نصر)", spec.gift_types.some((t) => t.id === "victory_gift"));
check("يوجد نوع تبرع (source: tech_donation)", spec.gift_types.some((t) => t.source === "tech_donation"), spec.gift_types.map((t) => t.source).join(","));
check("كل الأنواع بـ min_hall_level محدودة (أدناها <= 10)", Math.min(...spec.gift_types.map((t) => t.min_hall_level)) <= 10);

// --- 3) createGift ---
const now = 1_000_000_000_000;
let seq = 0;
const detRand = () => (seq++ / 100);
let r = makeGift("victory_gift", 10, 5, 0, now, detRand);
check("إنشاء صندوق نصر لمستوى قاعة 10 و5 أعضاء — ناجح", r.ok, JSON.stringify(r));
check("الصندوق ينتهي بعد 24 ساعة بالضبط", r.gift.expiresMs === now + CONST.gift_open_window_ms);
check("الصندوق يبدأ بمفتوحين صفر", r.gift.openedBy.length === 0);
check("الصندوق maxOpeners = عدد الأعضاء", r.gift.maxOpeners === 5);

r = makeGift("victory_gift", 4, 5, 0, now, detRand);
check("مستوى قاعة أقل من الحد الأدنى يُفشل الإنشاء", !r.ok && r.reason === "hall_level_low", JSON.stringify(r));

r = makeGift("unknown_type", 10, 5, 0, now, detRand);
check("نوع غير معروف يُفشل الإنشاء", !r.ok && r.reason === "gift_type_unknown", JSON.stringify(r));

r = makeGift("victory_gift", 10, 0, 0, now, detRand);
check("لا أعضاء (min_members_for_gift=1 مع 0 أعضاء) يُفشل الإنشاء", !r.ok && r.reason === "no_members", JSON.stringify(r));

r = makeGift("victory_gift", 10, 5, CONST.max_active_gifts_per_alliance, now, detRand);
check("سقف الصناديق النشطة يُفشل الإنشاء", !r.ok && r.reason === "max_active_gifts", JSON.stringify(r));

// --- 4) الاختيار المرجح (RNG) — توزيع لا ينحرف نحو نوع واحد فقط ---
const hist = {};
for (let i = 0; i < 600; i++) {
  const res = makeGift("victory_gift", 10, 5, 0, now, () => Math.random());
  const item = res.gift.items[0];
  hist[item.kind] = (hist[item.kind] || 0) + 1;
}
check(`التوزيع المرجح يغطي أكثر من نوع واحد (hist: ${JSON.stringify(hist)})`, Object.keys(hist).length >= 2, JSON.stringify(hist));
check("لا يوجد عنصر بوزن صفري أو سالب", spec.gift_types.every((t) => t.pool.every((e) => e.weight > 0)));

// --- 5) claimGift ---
const gift = makeGift("victory_gift", 10, 5, 0, now, detRand).gift;
const members = ["p1", "p2", "p3", "p4", "p5"];
let c = claimGift(gift, "p1", members, 0, now);
check("العضو الأول يفتح الصندوق بنجاح", c.ok, JSON.stringify(c));
check("المكافأة تحمل المورد مع amount", c.reward && (c.reward.resource || c.reward.speedup || typeof c.reward.gems === "number"), JSON.stringify(c.reward));
// ملاحظة: gift.openedBy يُحدّث داخل claimGift — التكرار يجب أن يُرفض الآن.

c = claimGift(gift, "p1", members, 0, now);
check("نفس العضو لا يفتح مرتين", !c.ok && c.reason === "already_opened", JSON.stringify(c));
check("سجل المفتوحين يتتبع", gift.openedBy.includes("p1"), JSON.stringify(c));

c = claimGift(gift, "outsider", members, 0, now);
check("غير العضو مرفوض", !c.ok && c.reason === "not_member", JSON.stringify(c));

c = claimGift(gift, "p2", members, 0, now + CONST.gift_open_window_ms + 1000);
check("صندوق منتهٍ مرفوض", !c.ok && c.reason === "gift_expired", JSON.stringify(c));

// امتلاء الصندوق (5 أعضاء مفتوحين) — ثم محاولة عضو سادس غير مسجل في الأعضاء
for (const m of ["p2", "p3", "p4", "p5"]) claimGift(gift, m, members, 0, now);
c = claimGift(gift, "p6", ["p1", "p2", "p3", "p4", "p5", "p6"], 0, now);
check("صندوق ممتلئ يُغلق", !c.ok && c.reason === "gift_full", JSON.stringify(c));

// سقف يومي — عضو جديد في صندوق جديد، ثم محاولة بعد تجاوز السقف
const gift2 = makeGift("victory_gift", 10, 5, 0, now, detRand).gift;
c = claimGift(gift2, "p1", members, CONST.max_daily_opens_per_member - 1, now);
check("قبل السقف اليومي يُقبل", c.ok, JSON.stringify(c));
c = claimGift(gift2, "p1", members, CONST.max_daily_opens_per_member - 1, now);
check("تكرار نفس الصندوق مرفوض (already_opened)", !c.ok && c.reason === "already_opened", JSON.stringify(c));
const gift3 = makeGift("victory_gift", 10, 5, 0, now, detRand).gift;
c = claimGift(gift3, "p1", members, CONST.max_daily_opens_per_member, now);
check("بعد السقف اليومي يُرفض", !c.ok && c.reason === "daily_cap", JSON.stringify(c));

// --- 6) slotsRemaining ---
const gift4 = makeGift("victory_gift", 10, 5, 0, now, detRand).gift;
check("slotsRemaining عند البدء = عدد الأعضاء", members.length - gift4.openedBy.length === 5);
claimGift(gift4, "p1", members, 0, now);
claimGift(gift4, "p2", members, 0, now);
check("slotsRemaining ينقص مع كل فتحة", members.length - gift4.openedBy.length === 3);

// --- 7) أنواع المكافآت الصالحة فقط ---
const validResources = new Set(["food", "wood", "stone", "gold"]);
for (let i = 0; i < 200; i++) {
  const res = makeGift("victory_gift", 10, 5, 0, now, () => Math.random());
  const item = res.gift.items[0];
  if (item.kind === "resource") check(`مورد صالح ${item.resource} + amount>0`, validResources.has(item.resource) && item.amount > 0, item.resource);
  if (item.kind === "gems") check(`جواهر amount>0`, item.amount > 0);
  if (item.kind === "speedup") check(`تسريع amount>0`, item.amount > 0);
}

console.log(`\nchecks: ${checks}, failed: ${failed}`);
if (failed === 0) console.log("ALL PASSED");
process.exit(failed === 0 ? 0 : 1);
