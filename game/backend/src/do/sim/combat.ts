import type { Troops } from "../../env";
import { commanderAttackMod, commanderDefenseMod, type CommanderInstance } from "./commanders";
import { talentAttackMod, talentCounterMod } from "./talents";
import { equipmentAttackMod } from "./equipment";
import { unitAtk, counterMult as troopCounterMult } from "./troops";

export type CombatSide = {
  name: string;
  troops: Troops;
};

export type TroopSplit = {
  slightly: Troops;
  severely: Troops;
  dead: Troops;
};

export type CombatResult = {
  winner: "attacker" | "defender" | "draw";
  attackerLosses: Troops;
  defenderLosses: Troops;
  attackerSplit: TroopSplit;
  defenderSplit: TroopSplit;
  attackerRemaining: Troops;
  defenderRemaining: Troops;
  powerBefore: { attacker: number; defender: number };
};

export type CommanderStub = CommanderInstance;

// P8-T3: القوة ومثلث التفوق يُقرأان من troop_tiers.json عبر sim/troops.ts —
// الوحدات الخاصة الحضارية تحل محل فرعها عند unlock_tier (مثلثها هو فرعها).
// التوافقية: وحدة غير معروفة في JSON تعطي قوة 0 ومثلث 1 (لا تكسر قتالات قديمة).

export function troopPower(troops: Troops, civId?: string): number {
  let p = 0;
  for (const [u, c] of Object.entries(troops)) {
    p += unitAtk(u, civId) * Math.max(0, c || 0);
  }
  return p;
}

/** P9-T1: تضخيم/تقليص قوات المسيرة بمعامل (لبافات تقنيات التحالف عند ضرب القوات قبل القتال). */
export function scaleTroops(troops: Troops, factor: number): Troops {
  const out: Troops = {};
  for (const [u, c] of Object.entries(troops)) {
    out[u] = Math.max(0, Math.floor(Number(c) * factor));
  }
  return out;
}

export function totalTroops(troops: Troops): number {
  return Object.values(troops).reduce((s, n) => s + Math.max(0, n || 0), 0);
}

export function resolveCombat(attacker: CombatSide, defender: CombatSide, zoneId: number = 1, attackerCommander?: CommanderStub, defenderCommander?: CommanderStub, attackerResearchMod: number = 0, defenderResearchMod: number = 0, attackerTalentAttackMod: number = 0, defenderTalentAttackMod: number = 0, attackerEquipmentMod: number = 0, defenderEquipmentMod: number = 0, attackerCiv?: string, defenderCiv?: string): CombatResult {
  const aPower = Math.max(1, troopPower(attacker.troops, attackerCiv));
  const dPower = Math.max(1, troopPower(defender.troops, defenderCiv));

  // weighted counter average
  let aMult = 1;
  let samples = 0;
  for (const au of Object.keys(attacker.troops)) {
    for (const du of Object.keys(defender.troops)) {
      aMult += troopCounterMult(au, du);
      samples++;
    }
  }
  if (samples > 0) aMult = aMult / (samples + 1);
  // P8-T1: مواهب counter_damage تضيف ضررًا تفوقيًا فوق مثلث التفوق الافتراضي (سقف 0.15)
  aMult = Math.min(aMult + 0.15, aMult + (talentCounterMod(attackerCommander?.talentAllocations) || 0));

  // P2-T1: مهارة attack للطرفين + مهارة defense تخفض فعالية المهاجم ضد المدافع
  // P2-T3: باف أبحاث العسكر (troop_attack) يضاف للطرفين
  // P8-T1: باف troop_attack من شجرتي المواهب (troop_type + role) للطرفين
  const aCommMod = 1 + commanderAttackMod(attackerCommander) + attackerResearchMod + attackerTalentAttackMod + attackerEquipmentMod;
  const dCommMod = 1 + commanderAttackMod(defenderCommander) + defenderResearchMod + defenderTalentAttackMod + defenderEquipmentMod;
  const dDefMod = 1 - Math.min(0.5, commanderDefenseMod(defenderCommander));

  const aEff = aPower * aMult * aCommMod * dDefMod;
  const dEff = dPower * dCommMod;

  const aLossRatio = Math.min(0.95, dEff / (aEff + dEff));
  const dLossRatio = Math.min(0.95, aEff / (aEff + dEff));

  const attackerLosses: Troops = {};
  const defenderLosses: Troops = {};
  const attackerRemaining: Troops = {};
  const defenderRemaining: Troops = {};

  for (const [u, c] of Object.entries(attacker.troops)) {
    const loss = Math.floor(c * aLossRatio);
    attackerLosses[u] = loss;
    attackerRemaining[u] = Math.max(0, c - loss);
  }
  for (const [u, c] of Object.entries(defender.troops)) {
    const loss = Math.floor(c * dLossRatio * 1.05);
    defenderLosses[u] = Math.min(c, loss);
    defenderRemaining[u] = Math.max(0, c - defenderLosses[u]);
  }

  const aLeft = totalTroops(attackerRemaining);
  const dLeft = totalTroops(defenderRemaining);
  let winner: CombatResult["winner"] = "draw";
  if (aLeft > dLeft * 1.05) winner = "attacker";
  else if (dLeft > aLeft * 1.05) winner = "defender";

  const deadRate = zoneId === 3 ? 0.35 : 0.05;
  const severeRate = zoneId === 3 ? 0.30 : 0.60;
  const slightRate = 0.35;

  // تقسيم الخسائر على الفئات الثلاث باستخدام النسب مباشرة. severeRate كان يُحسب
  // ولا يُستخدم سابقاً (الجرحى الخطيرون كانوا يُشتقّون كباقٍ بدل نسبتهم المقصودة).
  const attackerSplit: TroopSplit = { slightly: {}, severely: {}, dead: {} };
  for (const [u, c] of Object.entries(attackerLosses)) {
    attackerSplit.slightly[u] = Math.floor(c * slightRate);
    attackerSplit.severely[u] = Math.floor(c * severeRate);
    attackerSplit.dead[u] = Math.max(0, c - attackerSplit.slightly[u] - attackerSplit.severely[u]);
  }

  const defenderSplit: TroopSplit = { slightly: {}, severely: {}, dead: {} };
  for (const [u, c] of Object.entries(defenderLosses)) {
    defenderSplit.slightly[u] = Math.floor(c * slightRate);
    defenderSplit.severely[u] = Math.floor(c * severeRate);
    defenderSplit.dead[u] = Math.max(0, c - defenderSplit.slightly[u] - defenderSplit.severely[u]);
  }

  return {
    winner,
    attackerLosses,
    defenderLosses,
    attackerSplit,
    defenderSplit,
    attackerRemaining,
    defenderRemaining,
    powerBefore: { attacker: aPower, defender: dPower },
  };
}

/**
 * P8-T2: باف معدات القادة (قطع مجهزة × جودة × set bonus 2/4/6) —
 * تُحسب في caller عبر equipmentAttackMod() وتُمرر attackerEquipmentMod/defenderEquipmentMod.
 */
export function equipmentCombatMods(state: { equipped?: Record<string, { stats: { stat: string; value: number }[] } | null> } | null | undefined): number {
  return equipmentAttackMod(state as any);
}

export function barbApCost(level: number): number {
	return 40 + level * 10;
}
