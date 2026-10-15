import type { PersonalRoom } from "./companion";
import { planStreetRoute } from "./companionStreetRouting";
import { townSquareNavigation, townSquareStanding } from "./companionTownSquare";
import { roomLayoutFor } from "./companionRoomLayouts";
import { itemSurfaceHeight, itemVolume, roomObstacles, roomSurfaces, roomStandingPoints } from "./companionPlacement";

export type ScenePoint = readonly [number, number, number];

export type NavigationObstacle = { minX: number; maxX: number; minZ: number; maxZ: number;
  /** Local rectangle rotated around its centre, in radians. Omitted for existing rooms. */
  yaw?: number;
  /** Small world-space boundary, with enclosing min/max bounds. No rotation. */
  polygon?: readonly (readonly [number, number])[] };

export function navigationObstacleBounds(box: NavigationObstacle): NavigationObstacle {
  if (!box.yaw) return box;
  const x = (box.minX + box.maxX) / 2, z = (box.minZ + box.maxZ) / 2;
  const w = (box.maxX - box.minX) / 2, d = (box.maxZ - box.minZ) / 2;
  const c = Math.abs(Math.cos(box.yaw)), s = Math.abs(Math.sin(box.yaw));
  return { minX: x - w * c - d * s, maxX: x + w * c + d * s,
    minZ: z - w * s - d * c, maxZ: z + w * s + d * c };
}
export type RoomNavigation = {
  /** Changes whenever fixed geometry or editable furniture changes. */
  revision: string;
  bounds: NavigationObstacle;
  obstacles: readonly NavigationObstacle[];
  radius: number;
  floorY: number;
  /** Outdoor layouts retain a bounded grid without lowering interior precision. */
  gridSize?: number;
  /** Versioned outdoor topology; interiors retain the bounded precision grid. */
  streetGraph?: { points: readonly ScenePoint[]; edges: readonly (readonly [number, number])[] };
};

/** Closed segment versus a rectangle, including grazing contact. */
function intersects(a: ScenePoint, b: ScenePoint, box: NavigationObstacle, padding: number) {
  const minX = box.minX - padding, maxX = box.maxX + padding;
  const minZ = box.minZ - padding, maxZ = box.maxZ + padding;
  let ax = a[0], az = a[2], bx = b[0], bz = b[2];
  if (box.yaw) {
    // Rotate around the padded rectangle's centre, as in the swept-body check.
    const x = (minX + maxX) / 2, z = (minZ + maxZ) / 2;
    const c = Math.cos(box.yaw), s = Math.sin(box.yaw);
    ax = x + (a[0] - x) * c - (a[2] - z) * s;
    az = z + (a[0] - x) * s + (a[2] - z) * c;
    bx = x + (b[0] - x) * c - (b[2] - z) * s;
    bz = z + (b[0] - x) * s + (b[2] - z) * c;
  }
  let enter = 0, leave = 1;
  for (let axis = 0; axis < 2; axis++) {
    const origin = axis === 0 ? ax : az, delta = axis === 0 ? bx - ax : bz - az;
    const low = axis === 0 ? minX : minZ, high = axis === 0 ? maxX : maxZ;
    if (Math.abs(delta) < 1e-9) {
      if (origin < low || origin > high) return false;
      continue;
    }
    const first = (low - origin) / delta, second = (high - origin) / delta;
    enter = Math.max(enter, Math.min(first, second));
    leave = Math.min(leave, Math.max(first, second));
    if (enter > leave) return false;
  }
  return true;
}

function polygonContains(x: number, z: number, polygon: NonNullable<NavigationObstacle["polygon"]>) {
  let result = false;
  for (let i = 0, j = polygon.length - 1; i < polygon.length; j = i++) {
    const u = polygon[i], v = polygon[j];
    if ((u[1] > z) !== (v[1] > z) && x < (v[0] - u[0]) * (z - u[1]) / (v[1] - u[1]) + u[0]) result = !result;
  }
  return result;
}

