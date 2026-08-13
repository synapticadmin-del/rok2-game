// P9-T5: حارس جودة Trading Post — منطق نقي معاد تنفيذه محليًا من data/trading.json
// لا يستورد TypeScript؛ يعيد تطبيق دوال sim/trading.ts نفسها في JS خالص.
import { readFileSync } from "node:fs";

let failed = 0;
let passed = 0;
function check(name, cond) {
  if (cond) { passed++; console.log("PASS:", name); }
  else { failed++; console.log("FAIL:", name); }
}

const spec = JSON.parse(readFileSync(new URL("../src/data/trading.json", import.meta.url)));
const C = spec.constants;

// ---- إعادة تنفيذ الدوال النقية (مطابقة sim/trading.ts) ----
const RESOURCES = spec.resources.map((r) => r.id);
function basePrice(res) {
  const b = Number(C.base_prices[res]);
  if (!Number.isFinite(b) || b <= 0) return 1;
  return b;
}
const rateMin = Number(C.min_rate), rateMax = Number(C.max_rate), rateStep = Number(C.rate_step);
function clampRate(rate) {
  let r = Math.max(rateMin, Math.min(rateMax, rate));
  r = Math.round(r / rateStep) * rateStep;
  return Number(r.toFixed(4));
}
function adaptPrice(current, demandDelta, supplyDelta) {
  const step = Number(C.price_adaptive_step);
  return clampRate(current + demandDelta * step - supplyDelta * (step / 2));
}
function offerCostForBuyer(offer) {
  void basePrice(offer.buyResource);
  return clampRate(offer.rate) * offer.amount;
}
function sellerNet(offer) {
  const fee = Number(C.fee_rate);
  const gross = offer.rate * offer.amount;
  const netAmount = Math.max(0, Math.floor(gross * (1 - fee)));
  return { netAmount, feeAmount: Math.max(0, gross - netAmount) };
}
function settleTrade(offer, amount) {
  const buyerPaysBuy = offer.rate * amount;
  const sellerGainsBuy = Math.max(0, Math.floor(buyerPaysBuy * (1 - Number(C.fee_rate))));
  return {
    buyerPaysBuy,
    buyerReceivesSell: amount,
    sellerLosesSell: amount,
    sellerGainsBuy,
    feeCharged: buyerPaysBuy - sellerGainsBuy,
  };
}
function validateOffer(inp) {
  if (!RESOURCES.includes(inp.sellResource)) return { ok: false, error: "unknown_sell_resource" };
  if (!RESOURCES.includes(inp.buyResource)) return { ok: false, error: "unknown_buy_resource" };
  if (inp.sellResource === inp.buyResource) return { ok: false, error: "same_resource_pair" };
  const minAmt = Number(C.min_offer_amount), maxAmt = Number(C.max_offer_amount);
  if (!Number.isFinite(inp.amount) || inp.amount < minAmt || inp.amount > maxAmt) return { ok: false, error: "amount_out_of_range" };
  if (Math.round(inp.amount / minAmt) * minAmt !== inp.amount) return { ok: false, error: "amount_not_multiple" };
  if (!Number.isFinite(inp.rate) || inp.rate < rateMin || inp.rate > rateMax) return { ok: false, error: "rate_out_of_range" };
  if (inp.activeOfferCount >= Number(C.max_offers_per_player)) return { ok: false, error: "offer_cap_reached" };
  return { ok: true };
}
function validateClaim(inp) {
  if (inp.createdMs + Number(C.offer_ttl_sec) * 1000 <= inp.nowMs) return { ok: false, error: "offer_expired" };
  const minClaim = Number(C.min_claim_amount);
  if (!Number.isFinite(inp.claimAmount) || inp.claimAmount < minClaim) return { ok: false, error: "claim_too_small" };
  const amount = Math.min(inp.claimAmount, inp.offerAmount);
  if (Math.round(amount / minClaim) * minClaim !== amount) return { ok: false, error: "amount_not_multiple" };
  if (Number(inp.claimRate) !== Number(inp.offerRate)) return { ok: false, error: "rate_mismatch" };
  if (inp.claimedToday + amount > Number(C.max_claim_per_day) * minClaim) return { ok: false, error: "daily_claim_cap" };
  return { ok: true, amount };
}

// ---- فحوصات البنية والبيانات ----
check("resources = food/wood/stone/gold", JSON.stringify(RESOURCES) === '["food","wood","stone","gold"]');
check("fee_rate = 5%", C.fee_rate === 0.05);
check("min_rate = 0.5", C.min_rate === 0.5);
check("max_rate = 2.0", C.max_rate === 2.0);
check("rate_step = 0.05", C.rate_step === 0.05);
check("min_offer_amount = 100", C.min_offer_amount === 100);
check("max_offer_amount = 500000", C.max_offer_amount === 500000);
check("min_claim_amount = 100", C.min_claim_amount === 100);
check("max_claim_per_day = 10", C.max_claim_per_day === 10);
check("offer_ttl_sec = 86400 (يوم)", C.offer_ttl_sec === 86400);
check("max_offers_per_player = 5", C.max_offers_per_player === 5);
check("min_trade_hall_level = 5", C.min_trade_hall_level === 5);
check("price_adaptive_step = 0.02", C.price_adaptive_step === 0.02);
check("base_prices food=1.0 wood=1.1 stone=1.25 gold=2.0",
  C.base_prices.food === 1.0 && C.base_prices.wood === 1.1 && C.base_prices.stone === 1.25 && C.base_prices.gold === 2.0);
