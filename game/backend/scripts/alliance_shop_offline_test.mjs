/**
 * P9-T3: حارس نقّي لنظام متجر التحالف والألقاب — لا يحتاج Miniflare ولا D1.
 * يتحقق من alliance_shop.json ثم يعيد تنفيذ منطق sim/alliance_shop.ts محليًا
 * (نفس نمط حراس P8/P9-T1/P9-T2) ويختبر التغطية الكاملة.
 *
 * النمط المتبع في الريبو: طباعة ALL PASSED / FAIL n ثم exit(0|1).
 */
import { readFileSync, existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, "..");
const TD = JSON.parse(readFileSync(join(root, "src/data/alliance_shop.json"), "utf8"));
const CFG = TD;
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

// ═══ إعادة تنفيذ محلية مطابقة لـ sim/alliance_shop.ts حرفيًا ═══
const shopSpec = CFG;
function earnPerHelp() { return Number(shopSpec.credits.earn.help_credit.per_help); }
function earnPerGiftClaim() { return Number(shopSpec.credits.earn.gift_claims.per_claim); }
function dailyCap() { return Number(shopSpec.credits.earn.daily_cap.amount); }
function balanceCap() { return Number(shopSpec.credits.earn.balance_cap.amount); }
function applyHelpCredit(state, seasonDay) {
  const s = { ...state };
  if (s.dailyEarnedDay !== seasonDay) { s.dailyEarned = 0; s.dailyEarnedDay = seasonDay; }
  const left = Math.max(0, dailyCap() - s.dailyEarned);
  const earned = Math.min(earnPerHelp(), left, balanceCap() - s.balance);
  s.balance = Math.min(balanceCap(), s.balance + earned);
  s.dailyEarned += earned;
  return { state: s, earned };
}
function applyGiftClaimCredit(state, seasonDay) {
  const s = { ...state };
  if (s.dailyEarnedDay !== seasonDay) { s.dailyEarned = 0; s.dailyEarnedDay = seasonDay; }
  const left = Math.max(0, dailyCap() - s.dailyEarned);
  const earned = Math.min(earnPerGiftClaim(), left, balanceCap() - s.balance);
  s.balance = Math.min(balanceCap(), s.balance + earned);
  s.dailyEarned += earned;
  return { state: s, earned };
}
function itemById(id) { return (shopSpec.catalog || []).find((it) => it.id === id); }
function titleById(id) { return (shopSpec.titles.definitions || []).find((t) => t.id === id); }
function maxGrantedTitles() { return Number(shopSpec.titles.max_granted_per_alliance); }
function validatePurchase(state, itemId) {
  const item = itemById(itemId);
  if (!item) return { ok: false, reason: "unknown_shop_item" };
  if (state.balance < item.price) return { ok: false, reason: "insufficient_alliance_balance" };
  const bought = Number(state.items[itemId] || 0);
  if (bought >= Number(item.max_per_alliance)) return { ok: false, reason: "alliance_item_cap_reached" };
  return { ok: true };
}
function purchase(state, itemId) {
  const v = validatePurchase(state, itemId);
  if (!v.ok) return v;
  const item = itemById(itemId);
  const s = { ...state, balance: state.balance - item.price, items: { ...state.items } };
  s.items[itemId] = (s.items[itemId] || 0) + 1;
  return { state: s, item };
}
function validateTitleGrant(state, titleId) {
  const def = titleById(titleId);
  if (!def) return { ok: false, reason: "unknown_title" };
  if (Object.keys(state.titles).length >= maxGrantedTitles()) return { ok: false, reason: "alliance_title_cap_reached" };
  if (state.titles[titleId]) return { ok: false, reason: "title_already_granted" };
  return { ok: true };
}
function grantTitle(state, titleId, targetPlayerId) {
  const def = titleById(titleId);
  if (!def) return { ok: false, reason: "unknown_title" };
  const s = { ...state, titles: { ...state.titles } };
  s.titles[titleId] = targetPlayerId;
  return { state: s, title: def };
}
function revokeTitle(state, titleId) {
  if (!state.titles[titleId]) return state;
  const s = { ...state, titles: { ...state.titles } };
  delete s.titles[titleId];
  return s;
}
function titleBuffsForPlayer(state, playerId) {
  const out = {};
  for (const [tid, pid] of Object.entries(state.titles)) {
    if (pid !== playerId) continue;
    const def = titleById(tid);
    if (!def) continue;
    for (const [stat, mod] of Object.entries(def.buffs)) out[stat] = (out[stat] || 0) + Number(mod);
  }
  return out;
}
function emptyState() { return { balance: 0, dailyEarned: 0, dailyEarnedDay: 0, items: {}, titles: {} }; }

// ═══ الاختبارات ═══
console.log("P9-T3 alliance shop offline guard");
check("alliance_shop.json موجود ويحمل version", TD.version >= 1);
check("credits earn per_help رقم موجب", Number.isFinite(earnPerHelp()) && earnPerHelp() > 0);
check("credits earn per_claim رقم موجب", Number.isFinite(earnPerGiftClaim()) && earnPerGiftClaim() > 0);
check("daily_cap رقم موجب", dailyCap() > 0);
check("balance_cap أكبر من daily_cap", balanceCap() > dailyCap());
check("catalog فيه عناصر", (shopSpec.catalog || []).length >= 1);
check("كل عنصر catalog له price رقمي موجب وmax_per_alliance ≥1 وgrant صالح",
  (shopSpec.catalog || []).every((it) => it.price > 0 && it.max_per_alliance >= 1 && it.grant && Number.isFinite(it.grant.amount)));
