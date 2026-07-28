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
  // P3-T1: ثوابت خدمة الموسم (طول اليوم بالمللي ثانية + سقف أيام الموسم) — من JSON
  seasonDayMs: DATA.constants?.season_day_ms ?? 86_400_000,
  seasonMaxDay: DATA.constants?.season_max_day ?? 60,
};

/** P3-T1: إعداد خدمة فتح المناطق على مدار الموسم — تُقرأ من season_service في zones.json */
export const SEASON_SERVICE = {
  autoAdvance: DATA.season_service?.auto_advance ?? true,
  dayMs: DATA.season_service?.day_ms ?? DATA.constants?.season_day_ms ?? 86_400_000,
  maxDay: DATA.constants?.season_max_day ?? 60,
  announceEvents: (DATA.season_service?.announce_events as string[] | undefined) ?? ["zone_unlocked", "season_day"],
};

/** P3-T1: سجل فتح الموسم الكامل (zone1/zone2/zone3) — من season_unlock_schedule في JSON */
export type SeasonUnlockFeature = { day: number; feature: string };
export const SEASON_UNLOCK_SCHEDULE: SeasonUnlockFeature[] = (DATA.season_unlock_schedule ?? []) as SeasonUnlockFeature[];

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

// ---------------------------------------------------------------------------
// P3-T1 — Zone unlock service (جدول فتح الـ Zones كاملاً على السيرفر)
// ---------------------------------------------------------------------------

/**
 * يوم فتح العرش (Throne of Kingdoms) في قلب Zone 3 — من core_objective.open_day
 * في zones.json. لا قيمة ثابتة في الكود: العميل والسيرفر يقرآن من نفس JSON.
 */
export function throneUnlockDay(): number {
  const z3 = BY_ID[3];
  const day = (z3 as any)?.core_objective?.open_day;
  return typeof day === "number" ? day : 40;
}

/**
 * هل العرش مفتوح في هذا اليوم؟ (يتطلب بلوغ open_day — الميزة zone3_core_windows)
 */
export function isThroneUnlocked(seasonDay: number): boolean {
  return seasonDay >= throneUnlockDay();
}

/**
 * تقدّم يوم الموسم زمنياً — قلب خدمة الفتح (P3-T1).
 * يحسب اليوم الحالي من طوابع زمنية: كل seasonDayMs = يوم واحد، بحد أقصى seasonMaxDay.
 * حتمي بالكامل: نفس المدخلات ⇒ نفس اليوم، بدون أي حالة مخفية.
 */
export function seasonDayAt(seasonStartMs: number, nowMs: number): number {
  const dayMs = Math.max(1, SEASON_SERVICE.dayMs);
  const d = Math.floor((nowMs - seasonStartMs) / dayMs);
  return Math.max(0, Math.min(SEASON_SERVICE.maxDay, d));
}

/**
 * حالة فتح كاملة لكل ممرات/مناطق اللعبة في يوم معين —
 * الخلاصة الموحدة التي يستخدمها السيرفر للبث وللتحقق من المسيرات.
 */
export function seasonUnlockState(seasonDay: number) {
  return {
    day: seasonDay,
    throneUnlocked: isThroneUnlocked(seasonDay),
    throneDay: throneUnlockDay(),
    features: SEASON_UNLOCK_SCHEDULE.map((f) => ({ ...f, unlocked: seasonDay >= f.day })),
  };
}

/**
 * الجدول الزمني الكامل لفتح المناطق عبر الموسم — للعميل (مؤقتات/أقفال) وللاختبار.
 * يدمج: جدول الموسم (features) + المناطق (من unlock_schedule لكل Zone) + العرش.
 */
export function seasonSchedule(
  regions: Array<{ id: string; zone_id: number }>,
  passes: Array<{ id: string; unlockDay: number }>,
) {
  return {
    dayMs: SEASON_SERVICE.dayMs,
    maxDay: SEASON_SERVICE.maxDay,
    features: SEASON_UNLOCK_SCHEDULE,
    regions: regions.map((r) => ({ regionId: r.id, zoneId: r.zone_id, unlockDay: regionUnlockDay(r.id, r.zone_id) })),
    passes: passes.map((p) => ({ passId: p.id, unlockDay: p.unlockDay })),
    throne: { unlockDay: throneUnlockDay() },
  };
}

// ---------------------------------------------------------------------------
// P3-T2 — Zone 3 core contest (تسجيل نقاط الموسم عبر أهداف الاحتلال)
// ---------------------------------------------------------------------------

/** نوع هدف الاحتلال في قلب Zone 3. */
export type CoreObjectiveKind = "throne" | "outer_fort" | "side_altar";

/** إعداد مسابقة القلب (نقاط/حامية/مكاسب احتلال) — يُقرأ من core_contest في zones.json. */
export type CoreContestConfig = {
  holdScorePerTick: number;
  captureGainBase: number;
  captureGainPowerDiv: number;
  garrison: number;
  firstCaptureBonus: number;
};

const CORE: any = BY_ID[3] ?? {};
const CC: any = CORE.core_contest ?? {};

function contestCfg(kind: CoreObjectiveKind): CoreContestConfig {
  const c = CC[kind] ?? {};
  return {
    holdScorePerTick: c.hold_score_per_tick ?? (kind === "throne" ? 1 : kind === "outer_fort" ? 0.5 : 0.25),
    captureGainBase: c.capture_gain_base ?? (kind === "side_altar" ? 30 : 35),
    captureGainPowerDiv: c.capture_gain_power_div ?? (kind === "side_altar" ? 25 : 20),
    garrison: c.garrison ?? (kind === "throne" ? 2000 : kind === "outer_fort" ? 800 : 400),
    firstCaptureBonus: CC.first_capture_bonus ?? 25,
  };
}

/** نقاط الاحتفاظ لكل tick لهذا النوع من الأهداف — من JSON. */
export function holdScorePerTick(kind: CoreObjectiveKind): number {
  return contestCfg(kind).holdScorePerTick;
}

/** حامية الدفاع المحايدة لهذا النوع — من JSON. */
export function coreGarrison(kind: CoreObjectiveKind): number {
  return contestCfg(kind).garrison;
}

/** مكسب تقدّم الاحتلال من قوة القوات المتبقية — من JSON. */
export function coreCaptureGain(kind: CoreObjectiveKind, attackerRemainingPower: number): number {
  const c = contestCfg(kind);
  return Math.min(100, c.captureGainBase + Math.floor(attackerRemainingPower / c.captureGainPowerDiv));
}

/** مكافأة أول احتلال لهدف في الموسم — من JSON. */
export function firstCaptureBonus(): number {
  return CC.first_capture_bonus ?? 25;
}

/** هل هذا النوع من الأهداف يبدأ تسجيل نقاطه في هذا اليوم؟ (كلها مع فتح العرش يوم 40) */
export function coreContestActive(seasonDay: number): boolean {
  return isThroneUnlocked(seasonDay);
}
