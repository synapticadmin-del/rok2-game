/**
 * P9-T1: تكنولوجيا التحالف — منطق نقي بلا اعتماد على node:fs ولا على الخادم.
 * كل الثوابت تُقرأ من data/alliance_tech.json.
 *
 * المفاهيم:
 * - كل تبرع عضو = `points_per_donation` نقطة تقنية (1 نقطة).
 * - نافذة التبرع: max 20 تبرعًا لكل نافذة 30 دقيقة (RoK تقريبًا: تبرع كل 90s).
 * - البحث الجماعي: ضباط R3+ يبدأون بحث تقنية (بحث واحد نشط لكل تحالف).
 * - المستوى التالي من تقنية يتطلب نقاطًا تراكمية (level_required) — البحث
 *   النشط يحول النقاط المجمعة إلى مستويات عند بلوغ العتبة.
 * - بافات المستوى المطبق (buffs) تُقرأ وتُجمع هنا؛ تطبيقها في نقاط اللعب
 *   (help/rally/training...) يتم في KingdomShard عبر AllianceTechService.
 */

import techData from "../../data/alliance_tech.json";

export type TechCategory = "development" | "territory" | "war" | "skill";

type TechEffect = { buff: string; per_level: number; unit: string };

export type AllianceTechDef = {
  id: string;
  category: TechCategory;
  name: string;
  levels: number;
  effect: TechEffect;
  level_required: number[];
};

type TechRaw = {
  id: string;
  category: string;
  name: string;
  levels: number;
  effect: TechEffect;
  level_required: number[];
};

export type DonationWindow = {
  windowStartMs: number;
  count: number;
};

export type TechProgress = {
  points: number;
  level: number;
  researchStartedAtMs: number | null;
};

export const ALLIANCE_TECH_CFG = (techData as any).donation as {
  points_per_donation: number;
  max_donations_per_window: number;
  window_seconds: number;
};

export const ALLIANCE_TECH_RESEARCH_CFG = (techData as any).research as {
  min_rank: string;
  max_active_research: number;
};

const TECHS = ((techData as any).techs as TechRaw[]).map((t) => ({
  id: t.id,
  category: t.category as TechCategory,
  name: t.name,
  levels: t.levels,
  effect: t.effect,
  level_required: t.level_required,
}));

export class AllianceTechService {
  /** تعريفات التقنيات من JSON — المصدر الوحيد. */
  static techs(): AllianceTechDef[] {
    return TECHS;
  }

  static techById(id: string): AllianceTechDef | undefined {
    return TECHS.find((t) => t.id === id);
  }

  static categories(): TechCategory[] {
    return ["development", "territory", "war", "skill"];
  }

  /** نقاط تبرع واحدة. */
  static pointsPerDonation(): number {
    return ALLIANCE_TECH_CFG.points_per_donation;
  }

  /** هل يمكن للاعب التبرع الآن؟ نافذة 30 دقيقة بسقف 20 تبرعًا. */
  static canDonate(nowMs: number, windows: DonationWindow[]): boolean {
    const active = windows.filter((w) => nowMs - w.windowStartMs < ALLIANCE_TECH_CFG.window_seconds * 1000);
    const count = active.reduce((s, w) => s + w.count, 0);
    return count < ALLIANCE_TECH_CFG.max_donations_per_window;
  }

  /** سجّل تبرعًا ورجّع النوافذ المحدثة (ينظف النوافذ المنتهية). */
  static recordDonation(nowMs: number, windows: DonationWindow[]): DonationWindow[] {
    const windowMs = ALLIANCE_TECH_CFG.window_seconds * 1000;
    const alive = windows.filter((w) => nowMs - w.windowStartMs < windowMs);
    const current = alive.find((w) => nowMs - w.windowStartMs < 15 * 60 * 1000);
    if (current) {
      current.count += 1;
      return alive;
    }
    return [...alive, { windowStartMs: nowMs, count: 1 }];
  }

  /** أقصى مستوى يمكن بلوغه بالنقاط التراكمية الحالية. */
  static levelForPoints(tech: AllianceTechDef, points: number): number {
    let level = 0;
    for (let i = 0; i < tech.level_required.length; i++) {
      if (points >= tech.level_required[i]) level = i + 1;
      else break;
    }
    return Math.min(level, tech.levels);
  }

  /** الحد الأدنى للنقاط للوصول إلى مستوى معين (للعرض). */
  static pointsForLevel(tech: AllianceTechDef, level: number): number {
    if (level <= 0) return 0;
    return tech.level_required[level - 1] ?? Number.MAX_SAFE_INTEGER;
  }

  /** حالة التقنية بعد تطبيق نقاط جديدة — المستويات الجديدة تُطبق تلقائيًا. */
  static applyPoints(progress: TechProgress, tech: AllianceTechDef, added: number): TechProgress {
    const points = progress.points + added;
    const level = AllianceTechService.levelForPoints(tech, points);
    return { ...progress, points, level };
  }

  /** بافات التحالف الفعلية عند لحظة ما: {buffKey: مجموع النسب}. */
  static computeBuffs(state: Record<string, TechProgress>): Record<string, number> {
    const buffs: Record<string, number> = {};
    for (const [techId, p] of Object.entries(state)) {
      const tech = AllianceTechService.techById(techId);
      if (!tech || p.level <= 0) continue;
      const key = tech.effect.buff;
      buffs[key] = (buffs[key] || 0) + tech.effect.per_level * p.level;
    }
    return buffs;
  }

  /** باف تقنية بعينها (للتحقق من combat/queues). */
  static buffValue(state: Record<string, TechProgress>, buff: string): number {
    return AllianceTechService.computeBuffs(state)[buff] || 0;
  }

  /** هل يمكن لرتبة اللاعب بدء بحث جماعي؟ */
  static canStartResearch(rank: string): boolean {
    return rankLevel(rank) >= rankLevel(ALLIANCE_TECH_RESEARCH_CFG.min_rank);
  }
}

/** رتبة عددية: R1=1 .. R5=5 — متسقة مع alliance.ts دون استيراد دوائري. */
export function rankLevel(rank: string): number {
  const m = /^R(\d+)$/i.exec(rank);
  return m ? Number(m[1]) : 0;
}

export default AllianceTechService;