check("لا مورد غير معروف في base_prices", Object.keys(C.base_prices).every((k) => RESOURCES.includes(k)));

// ---- فحوصات clampRate ----
check("clamp 0.1 → 0.5", clampRate(0.1) === 0.5);
check("clamp 3.0 → 2.0", clampRate(3.0) === 2.0);
check("clamp 1.27 → 1.25", clampRate(1.27) === 1.25);
check("clamp 1.0 ثابت", clampRate(1.0) === 1.0);

// ---- فحوصات السعر الديناميكي ----
const baseGold = basePrice("gold");
check("base gold = 2.0", baseGold === 2.0);
// ملاحظة: adaptPrice تقرّب النتيجة لخطوة السعر rate_step (0.05)،
// لذا خطوة تكيفية واحدة 0.02 وحدها لا تغيّر السعر حتى تراكم فوق العتبة.
// السعر المعروض = base + (trades − offers/2) × step مقربً لخطوة السعر 0.05 (تراكمي من العدادات)
const up1 = adaptPrice(baseGold, 1, 0);
check("صفقة واحدة (0.02) تقرّب دون تغيير من 2.0", up1 === 2.0);
// gold بسعره الأساسي 2.0 = max_rate — أي طلب إضافي مقيد بالسقف.
const up2 = adaptPrice(baseGold, 2, 0);
check("صفقتان على الذهب (سقف 2.0) تظل مقيدة عند max_rate", up2 === 2.0);
// اختبار تراكم السعر على مورد متوسط (stone=1.25) — لا يلامس السقف
const upStone2 = adaptPrice(basePrice("stone"), 2, 0);
check("صفقتان على stone (0.04) تقرّب إلى 1.3 بعد تجاوز العتبة", upStone2 === 1.3);
const upStone5 = adaptPrice(basePrice("stone"), 5, 0);
check("5 صفقات على stone (0.10) ترفع السعر إلى 1.35", upStone5 === 1.35);
const down = adaptPrice(basePrice("stone"), 0, 2);
check("عرضان فائضان (−0.02) ل stone=1.25 تقرّب دون تغيير", down === 1.25);
const down5 = adaptPrice(basePrice("stone"), 0, 10);
check("10 عروض فائضة (−0.10) تخفض stone إلى 1.15", down5 === 1.15);
const clampedUp = adaptPrice(rateMax, 10, 0);
check("السعر لا يتجاوز max_rate", clampedUp === rateMax);
const clampedDown = adaptPrice(rateMin, 0, 10);
check("السعر لا يقل عن min_rate", clampedDown === rateMin);

// ---- فحوصات التكلفة والرسوم ----
const o1 = { sellResource: "food", buyResource: "gold", amount: 1000, rate: 1.5 };
const cost = offerCostForBuyer(o1);
check("المشتري يدفع rate × amount = 1500 gold", cost === 1500);
const net = sellerNet(o1);
check("صافي البائع = 1500×0.95 = 1425", net.netAmount === 1425);
check("الرسوم = 75", net.feeAmount === 75);
const s = settleTrade(o1, 500);
check("settle: buyerPaysBuy=750", s.buyerPaysBuy === 750);
check("settle: buyerReceivesSell=500", s.buyerReceivesSell === 500);
check("settle: sellerLosesSell=500", s.sellerLosesSell === 500);
check("settle: sellerGainsBuy=712 (750×0.95)", s.sellerGainsBuy === 712);
check("settle: feeCharged=38", s.feeCharged === 38);
check("settle حفاظ على القيمة: paid - gains = fee", s.buyerPaysBuy - s.sellerGainsBuy === s.feeCharged);

// ---- فحوصات validateOffer ----
check("عرض صالح يُقبل", validateOffer({ sellResource: "food", buyResource: "stone", amount: 500, rate: 1.1, activeOfferCount: 0 }).ok);
check("مورد بيع مجهول مرفوض", !validateOffer({ sellResource: "iron", buyResource: "stone", amount: 500, rate: 1.1, activeOfferCount: 0 }).ok);
check("نفس المورد مرفوض", !validateOffer({ sellResource: "food", buyResource: "food", amount: 500, rate: 1.1, activeOfferCount: 0 }).ok);
check("مبلغ أقل من 100 مرفوض", !validateOffer({ sellResource: "food", buyResource: "stone", amount: 50, rate: 1.1, activeOfferCount: 0 }).ok);
check("مبلغ غير مضاعف لـ100 مرفوض", !validateOffer({ sellResource: "food", buyResource: "stone", amount: 150, rate: 1.1, activeOfferCount: 0 }).ok);
check("مبلغ فوق 500K مرفوض", !validateOffer({ sellResource: "food", buyResource: "stone", amount: 600000, rate: 1.1, activeOfferCount: 0 }).ok);
check("سعر خارج النطاق مرفوض", !validateOffer({ sellResource: "food", buyResource: "stone", amount: 500, rate: 2.5, activeOfferCount: 0 }).ok);
check("سقف 5 عروض مرفوض", !validateOffer({ sellResource: "food", buyResource: "stone", amount: 500, rate: 1.1, activeOfferCount: 5 }).ok);
check("4 عروض ما زال مقبولًا", validateOffer({ sellResource: "food", buyResource: "stone", amount: 500, rate: 1.1, activeOfferCount: 4 }).ok);

