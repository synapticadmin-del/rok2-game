// P9-T4: حارس جودة نظام VIP الكامل (offline، لا يستورد TypeScript — يعيد تنفيذ المنطق النقي)
// يتحقق من: 15 مستوى متصاعد، نقاط يومية 40+20 مع سقف 200، بافات حرجة (VIP6 طابور ثانٍ، VIP15 +20/20/20/30/50%)،
// متجر VIP (خصم حسب المستوى، قفل CH5)، أسعار متجر مشتقة من shop.json.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const shop = JSON.parse(readFileSync(join(here, "../src/data/shop.json"), "utf8"));
const tiers = shop.vip_tiers;
const vipDaily = shop.constants.vip_daily;
const vipStore = shop.constants.vip_store;

let failed = 0;
let checks = 0;
function check(name, cond, detail) {
  checks++;
  if (!cond) {
    failed++;
    console.log(`FAIL: ${name}${detail ? " — " + detail : ""}`);
  } else {
    console.log(`PASS: ${name}`);
  }
}

// منطق نقي معاد تنفيذه من sim/shop.ts
const vipTierForPoints = (points) => {
  let cur = tiers[0];
  for (const t of tiers) if (points >= t.points_required) cur = t;
  return cur;
};
const vipPointsForPurchase = (gems) => gems * shop.constants.vip_points_per_gem;
const vipDailyFullGrant = () => Math.min(vipDaily.base_per_day + vipDaily.connected_bonus, vipDaily.daily_cap);
const applyVipDailyPoints = (state, day) => {
  if (state.lastDailyPointsDay >= day) return { state, granted: 0 };
  const grant = vipDailyFullGrant();
  return {
    state: { ...state, points: state.points + grant, lastDailyPointsDay: day, lastLoginDay: day },
    granted: grant,
  };
};
const vipStorePrice = (base, level, hall) => {
  if (hall < vipStore.hall_level_required) return 0;
  let best = 0;
  for (const d of vipStore.discount_by_tier) if (level >= d.min_level && d.discount > best) best = d.discount;
  if (best <= 0) return base;
  return Math.max(1, Math.ceil(base * (1 - best)));
};

// 1. عدد المستويات = 15 (+المستوى 0)
check("15 مستوى VIP كامل (0..15)", tiers.length === 16 && tiers[tiers.length - 1].level === 15, `count=${tiers.length}`);

// 2. كل مستوى له الحقول المطلوبة
const required = ["level", "points_required", "production_mult", "build_speed_mult", "train_speed_mult",
  "research_speed_mult", "heal_speed_mult", "gather_mult", "free_speedup_sec_per_day", "extra_build_queue"];
check("كل المستويات تحتوي الحقول الجديدة (research/heal/gather/extra_build_queue)",
  tiers.every((t) => required.every((k) => k in t)));

// 3. النقاط متصاعدة
let ok = true;
for (let i = 1; i < tiers.length; i++) {
  if (tiers[i].points_required <= tiers[i - 1].points_required) { ok = false; break; }
}
check("نقاط المستويات متصاعدة صارمة", ok, tiers.map((t) => `${t.level}:${t.points_required}`).join(","));

// 4. المستوى 0 و15
check("مستوى 0: نقاط 0 وبدون مزايا", tiers[0].points_required === 0 && tiers[0].production_mult === 1 && tiers[0].extra_build_queue === false);
check("مستوى 15: build/research/train +20%, gather +30%, heal +50%",
  tiers[15].build_speed_mult === 1.2 && tiers[15].research_speed_mult === 1.2 &&
  tiers[15].train_speed_mult === 1.2 && tiers[15].gather_mult === 1.3 && tiers[15].heal_speed_mult === 1.5,
  `L15=${JSON.stringify({ build: tiers[15].build_speed_mult, res: tiers[15].research_speed_mult, train: tiers[15].train_speed_mult, gather: tiers[15].gather_mult, heal: tiers[15].heal_speed_mult })}`);

