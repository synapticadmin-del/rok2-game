import zonesData from "../../data/zones.json";

/**
 * فتح المناطق بالمؤقت الزمني + مناطق الموارد الأعلى (P2-T4)
 *
 * كل القواعد تُقرأ من data/zones.json — لا قيم ثابتة هنا:
 *  - unlock_schedule لكل Zone يحدد اليوم والمناطق/الممرات التي تُفتح فيه.
 *  - resource_level_range يحدد مستويات عقد الموارد لكل Zone.
 *  - constants.zone2_richness_mult يضاعف غنى عقد Zone 2 (موارد أعلى).
 *
 * "stubs": المنطقة تبقى مرئية على الخريطة لكنها مقفلة حتى يوم الفتح،
 * والمسيرات إليها تُرفض بـ zone_locked.
 */

export type ZoneUnlockEntry = { day: number; regions: string[]; passes: string[] };

export type ZoneDef = {
  zone_id: number;
  key: string;
  name: string;
  role: string;
  regions: Array<{ id: string; name: string }>;
  pass_level_range?: [number, number];
  resource_level_range?: [number, number];
  holy_sites_per_region?: number;
  unlock?: { day: number; mode: string };
  unlock_schedule?: ZoneUnlockEntry[];
};

const DATA = zonesData as any;
export const ZONES: ZoneDef[] = DATA.zones;
export const ZONE_CONSTANTS = {
  resourceAmountPerLevel: DATA.constants?.resource_amount_per_level ?? 2000,
  resourceBaseAmount: DATA.constants?.resource_base_amount ?? 5000,
  zone2RichnessMult: DATA.constants?.zone2_richness_mult ?? 1.5,
  barbHpPerLevel: DATA.constants?.barb_hp_per_level ?? 100,
};

const BY_ID: Record<number, ZoneDef> = {};
for (const z of ZONES) BY_ID[z.zone_id] = z;

export function getZone(zoneId: number): ZoneDef | undefined {
  return BY_ID[zoneId];
}

/** نطاق مستويات الموارد للمنطقة (zone_id) — من JSON */
export function resourceLevelRange(zoneId: number): [number, number] {
  return BY_ID[zoneId]?.resource_level_range ?? [1, 4];
}

/**
 * يوم فتح منطقة معينة: أصغر day في unlock_schedule يذكر المنطقة.
 * null = المنطقة مفتوحة منذ اليوم صفر (لا قيد زمني).
 */
export function regionUnlockDay(regionId: string, zoneId: number): number | null {
  const z = BY_ID[zoneId];
  if (!z) return null;
  for (const e of z.unlock_schedule ?? []) {
    if (e.regions.includes(regionId)) return e.day;
  }
  return null;
}

/** يوم فتح ممر معين حسب جدول المنطقة التي يربط إليها (أعلى zone في zone_link). */
export function passUnlockDay(passId: string, zoneLink: number[] | undefined): number | null {
  const zoneId = Math.max(...(zoneLink && zoneLink.length ? zoneLink : [1]));
  const z = BY_ID[zoneId];
  if (!z) return null;
  for (const e of z.unlock_schedule ?? []) {
    if (e.passes.includes(passId)) return e.day;
  }
  return null;
}

/** هل المنطقة مفتوحة في هذا اليوم من الموسم؟ */
export function isRegionUnlocked(regionId: string, zoneId: number, seasonDay: number): boolean {
  const day = regionUnlockDay(regionId, zoneId);
  return day === null || seasonDay >= day;
}

/** حالة فتح كل المناطق — تُرسل للعميل ضمن snapshot ليرسم الأقفال والمؤقتات. */
export function zonesStatus(
  seasonDay: number,
  regions: Array<{ id: string; zone_id: number }>,
): Array<{ zoneId: number; regionId: string; unlocked: boolean; unlockDay: number | null }> {
  return regions.map((r) => {
    const day = regionUnlockDay(r.id, r.zone_id);
    return { zoneId: r.zone_id, regionId: r.id, unlocked: day === null || seasonDay >= day, unlockDay: day };
  });
}

/**
 * مستوى عقدة موارد/برابرة في منطقة معينة بشكل حتمي (deterministic من id العقدة).
 * يقع دائماً ضمن resource_level_range للمنطقة.
 */
export function nodeLevelForRegion(regionId: string | null, zoneId: number, seed: string): number {
  const [min, max] = resourceLevelRange(zoneId);
  const span = Math.max(1, max - min + 1);
  let h = 0;
  const s = `${regionId ?? ""}:${seed}`;
  for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) >>> 0;
  return min + (h % span);
}

/** غنى عقدة موارد (الكمية المتاحة للجمع) — من JSON. barb يستخدم hp per level. */
export function nodeRichness(kind: string, level: number, zoneId: number): number {
  if (kind === "barb") return ZONE_CONSTANTS.barbHpPerLevel * level;
  const base = ZONE_CONSTANTS.resourceBaseAmount + ZONE_CONSTANTS.resourceAmountPerLevel * (level - 1);
  const mult = zoneId >= 2 ? ZONE_CONSTANTS.zone2RichnessMult : 1;
  return Math.floor(base * mult);
}

/** أقصى day في جدول فتح منطقة ما (لرسائل العميل: "تُفتح بالكامل يوم X"). */
export function zoneFullyUnlockedDay(zoneId: number): number | null {
  const z = BY_ID[zoneId];
  if (!z || !z.unlock_schedule || z.unlock_schedule.length === 0) return null;
  return Math.max(...z.unlock_schedule.map((e) => e.day));
}
