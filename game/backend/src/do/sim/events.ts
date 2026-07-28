import eventsData from "../../data/events.json";

/**
 * أحداث يومية/أسبوعية (P3-T3): barbarians, resource rush, war_fever
 *
 * كل القواعد تُقرأ من data/events.json — لا قيم ثابتة هنا:
 *  - events[] تعرّف كل حدث: نمط التكرار (daily/weekly)، مدة النشاط (ticks)، وبافاته.
 *  - constants: أزمنة إعادة البذر وحدود التكثيف أثناء الأحداث.
 *
 * النموذج: الأحداث دورية حتمية من يوم الموسم (seasonDay) — لا حالة مخفية.
 * اليومي ينشط مرة كل يوم، والأسبوعي في يوم أسبوع محدد (weekday 0=أحد).
 * "نشط" يعني: الحدث في نافذته الزمنية لهذا اليوم، وبافاته مطبَّقة على العالم.
 */

export type EventRecurrence = "daily" | "weekly";

export type EventDef = {
  id: string;
  name: string;
  kind: string; // barbarians | resource_rush | war_fever
  recurrence: EventRecurrence;
  durationTicks: number;
  weekday?: number; // للأسبوعية: 0=أحد .. 6=سبت
  buffs: Record<string, number>;
  notify?: string;
};

const DATA = eventsData as any;

export const EVENT_CONSTANTS = {
  barbRespawnMs: DATA.constants?.barb_respawn_ms ?? 300_000,
  nodeRespawnMs: DATA.constants?.node_respawn_ms ?? 600_000,
  maxExtraSpawnPerRegion: DATA.constants?.max_extra_spawn_per_region ?? 3,
  barbBaseLevel: DATA.constants?.barb_base_level ?? 2,
};

const RAW_EVENTS: any[] = DATA.events ?? [];
export const EVENTS: EventDef[] = RAW_EVENTS.map((e) => ({
  id: e.id,
  name: e.name,
  kind: e.kind,
  recurrence: e.recurrence,
  durationTicks: e.duration_ticks ?? e.durationTicks ?? 300,
  weekday: e.weekday,
  buffs: e.buffs ?? {},
  notify: e.notify,
}));

const BY_ID: Record<string, EventDef> = {};
for (const e of EVENTS) BY_ID[e.id] = e;

export function getEvent(id: string): EventDef | undefined {
  return BY_ID[id];
}

/**
 * يوم الأسبوع ليوم الموسم (0=أحد). الموسم يبدأ يوم الأحد (day 0).
 * حتمي: weekday = seasonDay mod 7.
 */
export function seasonWeekday(seasonDay: number): number {
  return ((seasonDay % 7) + 7) % 7;
}

/**
 * هل الحدث مجدول للنشاط في هذا اليوم من الموسم؟
 * daily: كل يوم. weekly: فقط في weekday المحدد.
 */
export function isEventDay(ev: EventDef, seasonDay: number): boolean {
  if (ev.recurrence === "daily") return true;
  if (ev.recurrence === "weekly") return ev.weekday === undefined || seasonWeekday(seasonDay) === ev.weekday;
  return false;
}

/**
 * هل الحدث نشط الآن (في نافذته لهذا اليوم)؟
 * tickInDay = عدد الـ ticks منذ بداية اليوم (0 .. ticksPerDay-1).
 * الحدث ينشط في أول durationTicks من يومه.
 */
export function isEventActive(ev: EventDef, seasonDay: number, tickInDay: number): boolean {
  if (!isEventDay(ev, seasonDay)) return false;
  return tickInDay >= 0 && tickInDay < ev.durationTicks;
}

/**
 * حالة كل الأحداث في لحظة معينة — تُرسل للعميل (أيقونات/مؤقتات) وللسيرفر (تطبيق البافات).
 */
export function eventsStatus(seasonDay: number, tickInDay: number) {
  return EVENTS.map((ev) => {
    const active = isEventActive(ev, seasonDay, tickInDay);
    const scheduledToday = isEventDay(ev, seasonDay);
    return {
      id: ev.id,
      name: ev.name,
      kind: ev.kind,
      recurrence: ev.recurrence,
      active,
      scheduledToday,
      durationTicks: ev.durationTicks,
      ticksRemaining: active ? ev.durationTicks - tickInDay : 0,
      notify: ev.notify,
    };
  });
}

/** الأحداث النشطة حالياً (تُطبَّق بافاتها). */
export function activeEvents(seasonDay: number, tickInDay: number): EventDef[] {
  return EVENTS.filter((ev) => isEventActive(ev, seasonDay, tickInDay));
}

/**
 * قيمة باف مجمّعة عبر كل الأحداث النشطة لمفتاح معين (مضاعفات تُضرب، ثوابت تؤخذ قصوى).
 * additive=false ⇒ اضرب القيم (افتراضي 1). additive=true ⇒ اجمع القيم (افتراضي 0).
 */
export function eventBuff(seasonDay: number, tickInDay: number, key: string, additive = false): number {
  const active = activeEvents(seasonDay, tickInDay);
  if (additive) {
    let sum = 0;
    for (const ev of active) sum += ev.buffs[key] ?? 0;
    return sum;
  }
  let mult = 1;
  for (const ev of active) {
    const v = ev.buffs[key];
    if (typeof v === "number") mult *= v;
  }
  return mult;
}

/** عدد معسكرات البرابرة الإضافية لكل منطقة أثناء حدث البرابرة (ثابت additive). */
export function barbExtraPerRegion(seasonDay: number, tickInDay: number): number {
  return Math.min(EVENT_CONSTANTS.maxExtraSpawnPerRegion, eventBuff(seasonDay, tickInDay, "barb_spawn_extra_per_region", true));
}

/** مستوى إضافي للبرابرة أثناء الحدث (additive). */
export function barbLevelBonus(seasonDay: number, tickInDay: number): number {
  return eventBuff(seasonDay, tickInDay, "barb_level_bonus", true);
}
