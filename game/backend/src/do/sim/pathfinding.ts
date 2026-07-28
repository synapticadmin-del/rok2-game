import { dist } from "../../lib/ids";
import type { MapPass, MapRegion } from "../../lib/gameData";

export type Point = { x: number; y: number };

/** Axis-aligned mountain belts between zone1 region borders + pass openings. */
export function isBlocked(
  x: number,
  y: number,
  regions: MapRegion[],
  passes: MapPass[],
  mountainBelt: number,
  passWidth: number,
): boolean {
  // Outside world roughly from region bounds
  const worldMax = Math.max(...regions.map((r) => r.aabb[2]));
  if (x < 0 || y < 0 || x > worldMax || y > worldMax) return true;

  // Mountain if near a shared region border AND not inside a pass opening
  for (const r of regions.filter((z) => z.zone_id === 1)) {
    const [x0, y0, x1, y1] = r.aabb;
    // vertical borders
    for (const bx of [x0, x1]) {
      if (Math.abs(x - bx) <= mountainBelt / 2 && y >= y0 && y <= y1) {
        if (!insidePassOpening(x, y, passes, passWidth, mountainBelt)) return true;
      }
    }
    // horizontal borders
    for (const by of [y0, y1]) {
      if (Math.abs(y - by) <= mountainBelt / 2 && x >= x0 && x <= x1) {
        if (!insidePassOpening(x, y, passes, passWidth, mountainBelt)) return true;
      }
    }
  }
  return false;
}

function insidePassOpening(
  x: number,
  y: number,
  passes: MapPass[],
  passWidth: number,
  mountainBelt: number,
): boolean {
  for (const p of passes) {
    const [cx, cy] = p.center;
    // treat pass as rectangle around center
    if (Math.abs(x - cx) <= Math.max(passWidth, mountainBelt) / 2 && Math.abs(y - cy) <= Math.max(passWidth, mountainBelt) / 2) {
      return true;
    }
  }
  return false;
}

type AStarNode = {
  gx: number;
  gy: number;
  f: number;
  g: number;
  h: number;
  parent: AStarNode | null;
  crossedPasses: Set<string>;
};

/**
 * Coarse grid A* march planning (48x48 grid for 1200x1200 map).
 * Returns path length, waypoints, and whether path is legal.
 */
export function planMarch(
  from: Point,
  to: Point,
  regions: MapRegion[],
  passes: MapPass[],
  mountainBelt: number,
  passWidth: number,
  canTraversePass: (passId: string) => boolean,
): { ok: boolean; distance: number; reason?: string; crossedPasses: string[], waypoints?: Point[] } {
  const straightDistance = dist(from.x, from.y, to.x, to.y);
  const CELL_SIZE = 1200 / 48; // 25
  const startX = Math.max(0, Math.min(47, Math.floor(from.x / CELL_SIZE)));
  const startY = Math.max(0, Math.min(47, Math.floor(from.y / CELL_SIZE)));
  const goalX = Math.max(0, Math.min(47, Math.floor(to.x / CELL_SIZE)));
  const goalY = Math.max(0, Math.min(47, Math.floor(to.y / CELL_SIZE)));

  const openList: AStarNode[] = [];
  const closed = new Set<string>();

  openList.push({
    gx: startX,
    gy: startY,
    f: 0,
    g: 0,
    h: Math.abs(startX - goalX) + Math.abs(startY - goalY),
    parent: null,
    crossedPasses: new Set<string>(),
  });

  const getPassId = (cx: number, cy: number): string | null => {
    for (const p of passes) {
      const [px, py] = p.center;
      if (Math.abs(cx - px) <= Math.max(passWidth, mountainBelt) && Math.abs(cy - py) <= Math.max(passWidth, mountainBelt)) {
         return p.id;
      }
    }
    return null;
  };

  while (openList.length > 0) {
    openList.sort((a, b) => a.f - b.f);
    const curr = openList.shift()!;

    if (curr.gx === goalX && curr.gy === goalY) {
      const waypoints: Point[] = [];
      let node: AStarNode | null = curr;
      while (node) {
        waypoints.unshift({ x: node.gx * CELL_SIZE + CELL_SIZE/2, y: node.gy * CELL_SIZE + CELL_SIZE/2 });
        node = node.parent;
      }
      waypoints[0] = from;
      waypoints[waypoints.length - 1] = to;

      let distance = 0;
      for (let i = 0; i < waypoints.length - 1; i++) {
        distance += dist(waypoints[i].x, waypoints[i].y, waypoints[i+1].x, waypoints[i+1].y);
      }
      return { ok: true, distance, crossedPasses: [...curr.crossedPasses], waypoints };
    }

    const key = `${curr.gx},${curr.gy}`;
    if (closed.has(key)) continue;
    closed.add(key);

    const neighbors = [
      [0, -1], [0, 1], [-1, 0], [1, 0],
      [-1, -1], [1, -1], [-1, 1], [1, 1]
    ];

    for (const [dx, dy] of neighbors) {
      const nx = curr.gx + dx;
      const ny = curr.gy + dy;
      if (nx < 0 || ny < 0 || nx > 47 || ny > 47) continue;
      const nKey = `${nx},${ny}`;
      if (closed.has(nKey)) continue;

      const cx = nx * CELL_SIZE + CELL_SIZE/2;
      const cy = ny * CELL_SIZE + CELL_SIZE/2;

      const passId = getPassId(cx, cy);
      let blocked = isBlocked(cx, cy, regions, passes, mountainBelt, passWidth);

      if (blocked && passId) {
        blocked = false;
      }

      if (blocked) continue;

      const nCrossed = new Set(curr.crossedPasses);
      if (passId) {
        if (!canTraversePass(passId)) continue;
        nCrossed.add(passId);
      }

      const cost = (dx !== 0 && dy !== 0) ? 1.414 : 1;
      const g = curr.g + cost;
      const h = Math.abs(nx - goalX) + Math.abs(ny - goalY);
      const f = g + h;

      openList.push({ gx: nx, gy: ny, f, g, h, parent: curr, crossedPasses: nCrossed });
    }
  }

  return { ok: false, distance: straightDistance, reason: "path_blocked_by_mountain_or_pass", crossedPasses: [] };
}

export function marchDurationMs(distance: number, speedTilesPerSec = 20): number {
  // prototype: fast marches for playable tests (cap 8s)
  const ms = Math.floor((distance / Math.max(0.1, speedTilesPerSec)) * 1000);
  return Math.min(8000, Math.max(800, ms));
}