function pointSegmentDistanceSquared(px: number, pz: number, ux: number, uz: number, vx: number, vz: number) {
  const x = vx - ux, z = vz - uz, length = x * x + z * z;
  const t = length ? Math.max(0, Math.min(1, ((px - ux) * x + (pz - uz) * z) / length)) : 0;
  return (px - ux - t * x) ** 2 + (pz - uz - t * z) ** 2;
}

const cross2D = (ux: number, uz: number, vx: number, vz: number, px: number, pz: number) =>
  (vx - ux) * (pz - uz) - (vz - uz) * (px - ux);

/** Swept disc versus a small polygon, including edge and corner contact. */
function intersectsPolygon(a: ScenePoint, b: ScenePoint, box: NavigationObstacle, radius: number) {
  if (!intersects(a, b, box, radius)) return false;
  const polygon = box.polygon!;
  const ax = a[0], az = a[2], bx = b[0], bz = b[2];
  if (polygonContains(ax, az, polygon) || polygonContains(bx, bz, polygon)) return true;
  for (let i = 0; i < polygon.length; i++) {
    const u = polygon[i], v = polygon[(i+1)%polygon.length];
    // Strict crossing; collinear/touching segments are caught by distance below.
    if (cross2D(ax, az, bx, bz, u[0], u[1]) * cross2D(ax, az, bx, bz, v[0], v[1]) < 0
      && cross2D(u[0], u[1], v[0], v[1], ax, az) * cross2D(u[0], u[1], v[0], v[1], bx, bz) < 0) return true;
    if (Math.min(pointSegmentDistanceSquared(ax, az, u[0], u[1], v[0], v[1]),
      pointSegmentDistanceSquared(bx, bz, u[0], u[1], v[0], v[1]),
      pointSegmentDistanceSquared(u[0], u[1], ax, az, bx, bz),
      pointSegmentDistanceSquared(v[0], v[1], ax, az, bx, bz)) <= radius * radius) return true;
  }
  return false;
}

function validNavigationObstacle(box: NavigationObstacle) {
  return Number.isFinite(box.minX) && Number.isFinite(box.maxX) && Number.isFinite(box.minZ) && Number.isFinite(box.maxZ)
    && box.minX <= box.maxX && box.minZ <= box.maxZ && (box.yaw === undefined || Number.isFinite(box.yaw))
    && (!box.polygon || (!box.yaw && box.polygon.length >= 3 && box.polygon.length <= 8
      && box.polygon.every(p => p.length === 2 && p.every(Number.isFinite)
        && p[0] >= box.minX && p[0] <= box.maxX && p[1] >= box.minZ && p[1] <= box.maxZ)));
}

export function routeSegmentClear(nav: RoomNavigation, a: ScenePoint, b: ScenePoint) {
  const { radius: r, bounds } = nav;
  if (!a.every(Number.isFinite) || !b.every(Number.isFinite) || !Number.isFinite(nav.floorY) || !Number.isFinite(r) || r <= 0 ||
    !validNavigationObstacle(bounds) || !nav.obstacles.every(validNavigationObstacle)) return false;
  for (let endpoint = 0; endpoint < 2; endpoint++) {
    const point = endpoint === 0 ? a : b;
    if (Math.abs(point[1] - nav.floorY) > .01 || point[0] < bounds.minX + r || point[0] > bounds.maxX - r ||
      point[2] < bounds.minZ + r || point[2] > bounds.maxZ - r) return false;
  }
  const minX = Math.min(a[0], b[0]), maxX = Math.max(a[0], b[0]);
  const minZ = Math.min(a[2], b[2]), maxZ = Math.max(a[2], b[2]);
  return !nav.obstacles.some(box => {
    let lowX = box.minX - r, highX = box.maxX + r, lowZ = box.minZ - r, highZ = box.maxZ + r;
    if (box.yaw) {
      const x = (box.minX + box.maxX) / 2, z = (box.minZ + box.maxZ) / 2;
      const width = (box.maxX - box.minX) / 2 + r, depth = (box.maxZ - box.minZ) / 2 + r;
      const c = Math.abs(Math.cos(box.yaw)), s = Math.abs(Math.sin(box.yaw));
      const halfX = width * c + depth * s, halfZ = width * s + depth * c;
      lowX = x - halfX; highX = x + halfX; lowZ = z - halfZ; highZ = z + halfZ;
    }
    // Enclose the same padded, rotated rectangle before the precise swept-body check.
    // Keep full validation above, including distant obstacles from mutable server input.
    const tolerance = 1e-9;
    if (maxX < lowX - tolerance || minX > highX + tolerance || maxZ < lowZ - tolerance || minZ > highZ + tolerance) return false;
    return box.polygon ? intersectsPolygon(a, b, box, r) : intersects(a, b, box, r);
  });
}

