import commandersData from "../../data/commanders.json";

/**
 * نظام القادة (P2-T1)
 * كل قائد يملك 3 مهارات: attack (هجوم القوات)، defense (دفاع القوات عند الدفاع)، passive (سرعة مسير / خبرة).
 * التأثيرات تُقرأ من data/commanders.json — لا قيم ثابتة هنا.
 */

export type SkillEffect = { stat: string; per_level: number };

export type CommanderSkillDef = {
  id: string;
  slot: number;
  name: string;
  type: "attack" | "defense" | "passive";
  max_level: number;
  effects: SkillEffect[];
  description: string;
};

export type CommanderDef = {
  id: string;
  name: string;
  rarity: string;
  nation: string;
  tags: string[];
  base_stats: { attack: number; defense: number; utility: number };
  skills: CommanderSkillDef[];
};

export type CommanderInstance = {
  commanderId: string;
  level: number;
  skills: number[]; // مستويات المهارات الثلاث [attack, defense, passive]
  xp?: number; // خبرة تراكمية نحو المستوى التالي (تُستخدم في addXp)
  talentAllocations?: Record<string, number>; // P8-T1: نقاط المواهب الموزعة (nodeId -> points)
  equipmentState?: any; // P8-T2: معدات القائد المجهزة (EquipmentState) على المسيرة
};

const DEFS: Record<string, CommanderDef> = {};
for (const c of (commandersData as any).commanders as CommanderDef[]) {
  DEFS[c.id] = c;
}

export const COMMANDER_DEFS = DEFS;

export const COMMANDER_CONSTANTS = (commandersData as any).constants as {
  max_level: number;
  max_skill_level: number;
  xp_base: number;
  xp_level_mult: number;
  tome_xp: number;
  starter_tomes: number;
  summon_cost_gold: number;
  skill_upgrade_tome_cost: number;
};

export function getCommanderDef(id: string): CommanderDef | undefined {
  return DEFS[id];
}

export function isValidCommander(id: string): boolean {
  return !!DEFS[id];
}

/** XP المطلوب للانتقال من level إلى level+1 */
export function xpForLevel(level: number): number {
  return COMMANDER_CONSTANTS.xp_base + level * COMMANDER_CONSTANTS.xp_level_mult;
}

/** مجموع معدل هجوم القوات من مهارات نوع attack (نسبة مئوية كرقم عشري، مثلاً 0.08) */
export function commanderAttackMod(inst?: CommanderInstance | null): number {
  if (!inst) return 0;
  const def = DEFS[inst.commanderId];
  if (!def) return 0;
  let mod = 0;
  for (let i = 0; i < def.skills.length; i++) {
    const s = def.skills[i];
    if (s.type !== "attack") continue;
    const lvl = Math.max(0, Math.min(inst.skills[i] || 0, s.max_level));
    for (const e of s.effects) mod += e.per_level * lvl;
  }
  return mod;
}

/** مجموع معدل دفاع القوات من مهارات نوع defense (يُطبق على المدافع فقط) */
export function commanderDefenseMod(inst?: CommanderInstance | null): number {
  if (!inst) return 0;
  const def = DEFS[inst.commanderId];
  if (!def) return 0;
  let mod = 0;
  for (let i = 0; i < def.skills.length; i++) {
    const s = def.skills[i];
    if (s.type !== "defense") continue;
    const lvl = Math.max(0, Math.min(inst.skills[i] || 0, s.max_level));
    for (const e of s.effects) mod += e.per_level * lvl;
  }
  return mod;
}

/** معدل passive (سرعة مسير أو خبرة) حسب الإحصائية المطلوبة */
export function commanderPassiveMod(inst: CommanderInstance | null | undefined, stat: "march_speed" | "xp_gain"): number {
  if (!inst) return 0;
  const def = DEFS[inst.commanderId];
  if (!def) return 0;
  let mod = 0;
  for (let i = 0; i < def.skills.length; i++) {
    const s = def.skills[i];
    if (s.type !== "passive") continue;
    const lvl = Math.max(0, Math.min(inst.skills[i] || 0, s.max_level));
    for (const e of s.effects) if (e.stat === stat) mod += e.per_level * lvl;
  }
  return mod;
}

/** إضافة XP ومعالجة رفع المستوى التلقائي. يعيد النسخة المحدثة. */
export function addXp(inst: CommanderInstance, amount: number): CommanderInstance {
  const maxLevel = COMMANDER_CONSTANTS.max_level;
  let level = inst.level;
  let xp = (inst.xp || 0) + amount;
  while (level < maxLevel && xp >= xpForLevel(level)) {
    xp -= xpForLevel(level);
    level++;
  }
  if (level >= maxLevel) xp = 0;
  return { ...inst, level, xp };
}

/** قائد البداية لحضارة معينة (يُقرأ من civilizations.json starter_commander) */
export function commanderRarity(commanderId: string): string {
  return DEFS[commanderId]?.rarity || "common";
}

export function starterCommanderForCiv(civ: string): string | null {
  // القيمة تُقرأ من بيانات الحضارات؛ نتجنب import دائري هنا، لذا نطابق بالاسم
  const match = Object.values(DEFS).find((d) => d.nation === civ && d.id.endsWith("_starter"));
  return match ? match.id : null;
}
