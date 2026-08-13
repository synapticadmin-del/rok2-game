// P9-T5: منطق Trading Post النقي — سعر صرف ديناميكي (عرض/طلب) + رسوم + حدود يومية.
// كل القيم تُقرأ من data/trading.json عبر getTrading() — لا ثوابت هنا.
import { getTrading } from "../../lib/gameData";

export type TradingResource = { id: string; name: string; icon: string };
export type TradingMarketPrice = { resource: string; price: number; day: number; updated_ms: number };
export type TradingOffer = {
  id: string;
  sellerId: string;
  sellResource: string;
  buyResource: string;
  amount: number;
  rate: number;
  created_ms: number;
  remaining?: number; // الكمية المتبقية غير المشتراة بعد
};

export function tradingConstants() {
  return getTrading().constants;
}

export function tradingResources(): string[] {
  return getTrading().resources.map((r) => r.id);
}

export function isValidTradingResource(id: string): boolean {
  return tradingResources().includes(id);
}

export function resourceBasePrice(resource: string): number {
  const c = tradingConstants();
  const base = Number((c.base_prices as Record<string, number>)[resource]);
  if (!Number.isFinite(base) || base <= 0) return 1;
  return base;
}

/** P9-T5: السعر الأولي للمورد عند بدء الموسم (من base_prices في JSON) */
export function initialPriceFor(resource: string): number {
  return clampRate(resourceBasePrice(resource));
}

/** P9-T5: نطاق السعر المسموح به من JSON (min_rate..max_rate) */
export function rateBounds() {
  const c = tradingConstants();
  return { min: Number(c.min_rate), max: Number(c.max_rate), step: Number(c.rate_step) };
}

/** P9-T5: تقييد سعر صرف ضمن النطاق مع التقريب للخطوة */
export function clampRate(rate: number): number {
  const b = rateBounds();
  let r = Math.max(b.min, Math.min(b.max, rate));
  r = Math.round(r / b.step) * b.step;
  return Number(r.toFixed(4));
}

/** P9-T5: السعر الفعلي الذي يدفعه المشتري = rate × base(المورد المطلوب شراؤه بالمقابل).
 *  التفسير السلطوي: العرض يبيع sellResource ويطلب buyResource بسعر صرف rate —
 *  يعني المشتري يدفع (rate × amount) وحدات buyResource مقابل amount وحدة sellResource. */
export function offerCostForBuyer(offer: TradingOffer): number {
  const b = resourceBasePrice(offer.buyResource);
  return clampRate(rateBounds().step && Number(offer.rate) > 0 ? Number(offer.rate) : 1) * offer.amount;
}

/** P9-T5: صافي ما يحصل عليه البائع بعد الرسوم — البائع يقايض amount من sellResource
 *  ويحصل على (rate × amount × (1 − fee)) من buyResource (تُقرَّب لأدنى عدد صحيح ≥ 0). */
export function sellerNet(offer: TradingOffer): { netAmount: number; feeAmount: number } {
  const c = tradingConstants();
  const fee = Number(c.fee_rate);
  const gross = offer.rate * offer.amount;
  const netAmount = Math.max(0, Math.floor(gross * (1 - fee)));
  return { netAmount, feeAmount: Math.max(0, gross - netAmount) };
}

/** P9-T5: تحديث السعر الديناميكي للمورد بعد صفقة — الطلب على مورد يرفع سعره
 *  (كل صفقة شراء تضيف adaptive_step)، وحجم العرض الفائض يخفضه. البائع يبيع موردًا
 *  فيرغب به السوق (طلب) فنرفع سعر المورد المشتَرى به؛ والعرض الزائد على المورد المباع
 *  (كل عرض يُفتح دون إتمام يضيف نصف خطوة تخفيض). يظل السعر داخل النطاق. */
export function adaptPrice(current: number, demandDelta: number, supplyDelta: number): number {
  const c = tradingConstants();
  const step = Number(c.price_adaptive_step);
  let price = current + demandDelta * step - supplyDelta * (step / 2);
  return clampRate(price);
}

/** P9-T5: سعر مورد حالي = base_price مع تعديلات الطلب/العرض المتراكمة —
 *  الدالة النقية تحسب من سجل التحركات (demandTrades − supplyOffers). */
