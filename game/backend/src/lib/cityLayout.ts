export const CITY_LAYOUT_FACADES = ["standard", "ceremonial", "fortified"] as const;
export type CityLayoutFacade = typeof CITY_LAYOUT_FACADES[number];

export type CityLayoutPlacement = {
  buildingId: string;
  q: number;
  r: number;
  rotationSteps: number;
  facade: CityLayoutFacade;
};

export type CityLayoutView = {
  version: number;
  updatedAt: number;
  placements: CityLayoutPlacement[];
};

const MEDIUM_BUILDINGS = new Set([
  "barracks", "stable", "archery_range", "siege_workshop", "hospital",
  "academy", "tavern", "trading_post", "alliance_center", "builders_hut",
]);

function layoutError(code: string): never {
  const error = new Error(code) as Error & { code: string };
  error.code = code;
  throw error;
}

/** يجب أن تبقى هذه العتبات مطابقة لـ ARok2CityLayoutActor::RadiusForCityHallLevel. */
export function cityLayoutRadiusForHallLevel(level: number): number {
  if (level >= 22) return 12;
  if (level >= 16) return 10;
  if (level >= 10) return 8;
  if (level >= 5) return 7;
  return 6;
}

function isInteger(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value);
}

function isFacade(value: unknown): value is CityLayoutFacade {
  return typeof value === "string" && (CITY_LAYOUT_FACADES as readonly string[]).includes(value);
}

function footprintRadius(buildingId: string): number {
  if (buildingId === "city_hall" || buildingId === "castle") return 2;
  return MEDIUM_BUILDINGS.has(buildingId) ? 1 : 0;
}

function insideRadius(q: number, r: number, radius: number): boolean {
  return Math.max(Math.abs(q), Math.abs(r), Math.abs(-q - r)) <= radius;
}

function occupiedCells(q: number, r: number, radius: number): Array<[number, number]> {
  const cells: Array<[number, number]> = [];
  for (let dq = -radius; dq <= radius; dq++) {
    const minDr = Math.max(-radius, -dq - radius);
    const maxDr = Math.min(radius, -dq + radius);
    for (let dr = minDr; dr <= maxDr; dr++) cells.push([q + dq, r + dr]);
  }
  return cells;
}

/**
 * يتحقق من تخطيط كامل قبل تخزينه. لا يأخذ هوية لاعب أو قائمة مبانٍ من العميل؛
 * تمرر الملكية من نتيجة قاعدة البيانات فقط.
 */
export function validateCityLayout(
  rawPlacements: unknown,
  ownedBuildings: Record<string, number>,
  hallLevel: number,
): CityLayoutPlacement[] {
  if (!Array.isArray(rawPlacements)) layoutError("layout_placements_required");
  const ownedIds = Object.keys(ownedBuildings).filter((id) => ownedBuildings[id] > 0).sort();
  const ownedIdSet = new Set(ownedIds);
  if (rawPlacements.length !== ownedIds.length || rawPlacements.length > 64) layoutError("layout_building_count_invalid");

  const radius = cityLayoutRadiusForHallLevel(hallLevel);
  const seenBuildings = new Set<string>();
  const occupied = new Set<string>();
  const validated: CityLayoutPlacement[] = [];

  for (const raw of rawPlacements) {
    if (!raw || typeof raw !== "object" || Array.isArray(raw)) layoutError("layout_placement_invalid");
    const placement = raw as Record<string, unknown>;
    const buildingId = placement.buildingId;
    const q = placement.q;
    const r = placement.r;
    const rotationSteps = placement.rotationSteps;
    const facade = placement.facade;

    if (typeof buildingId !== "string" || !ownedIdSet.has(buildingId)) layoutError("layout_building_not_owned");
    if (seenBuildings.has(buildingId)) layoutError("layout_building_duplicate");
    if (!isInteger(q) || !isInteger(r)) layoutError("layout_cell_invalid");
    if (!isInteger(rotationSteps) || rotationSteps < 0 || rotationSteps > 5) layoutError("layout_rotation_invalid");
    if (!isFacade(facade)) layoutError("layout_facade_invalid");

    if (buildingId === "city_hall" && (q !== 0 || r !== 0 || rotationSteps !== 0 || facade !== "standard")) {
      layoutError("layout_city_hall_fixed");
    }

    const footprint = footprintRadius(buildingId);
    for (const [cellQ, cellR] of occupiedCells(q, r, footprint)) {
      // السور يشغل الحلقة CityRadiusCells؛ المساحة الصالحة للبناء داخله فقط.
      if (!insideRadius(cellQ, cellR, radius - 1)) layoutError("layout_outside_city_wall");
      const cellKey = `${cellQ},${cellR}`;
      if (occupied.has(cellKey)) layoutError("layout_overlap");
      occupied.add(cellKey);
    }

    seenBuildings.add(buildingId);
    validated.push({ buildingId, q, r, rotationSteps, facade });
  }

  if (seenBuildings.size !== ownedIds.length || ownedIds.some((id) => !seenBuildings.has(id))) {
    layoutError("layout_buildings_incomplete");
  }

  return validated.sort((a, b) => a.buildingId.localeCompare(b.buildingId));
}
