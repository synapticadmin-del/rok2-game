import equipmentData from "../../data/equipment.json";
/**
 * نظام معدات القادة / الحدادة (P8-T2)
 * 6 خانات (weapon/helmet/chest/gloves/legs/boots)، 4 مواد × 5 جودات،
 * تصنيع من blueprints، دمج 4 معدات متطابقة للترقية، مكافآت مجموعات 2/4/6 —
 * بافات المعدات تُطبق على قوة المسيرة في القتال. كل القيم من data/equipment.json.
 */
export type EquipmentSlotDef = { id: string; name: string; name_ar: string; material: string };
export type EquipmentConstants = {
  blacksmith_unlock_city_hall_level: number;
  materials: string[];
  quality_order: string[];
  upgrade_merge_count: number;
  max_quality_index: number;
  max_blueprint_level: number;
  set_bonus_cap: number;
};
export type EquipmentItem = {
  id: string; // item uid
  slot: string;
  quality: string;
  stats: { stat: string; value: number }[];
  material: string;
};
export type EquipmentInventory = EquipmentItem[];
export type EquippedSlots = Record<string, EquipmentItem | null>; // slotId -> item | null
export type EquipmentState = { inventory: EquipmentInventory; equipped: EquippedSlots };
export type EquipmentMods = { troop_attack?: number; troop_defense?: number; troop_health?: number; march_speed?: number; training_speed?: number; siege_damage?: number };
export const EQUIPMENT_CONSTANTS = (equipmentData as any).constants as EquipmentConstants;
export const EQUIPMENT_SLOTS = (equipmentData as any).slots as EquipmentSlotDef[];
export const EQUIPMENT_QUALITIES = (equipmentData as any).qualities;
export const EQUIPMENT_BLUEPRINTS = (equipmentData as any).blueprints;
export const EQUIPMENT_MATERIAL_COSTS = (equipmentData as any).material_costs;
export const EQUIPMENT_STAT_RANGES = (equipmentData as any).stat_ranges;
export const EQUIPMENT_SET_BONUSES = (equipmentData as any).set_bonuses;
export const SLOT_INDEX: Record<string, EquipmentSlotDef> = {};
for (const s of EQUIPMENT_SLOTS) SLOT_INDEX[s.id] = s;
export function equipmentSlot(slotId: string): EquipmentSlotDef | undefined {
  return SLOT_INDEX[slotId];
}
/** عدد القطع المجهزة — لصندوق set bonus */
export function equippedCount(equipped: EquippedSlots | null | undefined): number {
  if (!equipped) return 0;
  let n = 0;
  for (const v of Object.values(equipped)) if (v) n++;
  return n;
}
/** باف المجموعة: 2/4/6 قطع → troop_attack. مقيد بـ set_bonus_cap */
export function setBonusMod(equipped: EquippedSlots | null | undefined): number {
  const n = equippedCount(equipped);
  const sb = EQUIPMENT_SET_BONUSES;
  const cap = EQUIPMENT_CONSTANTS.set_bonus_cap ?? 0.25;
  let bonus = 0;
  if (n >= 6) bonus = Math.max(bonus, sb["6_piece"] ?? 0);
  if (n >= 4) bonus = Math.max(bonus, sb["4_piece"] ?? 0);
  if (n >= 2) bonus = Math.max(bonus, sb["2_piece"] ?? 0);
  return Math.min(cap, bonus);
}
/** مجموع بافات القطع المجهزة (بما فيها set bonus) — يُمرر combat.ts */
export function computeEquipmentMods(state: EquipmentState | null | undefined): EquipmentMods {
  const mods: EquipmentMods = {};
  if (!state?.equipped) return mods;
  for (const item of Object.values(state.equipped)) {
    if (!item) continue;
    const mult = EQUIPMENT_QUALITIES[item.quality]?.quality_mult ?? 1;
    for (const st of item.stats) {
      const cur = mods[st.stat as keyof EquipmentMods] ?? 0;
      mods[st.stat as keyof EquipmentMods] = cur + st.value * mult;
    }
  }
  const setBonus = setBonusMod(state.equipped);
  if (setBonus > 0) {
    const cur = mods.troop_attack ?? 0;
    mods.troop_attack = cur + setBonus;
  }
  return mods;
}
/** باف troop_attack للمعدات (للاستهلاك في resolveCombat opts) */
export function equipmentAttackMod(state: EquipmentState | null | undefined): number {
  return computeEquipmentMods(state).troop_attack ?? 0;
}
/** جودة جديدة أعلى — index جودة الحالية + 1 */
export function nextQuality(quality: string): string | null {
  const idx = EQUIPMENT_QUALITIES[quality]?.index;
  if (idx === undefined) return null;
  const order = EQUIPMENT_CONSTANTS.quality_order as string[];
  if (idx >= EQUIPMENT_CONSTANTS.max_quality_index) return null;
  return order[idx + 1] ?? null;
}
/**
 * تصنيع قطعة جديدة من blueprint — يحدد الجودة من معامل الخانة
 * ويرجع item جديد. gold/resources/materials تُتحقق خارجيًا (router).
 */