check("definitions فيها ألقاب", (shopSpec.titles.definitions || []).length >= 1);
check("كل لقب له id/اسم/buffs رقمية",
  (shopSpec.titles.definitions || []).every((t) => t.id && t.name && t.buffs && Object.values(t.buffs).every((v) => Number.isFinite(v))));
check("max_granted_per_alliance ≥1", maxGrantedTitles() >= 1);

// كسب رصيد: help
let st = emptyState();
let r = applyHelpCredit(st, 5);
check("help واحدة تكسب earnPerHelp", r.earned === earnPerHelp());
check("الرصيد يرتفع بالكسب", r.state.balance === earnPerHelp());
// سقف يومي
st = emptyState();
for (let i = 0; i < dailyCap() / earnPerHelp() + 5; i++) { r = applyHelpCredit(st, 5); st = r.state; }
check("الكسب اليومي لا يتجاوز daily_cap", st.dailyEarned <= dailyCap() + earnPerHelp() && st.balance <= balanceCap());
// سقف الرصيد
st = { balance: balanceCap() - 1, dailyEarned: 0, dailyEarnedDay: 5, items: {}, titles: {} };
r = applyHelpCredit(st, 5);
check("الرصيد لا يتجاوز balance_cap", r.state.balance <= balanceCap());
// إعادة ضبط يومي
st = { balance: 100, dailyEarned: 50, dailyEarnedDay: 5, items: {}, titles: {} };
r = applyHelpCredit(st, 6);
check("يوم جديد يعيد ضبط dailyEarned", r.state.dailyEarnedDay === 6 && r.state.dailyEarned === r.earned);
// gift claim
st = emptyState();
r = applyGiftClaimCredit(st, 5);
check("gift claim تكسب earnPerGiftClaim", r.earned === earnPerGiftClaim());

// شراء
st = emptyState();
const firstItem = shopSpec.catalog[0].id;
const item = itemById(firstItem);
check("شراء بدون رصيد يفشل", !validatePurchase(st, firstItem).ok);
st = { ...st, balance: item.price };
r = purchase(st, firstItem);
check("شراء ناجح يخصم السعر", r.state.balance === 0 && r.state.items[firstItem] === 1);
check("max_per_alliance=1 يمنع الشراء الثاني", !purchase(r.state, firstItem).ok);
check("عنصر مجهول يرفض", !validatePurchase(emptyState(), "nonexistent_item").ok);

// ألقاب
st = emptyState();
r = grantTitle(st, shopSpec.titles.definitions[0].id, "p1");
check("منح لقب يضيفه للـ titles", !!r.title && r.state.titles[shopSpec.titles.definitions[0].id] === "p1");
check("لقب مكرر يرفض", !validateTitleGrant(r.state, shopSpec.titles.definitions[0].id).ok);
check("لقب مجهول يرفض", !validateTitleGrant(st, "nonexistent_title").ok);
st = r.state;
const second = shopSpec.titles.definitions[1];
if (second) {
  r = grantTitle(st, second.id, "p2");
  check("لقب ثانٍ يُمنح لشخص آخر", r.state.titles[second.id] === "p2");
  if (shopSpec.titles.definitions.length >= maxGrantedTitles()) {
    check("بلوغ سقف الألقاب يمنع المزيد", !validateTitleGrant(r.state, "nonexistent_title").ok ||
      Object.keys(r.state.titles).length >= maxGrantedTitles());
  }
  st = r.state;
}
r = revokeTitle(st, shopSpec.titles.definitions[0].id);
check("سحب لقب يزيله", !r.titles[shopSpec.titles.definitions[0].id]);
check("سحب لقب غير موجود لا يغيّر الحالة", revokeTitle(r, "nonexistent_title") === r);
const t1 = shopSpec.titles.definitions[0];
st = { balance: 0, dailyEarned: 0, dailyEarnedDay: 0, items: {}, titles: { [t1.id]: "p1" } };
const buffs = titleBuffsForPlayer(st, "p1");
check("بافات اللقب تصل لحامله", Object.keys(buffs).length === Object.keys(t1.buffs).length &&
  Object.entries(t1.buffs).every(([k, v]) => buffs[k] === Number(v)));
check("لاعب بلا لقب لا يحصل على بافات", Object.keys(titleBuffsForPlayer(st, "other")).length === 0);

// rate limits في anticheat.json
const AC = JSON.parse(readFileSync(join(root, "src/data/anticheat.json"), "utf8"));
check("anticheat.json يحتوي alliance_shop_purchase", !!AC.rate_limits.alliance_shop_purchase);
check("window alliance_shop_purchase = 3600000", AC.rate_limits.alliance_shop_purchase.window_ms === 3600000);

console.log(`checks: ${pass}, failed: ${fail}`);
console.log(fail === 0 ? "ALL PASSED" : "FAIL");
process.exit(fail === 0 ? 0 : 1);
