/**
 * P9-T6: صناديق هدايا التحالف — منطق نقي.
 * شراء الباقات وهدايا الصناديق (P8-T6) + مصادر نصر/تبرعات يولّد صناديق جماعية
 * قابلة للمطالبة من كل أعضاء التحالف. كل القيم من alliance_gifts.json.
 */

export type GiftItem =
  | { kind: "resource"; resource: string; amount: number }
  | { kind: "speedup"; speedup_id: string; amount: number }
  | { kind: "gems"; amount: number };

export type AllianceGift = {
  id: string;
  allianceId: string;
  giftTypeId: string;
  items: GiftItem[];
  createdMs: number;
  expiresMs: number;
  openedBy: string[]; // player ids فتحوا الصندوق (سقف فتح واحد/عضو)
  maxOpeners: number; // عدد الأعضاء وقت الإنشاء (سقف المطالبين = عدد الأعضاء الحاليين)
};

export type GiftPoolEntry = {
  kind: "resource" | "speedup" | "gems";
  resource?: string;
  speedup_id?: string;
  amount: number;
  weight: number;
};

export type GiftType = {
  id: string;
  name: string;
  source: string;
  description: string;
  min_hall_level: number;
  open_weight: number;
  pool: GiftPoolEntry[];
};

export type AllianceGiftsSpec = {
  constants: {
    max_active_gifts_per_alliance: number;
    gift_open_window_ms: number;
    max_opens_per_member_per_gift: number;
    max_daily_opens_per_member: number;
    min_members_for_gift: number;
  };
  gift_types: GiftType[];
};

export function giftPoolTotalWeight(pool: GiftPoolEntry[]): number {
  return pool.reduce((sum, e) => sum + Math.max(0, e.weight), 0);
}

/** اختيار عنصر من pool بوزن مرجح (RNG مخارجي عبر rand). */
export function pickPoolItem(
  pool: GiftPoolEntry[],
  rand: () => number,
): GiftItem | null {
  const total = giftPoolTotalWeight(pool);
  if (total <= 0) return null;
  let roll = rand() * total;
  for (const entry of pool) {
    roll -= Math.max(0, entry.weight);
    if (roll <= 0) {
      if (entry.kind === "resource") return { kind: "resource", resource: entry.resource!, amount: entry.amount };
      if (entry.kind === "speedup") return { kind: "speedup", speedup_id: entry.speedup_id!, amount: entry.amount };
      return { kind: "gems", amount: entry.amount };
    }
  }
  const last = pool[pool.length - 1];
  if (last.kind === "resource") return { kind: "resource", resource: last.resource!, amount: last.amount };
  if (last.kind === "speedup") return { kind: "speedup", speedup_id: last.speedup_id!, amount: last.amount };
  return { kind: "gems", amount: last.amount };
}

export function isGiftExpired(gift: AllianceGift, now: number): boolean {
  return now >= gift.expiresMs;
}

export function giftOpenSlotsRemaining(gift: AllianceGift, memberCount: number): number {
  return Math.max(0, memberCount - gift.openedBy.length);
}

/** محاولة إضافة صندوق جديد لتحالف: يفشل إذا السقف نشط أو قاعة المدينة الأدنى لم تبلغ. */
export function createGift(opts: {
  allianceId: string;
  giftTypeId: string;
  hallLevel: number;
  memberCount: number;
  activeGiftCount: number;
  spec: AllianceGiftsSpec;
  now: number;
  rand: () => number;
}): { ok: true; gift: AllianceGift } | { ok: false; reason: "gift_type_unknown" | "hall_level_low" | "max_active_gifts" | "no_members" | "empty_pool" } {
  const { spec } = opts;
  const giftType = spec.gift_types.find((t) => t.id === opts.giftTypeId);
  if (!giftType) return { ok: false, reason: "gift_type_unknown" };
  if (opts.hallLevel < giftType.min_hall_level) return { ok: false, reason: "hall_level_low" };
  if (opts.memberCount < spec.constants.min_members_for_gift) return { ok: false, reason: "no_members" };
  if (opts.activeGiftCount >= spec.constants.max_active_gifts_per_alliance) return { ok: false, reason: "max_active_gifts" };
  const item = pickPoolItem(giftType.pool, opts.rand);
  if (!item) return { ok: false, reason: "empty_pool" };
  const gift: AllianceGift = {
    id: `gift:${opts.allianceId}:${opts.giftTypeId}:${opts.now}`,
    allianceId: opts.allianceId,
    giftTypeId: giftType.id,
    items: [item],
    createdMs: opts.now,
    expiresMs: opts.now + spec.constants.gift_open_window_ms,
    openedBy: [],
    maxOpeners: opts.memberCount,
  };
  return { ok: true, gift };
}

/** مطالبة عضو بفتحه الصندوق: فحص العنصر الواحد وتوزيعه + سجل الفاتحين. */
export function claimGift(opts: {
  gift: AllianceGift;
  playerId: string;
  memberIds: string[];
  dailyOpens: number; // عدد الفتحات اليومية لهذا العضو
  spec: AllianceGiftsSpec;
  now: number;
}): { ok: true; item: GiftItem; reward: { resource?: { resource: string; amount: number }; speedup?: { speedup_id: string; amount: number }; gems?: number }; opened: boolean } | { ok: false; reason: "gift_expired" | "not_member" | "already_opened" | "gift_full" | "daily_cap" } {
  if (isGiftExpired(opts.gift, opts.now)) return { ok: false, reason: "gift_expired" };
  if (!opts.memberIds.includes(opts.playerId)) return { ok: false, reason: "not_member" };
  if (opts.gift.openedBy.includes(opts.playerId)) return { ok: false, reason: "already_opened" };
  if (giftOpenSlotsRemaining(opts.gift, opts.memberIds.length) <= 0) return { ok: false, reason: "gift_full" };
  if (opts.dailyOpens >= opts.spec.constants.max_daily_opens_per_member) return { ok: false, reason: "daily_cap" };
  const item = opts.gift.items[0];
  const reward =
    item.kind === "resource" ? { resource: { resource: item.resource, amount: item.amount } } :
    item.kind === "speedup" ? { speedup: { speedup_id: item.speedup_id, amount: item.amount } } :
    { gems: item.amount };
  const openedBy = opts.gift.openedBy.concat(opts.playerId);
  return {
    ok: true,
    item,
    reward,
    opened: true,
  };
}

export function buildGiftForPersistence(gift: AllianceGift, playerIds: string[]): AllianceGift {
  return { ...gift, openedBy: playerIds };
}

export function expiredGifts(gifts: AllianceGift[], now: number): AllianceGift[] {
  return gifts.filter((g) => isGiftExpired(g, now));
}