export function craftEquipment(slotId: string, quality: string, seed: number = Date.now()): { item: EquipmentItem; error: string | null } {
  const slot = equipmentSlot(slotId);
  if (!slot) return { item: null as any, error: "Unknown equipment slot" };
  if (!(quality in EQUIPMENT_QUALITIES)) return { item: null as any, error: "Unknown quality" };
  const bp = EQUIPMENT_BLUEPRINTS[slotId];
  if (!bp) return { item: null as any, error: "No blueprint for this slot" };
  const mult = EQUIPMENT_QUALITIES[quality].quality_mult;
  const range = EQUIPMENT_STAT_RANGES[quality];
  const stats = bp.stats.map((stat: string, i: number) => {
    // توزيع شبه عشوائي داخل النطاق (deterministic عبر seed)
    const t = ((seed * 9301 + i * 49297 + stat.length * 7919) % 233280) / 233280;
    const value = Math.round(range.min + t * (range.max - range.min));
    return { stat, value: Math.round(value * mult * 10) / 10 };
  });
  const item: EquipmentItem = {
    id: `item_${slotId}_${quality}_${seed}`,
    slot: slotId,
    quality,
    stats,
    material: slot.material,
  };
  return { item, error: null };
}
/**
 * دمج 4 معدات متطابقة (نفس الخانة + نفس الجودة) لترقية لجودة أعلى.
 * يرجع القطعة المرقّاة أو رسالة خطأ.
 */
export function mergeEquipment(items: EquipmentItem[]): { item: EquipmentItem | null; error: string | null } {
  const need = EQUIPMENT_CONSTANTS.upgrade_merge_count ?? 4;
  if (!items || items.length !== need) return { item: null, error: `Merge requires exactly ${need} identical items` };
  const q = items[0].quality;
  const slot = items[0].slot;
  for (const it of items) {
    if (it.slot !== slot || it.quality !== q) return { item: null, error: "Items must share slot and quality" };
  }
  const nq = nextQuality(q);
  if (!nq) return { item: null, error: "Already at maximum quality" };
  const bp = EQUIPMENT_BLUEPRINTS[slot];
  const mult = EQUIPMENT_QUALITIES[nq].quality_mult;
  const range = EQUIPMENT_STAT_RANGES[nq];
  const stats = bp.stats.map((stat: string, i: number) => {
    const baseAvg = (range.min + range.max) / 2;
    // ترقية تحفظ متوسط القطع المدمجة (70%) + قيمة أساسية (30%)
    const mergedAvg = items.reduce((s, it) => {
      const st = it.stats.find((x) => x.stat === stat);
      return s + (st ? st.value : 0);
    }, 0) / items.length;
    const value = Math.round((baseAvg * 0.3 + mergedAvg * 0.7) * mult * 10) / 10;
    return { stat, value };
  });
  return {
    item: { id: `item_${slot}_${nq}_merged_${items[0].id.slice(-6)}`, slot, quality: nq, stats, material: items[0].material },
    error: null,
  };
}
/** تكلفة التصنيع لجودة معينة */
export function craftCost(quality: string): { gold: number; resource_cost: Record<string, number> } | null {
  const c = EQUIPMENT_MATERIAL_COSTS[quality];
  if (!c) return null;
  return { gold: c.gold, resource_cost: c.resource_cost };
}