export function priceAfterActivity(resource: string, demandTrades: number, supplyOffers: number): number {
  return adaptPrice(resourceBasePrice(resource), demandTrades, supplyOffers);
}

/** P9-T5: تحقق من صحة عرض جديد — الموردان معروفان وغير متطابقين، المبلغ ضمن النطاق،
 *  السعر ضمن النطاق، وعدد العروض النشطة تحت السقف، والمبلغ مضاعف للحد الأدنى. */
export function validateOffer(input: {
  sellResource: string;
  buyResource: string;
  amount: number;
  rate: number;
  activeOfferCount: number;
}): { ok: true } | { ok: false; error: string; min?: number; max?: number; cap?: number } {
  const c = tradingConstants();
  if (!isValidTradingResource(input.sellResource)) return { ok: false, error: "unknown_sell_resource" };
  if (!isValidTradingResource(input.buyResource)) return { ok: false, error: "unknown_buy_resource" };
  if (input.sellResource === input.buyResource) return { ok: false, error: "same_resource_pair" };
  const minAmt = Number(c.min_offer_amount);
  const maxAmt = Number(c.max_offer_amount);
  if (!Number.isFinite(input.amount) || input.amount < minAmt || input.amount > maxAmt)
    return { ok: false, error: "amount_out_of_range", min: minAmt, max: maxAmt };
  if (Math.round(input.amount / minAmt) * minAmt !== input.amount) return { ok: false, error: "amount_not_multiple" };
  const b = rateBounds();
  const rate = Number(input.rate);
  if (!Number.isFinite(rate) || rate < b.min || rate > b.max)
    return { ok: false, error: "rate_out_of_range", min: b.min, max: b.max };
  const maxOffers = Number(c.max_offers_per_player);
  if (input.activeOfferCount >= maxOffers) return { ok: false, error: "offer_cap_reached", cap: maxOffers };
  return { ok: true };
}

/** P9-T5: تحقق من إتمام صفقة شراء — العرض موجود وغير منتهٍ، المبلغ ضمن المتاح،
 *  سعر الطلب يطابق سعر العرض، والمشتري لم يتجاوز حده اليومي. */
export function validateClaim(input: {
  claimAmount: number;
  offerAmount: number;
  claimRate: number;
  offerRate: number;
  createdMs: number;
  ttlSec: number;
  nowMs: number;
  claimedToday: number;
}): { ok: true; amount: number } | { ok: false; error: string; min?: number; cap?: number } {
  const c = tradingConstants();
  if (input.createdMs + Number(c.offer_ttl_sec) * 1000 <= input.nowMs) return { ok: false, error: "offer_expired" };
  const minClaim = Number(c.min_claim_amount);
  if (!Number.isFinite(input.claimAmount) || input.claimAmount < minClaim)
    return { ok: false, error: "claim_too_small", min: minClaim };
  const amount = Math.min(input.claimAmount, input.offerAmount);
  if (Math.round(amount / minClaim) * minClaim !== amount) return { ok: false, error: "amount_not_multiple" };
  if (Number(input.claimRate) !== Number(input.offerRate)) return { ok: false, error: "rate_mismatch" };
  const cap = Number(c.max_claim_per_day);
  if (input.claimedToday + amount > cap * minClaim) return { ok: false, error: "daily_claim_cap", cap };
  return { ok: true, amount };
}

/** P9-T5: التسوية المالية للصفقة — من منظور المشتري:
 *  يدفع (rate × amount) من buyResource، يستلم amount من sellResource.
 *  من منظور البائع: يخصم من رصيده amount من sellResource، ويضاف netAmount من buyResource. */
export function settleTrade(offer: TradingOffer, amount: number): {
  buyerPaysBuy: number;
  buyerReceivesSell: number;
  sellerLosesSell: number;
  sellerGainsBuy: number;
  feeCharged: number;
} {
  const buyerPaysBuy = offer.rate * amount;
  const sellerLosesSell = amount;
  const sellerGainsBuy = Math.max(0, Math.floor(buyerPaysBuy * (1 - Number(tradingConstants().fee_rate))));
  const feeCharged = buyerPaysBuy - sellerGainsBuy;
  return { buyerPaysBuy, buyerReceivesSell: amount, sellerLosesSell, sellerGainsBuy, feeCharged };
}
