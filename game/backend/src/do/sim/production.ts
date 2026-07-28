import { productionPerHour } from "../../lib/gameData";

export type BuildingLevels = Record<string, number>;

export type ResourceState = {
  food: number;
  wood: number;
  stone: number;
  gold: number;
  updated_at: number;
};

export function applyProduction(city: ResourceState, buildings: BuildingLevels, now: number): ResourceState {
  const dtH = Math.max(0, (now - city.updated_at) / 3_600_000);
  if (dtH <= 0) return { ...city, updated_at: now };

  const foodRate = productionPerHour("farm", buildings.farm || 1);
  const woodRate = productionPerHour("lumber_mill", buildings.lumber_mill || 1);
  const stoneRate = productionPerHour("quarry", buildings.quarry || 1);
  const goldRate = productionPerHour("goldmine", buildings.goldmine || 1);

  return {
    food: city.food + foodRate * dtH,
    wood: city.wood + woodRate * dtH,
    stone: city.stone + stoneRate * dtH,
    gold: city.gold + goldRate * dtH,
    updated_at: now,
  };
}

export function canAfford(
  city: ResourceState,
  cost: { food: number; wood: number; stone: number; gold: number },
): boolean {
  return city.food >= cost.food && city.wood >= cost.wood && city.stone >= cost.stone && city.gold >= cost.gold;
}

export function spend(
  city: ResourceState,
  cost: { food: number; wood: number; stone: number; gold: number },
): ResourceState {
  return {
    ...city,
    food: city.food - cost.food,
    wood: city.wood - cost.wood,
    stone: city.stone - cost.stone,
    gold: city.gold - cost.gold,
  };
}