/** Bounded A* with swept-character edge checks, then collision-safe smoothing. */
export function planRoomRoute(nav: RoomNavigation, start: ScenePoint, destination: ScenePoint): ScenePoint[] | null {
  if (!routeSegmentClear(nav, start, start) || !routeSegmentClear(nav, destination, destination)) return null;
  if (routeSegmentClear(nav, start, destination)) return [destination];
  if (nav.streetGraph) return planStreetRoute(nav, start, destination, routeSegmentClear, planRoomRoute);
  const cell = nav.gridSize ?? .12;
  if (!Number.isFinite(cell) || cell < .12 || cell > 1) return null;
  const width = Math.ceil((nav.bounds.maxX - nav.bounds.minX) / cell) + 1;
  const depth = Math.ceil((nav.bounds.maxZ - nav.bounds.minZ) / cell) + 1;
  if (width * depth > 10000) return null;
  const point = (key: number): ScenePoint => [nav.bounds.minX + key % width * cell, nav.floorY, nav.bounds.minZ + Math.floor(key / width) * cell];
  const distance = (a: ScenePoint, b: ScenePoint) => Math.hypot(a[0] - b[0], a[2] - b[2]);
  const cost = new Map<number, number>(), parent = new Map<number, number>();
  const open = new Set<number>(), closed = new Set<number>();
  // Connect the true start to nearby clear cells, never snap through an obstacle.
  for (let key = 0; key < width * depth; key++) {
    const x = nav.bounds.minX + key % width * cell, z = nav.bounds.minZ + Math.floor(key / width) * cell;
    const length = Math.hypot(start[0] - x, start[2] - z);
    // The bounded scan retains its order; distant cells need no point array.
    if (length <= cell * 2 && routeSegmentClear(nav, start, [x, nav.floorY, z])) {
      cost.set(key, length); parent.set(key, -1); open.add(key);
    }
  }
  while (open.size) {
    let current = -1, best = Infinity;
    for (const key of open) {
      const x = nav.bounds.minX + key % width * cell, z = nav.bounds.minZ + Math.floor(key / width) * cell;
      const score = cost.get(key)! + Math.hypot(x - destination[0], z - destination[2]);
      if (score < best) { best = score; current = key; }
    }
    open.delete(current); closed.add(current);
    const here = point(current);
    if (distance(here, destination) <= cell * 2 && routeSegmentClear(nav, here, destination)) {
      const path: ScenePoint[] = [destination];
      for (let key = current; key !== -1; key = parent.get(key)!) path.unshift(point(key));
      const result: ScenePoint[] = [];
      let anchor = start, index = 0;
      while (index < path.length) {
        let next = path.length - 1;
        while (next > index && !routeSegmentClear(nav, anchor, path[next])) next--;
        result.push(path[next]); anchor = path[next]; index = next + 1;
      }
      return result;
    }
    const x = current % width, z = Math.floor(current / width);
    for (let dx = -1; dx <= 1; dx++) for (let dz = -1; dz <= 1; dz++) {
      if (!dx && !dz || x + dx < 0 || x + dx >= width || z + dz < 0 || z + dz >= depth) continue;
      const next = current + dx + dz * width;
      if (closed.has(next)) continue;
      const there = point(next), nextCost = cost.get(current)! + distance(here, there);
      if (nextCost >= (cost.get(next) ?? Infinity) || !routeSegmentClear(nav, here, there)) continue;
      cost.set(next, nextCost); parent.set(next, current); open.add(next);
    }
  }
  return null;
}

