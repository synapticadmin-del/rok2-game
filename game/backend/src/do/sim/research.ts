export type TechBranch = "economy" | "military" | "defense";

export type TechDef = {
  id: string;
  name: string;
  branch: TechBranch;
  maxLevel: number;
  academyLevelReq: (level: number) => number;
  cost: (level: number) => { food: number; wood: number; stone: number; gold: number };
  duration: (level: number) => number; // seconds
};

export const TECHNOLOGIES: Record<string, TechDef> = {
  eco_production: {
    id: "eco_production",
    name: "Economy Production Boost",
    branch: "economy",
    maxLevel: 5,
    academyLevelReq: (level) => Math.max(1, level * 2),
    cost: (level) => ({ food: level * 500, wood: level * 500, stone: 0, gold: 0 }),
    duration: (level) => level * 60,
  },
  mil_attack: {
    id: "mil_attack",
    name: "Military Attack Boost",
    branch: "military",
    maxLevel: 5,
    academyLevelReq: (level) => Math.max(1, level * 2),
    cost: (level) => ({ food: level * 600, wood: level * 600, stone: 0, gold: level * 100 }),
    duration: (level) => level * 60,
  },
  def_hospital: {
    id: "def_hospital",
    name: "Defense Hospital Capacity",
    branch: "defense",
    maxLevel: 5,
    academyLevelReq: (level) => Math.max(1, level * 2),
    cost: (level) => ({ food: level * 400, wood: level * 800, stone: level * 200, gold: 0 }),
    duration: (level) => level * 60,
  }
};
