/** P9-T3: منطق نقية لمتجر التحالف والألقاب.
 * كل القيم تُقرأ من alliance_shop.json — لا قيم hard-coded. */
import spec from "../../data/alliance_shop.json";

export type TitleId = string;
export type ShopItemId = string;

export interface TitleDef {
  id: string;
  name: string;
  icon: string;
  buffs: Record<string, number>;
  description: string;
}

export interface ShopItem {
  id: string;
  name: string;
  category: string;
  price: number;
  max_per_alliance: number;
  description: string;
  grant: { type: string; amount: number };
}

export interface AllianceShopState {
  balance: number;
  dailyEarned: number;
  dailyEarnedDay: number; // seasonDay عند آخر إعادة ضبط
  items: Record<string, number>; // كمية مشتريات catalog
  titles: Record<string, string>; // titleId -> playerId
}

export const shopSpec = spec as unknown as {
  version: number;
  credits: {
    earn: {
      help_credit: { per_help: number };
      gift_claims: { per_claim: number };
      daily_cap: { amount: number };
      balance_cap: { amount: number };
    };
    rate_limits: { window_seconds: number; max_purchases_per_hour: number };
  };
  catalog: ShopItem[];
  titles: {
    max_granted_per_alliance: number;
    max_holders_per_title: number;
    definitions: TitleDef[];
  };
};

/** رصيد تحالف من help واحدة */
export function earnPerHelp(): number {
  return Number(shopSpec.credits.earn.help_credit.per_help);
}

/** رصيد تحالف من مطالبة هدية واحدة */
export function earnPerGiftClaim(): number {
  return Number(shopSpec.credits.earn.gift_claims.per_claim);
}

export function dailyCap(): number {
  return Number(shopSpec.credits.earn.daily_cap.amount);
}

export function balanceCap(): number {
  return Number(shopSpec.credits.earn.balance_cap.amount);
}

/** كسب رصيد بعد help: يحترم daily_cap وbalance_cap */
export function applyHelpCredit(state: AllianceShopState, seasonDay: number, now: number): { state: AllianceShopState; earned: number } {
  const s = { ...state };
  if (s.dailyEarnedDay !== seasonDay) {
    s.dailyEarned = 0;
    s.dailyEarnedDay = seasonDay;
  }
  const left = Math.max(0, dailyCap() - s.dailyEarned);
  const earned = Math.min(earnPerHelp(), left, balanceCap() - s.balance);
  s.balance = Math.min(balanceCap(), s.balance + earned);
  s.dailyEarned += earned;
  return { state: s, earned };
}

/** كسب رصيد بعد مطالبة هدية */
export function applyGiftClaimCredit(state: AllianceShopState, seasonDay: number): { state: AllianceShopState; earned: number } {
  const s = { ...state };
  if (s.dailyEarnedDay !== seasonDay) {
    s.dailyEarned = 0;
    s.dailyEarnedDay = seasonDay;
  }
  const left = Math.max(0, dailyCap() - s.dailyEarned);
  const earned = Math.min(earnPerGiftClaim(), left, balanceCap() - s.balance);
  s.balance = Math.min(balanceCap(), s.balance + earned);
  s.dailyEarned += earned;
  return { state: s, earned };
}

export function itemCatalog(): ShopItem[] {
  return shopSpec.catalog;
}

export function itemById(id: string): ShopItem | undefined {
  return itemCatalog().find((it) => it.id === id);
}

export function titleDefinitions(): TitleDef[] {
  return shopSpec.titles.definitions;
}

export function titleById(id: string): TitleDef | undefined {
  return titleDefinitions().find((t) => t.id === id);
}

export function maxGrantedTitles(): number {
  return Number(shopSpec.titles.max_granted_per_alliance);
}

/** التحقق من شراء عنصر من متجر التحالف. يعيد {ok, reason} بدلاً من رمي أخطاء. */
export function validatePurchase(state: AllianceShopState, itemId: string): { ok: true } | { ok: false; reason: string } {
  const item = itemById(itemId);
  if (!item) return { ok: false, reason: "unknown_shop_item" };
  if (state.balance < item.price) return { ok: false, reason: "insufficient_alliance_balance" };
  const bought = Number(state.items[itemId] || 0);
  if (bought >= Number(item.max_per_alliance)) return { ok: false, reason: "alliance_item_cap_reached" };
  return { ok: true };
}

/** تنفيذ الشراء: يخصم الرصيد ويضيف الكمية */
export function purchase(state: AllianceShopState, itemId: string, now: number): { state: AllianceShopState; item: ShopItem } | { ok: false; reason: string } {
  const v = validatePurchase(state, itemId);
  if (!v.ok) return v;
  const item = itemById(itemId)!;
  const s = { ...state, balance: state.balance - item.price, items: { ...state.items } };
  s.items[itemId] = (s.items[itemId] || 0) + 1;
  return { state: s, item };
}

/** التحقق من منح لقب (القائد R5 يمنح لعضو). لقب واحد لكل title، حتى max_granted. */
export function validateTitleGrant(state: AllianceShopState, titleId: string, targetPlayerId: string): { ok: true } | { ok: false; reason: string } {
  const def = titleById(titleId);
  if (!def) return { ok: false, reason: "unknown_title" };
  const grantedCount = Object.keys(state.titles).length;
  if (grantedCount >= maxGrantedTitles()) return { ok: false, reason: "alliance_title_cap_reached" };
  if (state.titles[titleId]) return { ok: false, reason: "title_already_granted" };
  return { ok: true };
}

/** تنفيذ منح لقب أو إعادة ضبط لقب موجود (القائد يمنح أو يغيّر حامل لقب) */
export function grantTitle(state: AllianceShopState, titleId: string, targetPlayerId: string): { state: AllianceShopState; title: TitleDef } | { ok: false; reason: string } {
  const def = titleById(titleId);
  if (!def) return { ok: false, reason: "unknown_title" };
  const s = { ...state, titles: { ...state.titles } };
  s.titles[titleId] = targetPlayerId;
  return { state: s, title: def };
}

/** سحب لقب من حامله (يُستدعى عند مغادرة التحالف أو طرده) */
export function revokeTitle(state: AllianceShopState, titleId: string): AllianceShopState {
  if (!state.titles[titleId]) return state;
  const s = { ...state, titles: { ...state.titles } };
  delete s.titles[titleId];
  return s;
}

export function revokeTitlesForPlayer(state: AllianceShopState, playerId: string): AllianceShopState {
  const kept: Record<string, string> = {};
  for (const [tid, pid] of Object.entries(state.titles)) if (pid !== playerId) kept[tid] = pid;
  return { ...state, titles: kept };
}

/** إجمالي بافات الألقاب التي يحملها لاعب داخل تحالفه */
export function titleBuffsForPlayer(state: AllianceShopState, playerId: string): Record<string, number> {
  const out: Record<string, number> = {};
  for (const [tid, pid] of Object.entries(state.titles)) {
    if (pid !== playerId) continue;
    const def = titleById(tid);
    if (!def) continue;
    for (const [stat, mod] of Object.entries(def.buffs)) out[stat] = (out[stat] || 0) + Number(mod);
  }
  return out;
}

export function allianceShopStateInitial(): AllianceShopState {
  return { balance: 0, dailyEarned: 0, dailyEarnedDay: 0, items: {}, titles: {} };
}
