import type { Troops } from "../../env";
import { commanderAttackMod, commanderDefenseMod, type CommanderInstance } from "./commanders";

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

const BRANCH: Record<string, "infantry" | "cavalry" | "archer"> = {
  infantry_t1: "infantry",
  cavalry_t1: "cavalry",
  archer_t1: "archer",
};

function unitAtk(unitId: string): number {
  let base = 10;
  if (unitId.includes("cavalry")) base = 12;
  else if (unitId.includes("archer")) base = 11;

  if (unitId.includes("_t2")) return base * 1.5;
  if (unitId.includes("_t3")) return base * 2.2;
  if (unitId.includes("_t4")) return base * 3.5;
  return base * 1.0;
}

function counterMult(att: string, def: string): number {
  const a = BRANCH[att] || "infantry";
  const d = BRANCH[def] || "infantry";
  // infantry > cavalry > archer > infantry
  if (a === "infantry" && d === "cavalry") return 1.15;
  if (a === "cavalry" && d === "archer") return 1.15;
  if (a === "archer" && d === "infantry") return 1.15;
  if (a === "cavalry" && d === "infantry") return 0.87;
  if (a === "archer" && d === "cavalry") return 0.87;
  if (a === "infantry" && d === "archer") return 0.87;
  return 1;
}

export function troopPower(troops: Troops): number {
  let p = 0;
  for (const [u, c] of Object.entries(troops)) {
    p += unitAtk(u) * Math.max(0, c || 0);
  }
  return p;
}

export function totalTroops(troops: Troops): number {
  return Object.values(troops).reduce((s, n) => s + Math.max(0, n || 0), 0);
}

export function resolveCombat(attacker: CombatSide, defender: CombatSide, zoneId: number = 1, attackerCommander?: CommanderStub, defenderCommander?: CommanderStub): CombatResult {
  const aPower = Math.max(1, troopPower(attacker.troops));
  const dPower = Math.max(1, troopPower(defender.troops));

  // weighted counter average
  let aMult = 1;
  let samples = 0;
  for (const au of Object.keys(attacker.troops)) {
    for (const du of Object.keys(defender.troops)) {
      aMult += counterMult(au, du);
      samples++;
    }
  }
  if (samples > 0) aMult = aMult / (samples + 1);

  // P2-T1: مهارة attack للطرفين + مهارة defense تخفض فعالية المهاجم ضد المدافع
  const aCommMod = 1 + commanderAttackMod(attackerCommander);
  const dCommMod = 1 + commanderAttackMod(defenderCommander);
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

  const attackerSplit: TroopSplit = { slightly: {}, severely: {}, dead: {} };
  for (const [u, c] of Object.entries(attackerLosses)) {
    attackerSplit.slightly[u] = Math.floor(c * slightRate);
    attackerSplit.dead[u] = Math.floor(c * deadRate);
    attackerSplit.severely[u] = c - attackerSplit.slightly[u] - attackerSplit.dead[u];
  }

  const defenderSplit: TroopSplit = { slightly: {}, severely: {}, dead: {} };
  for (const [u, c] of Object.entries(defenderLosses)) {
    defenderSplit.slightly[u] = Math.floor(c * slightRate);
    defenderSplit.dead[u] = Math.floor(c * deadRate);
    defenderSplit.severely[u] = c - defenderSplit.slightly[u] - defenderSplit.dead[u];
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

export function barbApCost(level: number): number {
	return 40 + level * 10;
}
