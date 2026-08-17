// P19-T5: فهرس العناصر — الطبقة السلطوية بين `player_inventory` وواجهة الحقيبة.
//
// المشكلة قبل هذا الملف:
//
//   1. لا endpoint للحقيبة. `/v1/shop/catalog` يعيد `inventory` كخريطة
//      `{ item_id: count }` **بلا أي وصف للعنصر** — لا اسم ولا أيقونة ولا فئة.
//      وعلى العميل `HandleItemsAction` توست «الحقيبة قيد التجهيز».
//   2. المنح تكتب معرّفات لا يعرفها أي ملف بيانات: `lk_gems`،
//      `expedition_medal_token_sculpture`، `canyon_token_*`، ومفاتيح رميات
//      الحانة (`tavern:<player>:<day>:<n>`) — أي أن سطر الحقيبة سيكون معرّفاً
//      لاتينياً مركّباً وسط واجهة عربية.
//   3. خمسة مواضع في `KingdomShard` تكتب `INSERT INTO player_inventory
//      (player_id, day_key, key_id, amount)` — **أعمدة لا وجود لها**: الجدول في
//      `migrations/0005_shop.sql` أعمدته `(player_id, item_id, count, updated_at)`.
//      وكلها مغلّفة بـ`.catch(() => undefined)` فتفشل بصمت تام: اللاعب يفتح
//      صندوقاً ويرى النتيجة في الاستجابة ولا يدخل شيء حقيبته.
//
// كل الأسماء والفئات والأيقونات من `data/items.json` — لا ثابت هنا.

import { getItemsJson } from "../../lib/gameData";

export type ItemCategory = {
  id: string;
  name: string;
  icon: string;
  sort: number;
};

export type ItemDef = {
  id: string;
  category: string;
  name: string;
  description: string;
  icon: string;
  rarity: number;
  usable: boolean;
  use_action: string;
  /** للتسريعات فقط — الثواني التي يخصمها من الطابور. */
  seconds?: number;
};

export type InventoryEntry = {
  itemId: string;
  count: number;
  /** تعريف العنصر، أو null لمعرّف خارج الفهرس (يُعرض بمعرّفه ويُسجَّل). */
  def: ItemDef | null;
};

export function itemConstants() {
  return getItemsJson().constants;
}

export function itemCategories(): ItemCategory[] {
  return getItemsJson().categories as ItemCategory[];
}

export function itemCatalog(): ItemDef[] {
  return getItemsJson().items as ItemDef[];
}

export function getItem(itemId: string): ItemDef | null {
  if (!itemId) return null;
  return itemCatalog().find((i) => i.id === itemId) || null;
}

export function isKnownItem(itemId: string): boolean {
  return getItem(itemId) !== null;
}

/**
 * يترجم معرّفاً تاريخياً مركّباً إلى معرّف من الفهرس.
 *
 * ليست تجميلاً: خمسة مواضع في `KingdomShard` تكتب مفاتيح مبنيّة وقت التشغيل
 * (`expedition_medal_<itemId>`، `canyon_token_<itemId>`، `lk_<rewardKey>`،
 * ومفتاح رمية الحانة). تحويلها هنا يجعل الحقيبة تعرض «منحوتات قائد» بدل
 * `canyon_token_token_sculpture`، **ويحفظ التوافق** مع أي صفوف كُتبت قبل هذا
 * البند بدل حذفها.
 *
 * ما لا يُعرف يبقى كما هو — الواجهة تعرضه بمعرّفه ولا تخترع له اسماً.
 */
export function normalizeItemId(rawId: string): string {
  const id = String(rawId || "").trim();
  if (!id) return "";
  if (isKnownItem(id)) return id;

  // منحوتات القادة: كلها تدخل رصيداً واحداً — مصادرها الثلاثة (Expedition،
  // Canyon، Lost Kingdom) تمنح النوع نفسه بأسماء مفاتيح مختلفة.
  if (id.startsWith("expedition_medal_") || id.startsWith("canyon_token_")) {
    return "sculpture_shards";
  }
  if (id === "sculptureShards" || id === "sculpture_shards") {
    return "sculpture_shards";
  }
  if (id === "lk_sculpture_shards") return "sculpture_shards";
  if (id === "lk_speedups_8h") return "speedup_8h";
  if (id === "materials") return "equipment_materials";
  if (id === "expBoostPct") return "commander_exp_boost";

  // رميات الحانة كانت تُكتب بمفتاح مركّب (`tavern:<player>:<day>:<n>`) لا
  // بمعرّف عنصر — فيُترجم نوعه لا مفتاحه.
  if (id.startsWith("tavern:")) return "legendary_commander_sculpture";

  return id;
}

/** يبني صفوف الحقيبة من خريطة `{ item_id: count }` بترتيب الفئة ثم النُدرة. */
export function buildInventoryView(counts: Record<string, number>): InventoryEntry[] {
  // الدمج بعد التطبيع: `sculpture_shards` و`canyon_token_*` رصيدٌ واحد، فلو
  // بقيا سطرين لظهر للاعب رصيدان لعنصر واحد.
  const merged = new Map<string, number>();
  for (const [rawId, count] of Object.entries(counts || {})) {
    const amount = Math.max(0, Math.floor(Number(count) || 0));
    if (amount <= 0) continue;
    const id = normalizeItemId(rawId);
    merged.set(id, (merged.get(id) || 0) + amount);
  }

  const categoryOrder = new Map(itemCategories().map((c) => [c.id, c.sort]));
  const entries: InventoryEntry[] = [];
  for (const [itemId, count] of merged) {
    const def = getItem(itemId);
    entries.push({
      itemId,
      count: Math.min(count, Number(itemConstants().max_stack) || count),
      def,
    });
  }

  entries.sort((a, b) => {
    // المجهول آخراً: سطرٌ بلا تعريف لا يستحق صدر القائمة.
    const orderA = a.def ? (categoryOrder.get(a.def.category) ?? 99) : 999;
    const orderB = b.def ? (categoryOrder.get(b.def.category) ?? 99) : 999;
    if (orderA !== orderB) return orderA - orderB;
    const rarityA = a.def?.rarity ?? 0;
    const rarityB = b.def?.rarity ?? 0;
    if (rarityA !== rarityB) return rarityB - rarityA;
    return a.itemId.localeCompare(b.itemId);
  });

  return entries;
}
