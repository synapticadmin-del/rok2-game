export type Commander = {
  id: string;
  name: string;
  level: number;
  star: number;
  skills: number[];
  xp: number;
};

export const COMMANDER_DEFS: Record<string, { id: string; name: string }> = {
  scipio: { id: "scipio", name: "Scipio" },
  suntzu: { id: "suntzu", name: "Sun Tzu" },
  baibars: { id: "baibars", name: "Baibars" },
  hermann: { id: "hermann", name: "Hermann" },
  joan: { id: "joan", name: "Joan of Arc" },
  cleo: { id: "cleo", name: "Cleopatra" },
  pelagius: { id: "pelagius", name: "Pelagius" },
  eulji: { id: "eulji", name: "Eulji Mundeok" },
  kusunoki: { id: "kusunoki", name: "Kusunoki Masashige" },
  lohar: { id: "lohar", name: "Lohar" },
};

export function getCommanderAttackMod(commander?: Commander): number {
  if (!commander) return 0;
  // 2% attack per skill point
  return commander.skills.reduce((sum, lvl) => sum + lvl * 0.02, 0);
}

export function xpForLevel(level: number): number {
  return level * 1000;
}