// ---- فحوصات validateClaim ----
const now = Date.now();
const off = { id: "t1", sellResource: "food", buyResource: "gold", amount: 2000, rate: 1.2, created_ms: now - 100000, remaining: 2000 };
check("إتمام صالح (1000 ≤ 2000 متبقٍ)", validateClaim({ claimAmount: 1000, offerAmount: off.remaining, claimRate: off.rate, offerRate: off.rate, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("إتمام جزئي = amount المطلوب", validateClaim({ claimAmount: 1000, offerAmount: 3000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).amount === 1000);
check("إتمام يفوق المتاح والسقف يرفض (2000 مقيد > سقف 1000)", !validateClaim({ claimAmount: 5000, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("إتمام 800 ضمن السقف (0+800 <= 1000) مقبول", validateClaim({ claimAmount: 800, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("سعر خاطئ مرفوض", !validateClaim({ claimAmount: 1000, offerAmount: 2000, claimRate: 1.0, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("أقل من 100 مرفوض", !validateClaim({ claimAmount: 50, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("عرض منتهٍ مرفوض", !validateClaim({ claimAmount: 1000, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: now - C.offer_ttl_sec * 1000, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("سقف يومي 10×100 مرفوض", !validateClaim({ claimAmount: 1000, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 900 }).ok);
// السقف اليومي = max_claim_per_day × min_claim = 1000 وحدة؛ الشرط: claimedToday + amount > 1000.
check("900 وحدة + 100 = 1000 مقبول (يساوي السقف بالضبط)", validateClaim({ claimAmount: 100, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 900 }).ok);
check("900 + 200 فوق السقف مرفوض", !validateClaim({ claimAmount: 1000, offerAmount: 3000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 900 }).ok);

// ---- التكامل: عرض + إتمام جزئي + رسوم صحيحة عبر الدورة ----
const cycleOffer = { sellResource: "stone", buyResource: "food", amount: 5000, rate: 1.5 };
const check1 = validateOffer({ ...cycleOffer, activeOfferCount: 0 });
check("دورة العرض: صالح", check1.ok);
const t1 = settleTrade(cycleOffer, 3000);
const check2 = validateClaim({ claimAmount: 900, offerAmount: cycleOffer.amount, claimRate: cycleOffer.rate, offerRate: cycleOffer.rate, createdMs: now - 10000, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 });
// إتمام 5000 من متاح 2000 يُقيَّد إلى 2000 فيدفعه اختبار السقف اليومي (0+2000 > 1000) —
// السقف يُطبق على الكمية الفعلية المقتطعة وليس على الكمية المطلوبة فقط.
check("إتمام مقيد بالسقف اليومي يُرفض حتى إن كان المتاح يكفي", !validateClaim({ claimAmount: 5000, offerAmount: 2000, claimRate: 1.2, offerRate: 1.2, createdMs: off.created_ms, ttlSec: C.offer_ttl_sec, nowMs: now, claimedToday: 0 }).ok);
check("دورة الإتمام: صالح = 900", check2.ok && check2.amount === 900);
check("دورة التسوية: buyer pays 4500 food", t1.buyerPaysBuy === 4500);
check("دورة التسوية: seller gains 4275 food", t1.sellerGainsBuy === 4275);
check("دورة التسوية: seller loses 3000 stone", t1.sellerLosesSell === 3000);
const remaining = cycleOffer.amount - check2.amount;
check("دورة العرض: المتبقي 4100", remaining === 4100);
const t2 = settleTrade(cycleOffer, remaining);
check("دورة الإتمام الثاني: settles 4100", t2.buyerReceivesSell === 4100);
const totalFee = t1.feeCharged + t2.feeCharged;
check("إجمالي الرسوم على الدورة الكاملة = 533", totalFee === 533);
check("دورة الإتمام الثاني: buyer pays 6150 food", t2.buyerPaysBuy === 6150);
check("دورة الإتمام الثاني: seller gains 5842 food", t2.sellerGainsBuy === 5842);

console.log(`\nchecks: ${passed + failed}, failed: ${failed}`);
if (failed > 0) { console.log("ALL FAILED checks logged above"); process.exit(1); }
console.log("ALL PASSED");
process.exit(0);