export const legacyRoomNavigation: RoomNavigation = {
  revision: "legacy-studio-v1", floorY: 0, radius: .3,
  bounds: { minX: -2.25, maxX: 2.25, minZ: -1.65, maxZ: 1.85 },
  obstacles: [...roomObstacles, ...roomSurfaces.filter(surface => surface.id === "shelf")].map(o => ({ minX: o.x - o.width / 2, maxX: o.x + o.width / 2, minZ: o.z - o.depth / 2, maxZ: o.z + o.depth / 2 })),
};

/** Shared geometry contract for local prediction and authoritative call routes. */
export function roomNavigationFor(room?: PersonalRoom | null, environmentId: "studio" | "terrace" | "town-square" = room?.environmentId ?? "studio"): RoomNavigation {
  if (!room && environmentId === "town-square") return townSquareNavigation;
  const layout = roomLayoutFor(room);
  let fixed: RoomNavigation = legacyRoomNavigation;
  if (room?.layoutId) fixed = {
    revision: layout.id, bounds: layout.bounds, floorY: 0, radius: layout.characterRadius,
    obstacles: layout.obstacles.filter(o => (o.y ?? 0) < layout.characterHeight).map(o => ({ minX: o.x - o.width / 2, maxX: o.x + o.width / 2, minZ: o.z - o.depth / 2, maxZ: o.z + o.depth / 2 })),
  };
  else if (environmentId === "terrace") fixed = {
    ...legacyRoomNavigation, revision: "terrace-v03", bounds: { ...legacyRoomNavigation.bounds, maxZ: 2.85 },
    obstacles: [...legacyRoomNavigation.obstacles,
      { minX: -1.42, maxX: 1.42, minZ: -3.31, maxZ: -2.73 },
      { minX: -3.5, maxX: -2.6, minZ: -.05, maxZ: .85 },
      { minX: 2.62, maxX: 3.38, minZ: -.28, maxZ: .48 },
      { minX: 1.85, maxX: 2.55, minZ: -3, maxZ: -2.3 },
    ],
  };
  else if (!room) fixed = { ...fixed, obstacles: [...fixed.obstacles, { minX: 1.62, maxX: 2.18, minZ: -.43, maxZ: .13 }] };
  if (!room) return fixed;
  return { ...fixed, revision: `${room.id}:${fixed.revision}:${JSON.stringify(room.items)}`,
    obstacles: [...fixed.obstacles, ...room.items.filter(item => itemSurfaceHeight(item, layout) < layout.characterHeight).map(item => {
      const volume = itemVolume(item, layout);
      return { minX: volume.x - volume.width / 2 - .08, maxX: volume.x + volume.width / 2 + .08,
        minZ: volume.z - volume.depth / 2 - .08, maxZ: volume.z + volume.depth / 2 + .08 };
    })],
  };
}

export function navigableStandingPoints(room?: PersonalRoom | null, environmentId?: "studio" | "terrace" | "town-square") {
  const navigation = roomNavigationFor(room, environmentId);
  if (!room && environmentId === "town-square") return townSquareStanding;
  return roomStandingPoints(room?.items ?? [], roomLayoutFor(room)).filter(point => {
    const at: ScenePoint = [point.x, navigation.floorY, point.z];
    return routeSegmentClear(navigation, at, at);
  });
}