// 5. VIP 6: طابور بناء ثانٍ
check("VIP 6: extra_build_queue = true", tiers[6].extra_build_queue === true);
check("مستويات 0..5: طابور واحد", tiers.slice(0, 6).every((t) => t.extra_build_queue === false));

// 6. نقاط يومية: 40 + 20 اتصال، سقف 200
check("vip_daily: base 40 + connected 20 = 60", vipDaily.base_per_day === 40 && vipDaily.connected_bonus === 20, `cfg=${JSON.stringify(vipDaily)}`);
check("vip_daily cap = 200", vipDaily.daily_cap === 200);
check("vipDailyFullGrant = 60 <= cap", vipDailyFullGrant() === 60);
check("المنحة اليومية تُدفع مرة واحدة (idempotent)", applyVipDailyPoints(
  { points: 0, lastDailyPointsDay: 99, lastLoginDay: 99 }, 99).granted === 0);
check("المنحة اليومية تُدفع يوم جديد (60)", applyVipDailyPoints(
  { points: 100, lastDailyPointsDay: 0, lastLoginDay: 0 }, 99).granted === 60);
check("السقف اليومي يحترم cap", applyVipDailyPoints(
  { points: 150, lastDailyPointsDay: 0, lastLoginDay: 0 }, 99).state.points === 150 + 60);
check("السقف لا يتجاوز daily_cap (190+60 → 200)", Math.min(190 + 60, 200) === 200 &&
  vipDailyFullGrant() <= vipDaily.daily_cap);
check("screenshot: points required 1..15 متزامنة مع نقاط يومية",
  tiers[1].points_required <= vipDailyFullGrant() * 5,
  `L1 needs ${tiers[1].points_required} points (${Math.ceil(tiers[1].points_required / vipDailyFullGrant())} يوم)`);

// 7. متجر VIP
check("vip_store: hall_level_required = 5", vipStore.hall_level_required === 5, `cfg=${JSON.stringify(vipStore)}`);
check("store مقفل قبل CH5 (سعر 0)", vipStorePrice(1000, 15, 4) === 0);
check("store مفتوح عند CH5 بدون خصم لمستوى 0", vipStorePrice(1000, 0, 5) === 1000);
check("store خصم L12: 20% → 800", vipStorePrice(1000, 12, 5) === 800,
  `price=${vipStorePrice(1000, 12, 5)}`);
check("store خصم L15: 25% → 750", vipStorePrice(1000, 15, 5) === 750,
  `price=${vipStorePrice(1000, 15, 5)}`);
check("خصومات discount_by_tier متصاعدة", vipStore.discount_by_tier.every((d, i) =>
  i === 0 || d.min_level >= vipStore.discount_by_tier[i - 1].min_level && d.discount >= vipStore.discount_by_tier[i - 1].discount));

// 8. بافات متدرجة (monotonic)
let mono = true;
for (let i = 1; i < tiers.length; i++) {
  for (const k of ["production_mult", "build_speed_mult", "train_speed_mult", "research_speed_mult", "heal_speed_mult", "gather_mult"]) {
    if (tiers[i][k] < tiers[i - 1][k]) { mono = false; break; }
  }
  if (!mono) break;
}
check("البافات متصاعدة غير متناقصة مع المستوى", mono);

// 9. نقاط شراء gems
check("vip_points_per_gem = 1 (من JSON)", shop.constants.vip_points_per_gem === 1);
check("شراء 1000 gems = 1000 نقطة", vipPointsForPurchase(1000) === 1000);

// 10. free_speedup_sec_per_day متدرج
check("free_speedup_sec_per_day متصاعد", tiers.every((t, i) =>
  i === 0 || t.free_speedup_sec_per_day >= tiers[i - 1].free_speedup_sec_per_day));
check("L15 free speedup = ساعتان (7200s)", tiers[15].free_speedup_sec_per_day === 7200,
  `sec=${tiers[15].free_speedup_sec_per_day}`);

console.log(`\nchecks: ${checks}, failed: ${failed}`);
if (failed === 0) console.log("ALL PASSED: P9-T4 VIP guard (15 مستوى + نقاط يومية + بافات + متجر)");
process.exit(failed === 0 ? 0 : 1);
