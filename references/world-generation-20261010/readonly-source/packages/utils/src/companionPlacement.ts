import type { CompanionFurniture } from "./companion";
import { floorRoomKinds, tabletopRoomKinds, isWallSurface, isRoomItemKind, isPaidRoomItem, roomItemDefinitions, type HorizontalRoomSurfaceId, type WallRoomSurfaceId } from "./companionRoomItems";
type RoomItem = CompanionFurniture;
type RoomKind = RoomItem["kind"];
type RoomSurfaceId = NonNullable<RoomItem["surface"]>;
type RoomPlacement = { item: RoomItem; snapped: boolean; valid: boolean; reason: string | null };

export type RoomSurface = {
  id: HorizontalRoomSurfaceId; name: string; x: number; z: number; y: number; width: number; depth: number;
  kinds: RoomKind[]; anchors: { x: number; z: number }[]; supports?: readonly string[];
  /** Measured empty space above a support, including overhanging shades/leaves. */
  clearance?: { x: number; z: number; width: number; depth: number; height: number };
};
export type RoomObstacle = { id: string; x: number; z: number; width: number; depth: number; height: number; y?: number };
export type RoomFloorPatch = { x: number; z: number; width: number; depth: number; y: number };
export type RoomWall = {
  id: WallRoomSurfaceId; name: string; x: number; z: number; yaw: number;
  width: number; minY: number; maxY: number;
  /** Openings and fixed wall decor, measured in wall-local horizontal coordinates. */
  exclusions?: readonly { u: number; y: number; width: number; height: number }[];
};
export type RoomLayout = {
  id: string; surfaces: readonly RoomSurface[]; obstacles: readonly RoomObstacle[];
  walls?: readonly RoomWall[];
  /** Shallow walkable finishes, such as rugs. Routes remain on the base floor. */
  floorPatches?: readonly RoomFloorPatch[];
  bounds: { minX: number; maxX: number; minZ: number; maxZ: number };
  objectScale: number; characterRadius: number; characterHeight: number;
  standingCandidates: readonly { x: number; z: number; yaw: number }[];
  legacyPlacement?: boolean;
};
export const roomSurfaces: RoomSurface[] = [
  { id: "floor", name: "Floor", x: 0, z: .1, y: 0, width: 4.5, depth: 3.5, kinds: floorRoomKinds, anchors: [-1.45, -.7, 0, .75, 1.5].flatMap(x => [-.65, .25, 1.05].map(z => ({ x, z }))) },
  { id: "console", name: "Console", x: 1.05, z: -.72, y: .78, width: 1.75, depth: .9, kinds: tabletopRoomKinds, anchors: [{ x: .65, z: -.72 }, { x: 1.45, z: -.72 }] },
  { id: "shelf", name: "Shelf", x: -.95, z: -1.2, y: 1.48, width: 1.9, depth: .82, kinds: tabletopRoomKinds, anchors: [{ x: -1.4, z: -1.2 }, { x: -.5, z: -1.2 }] },
];
export const roomObstacles = [
  { id: "chair", x: -1.65, z: .35, width: .95, depth: 1.25, height: 1.2 },
  { id: "console", x: 1.05, z: -.72, width: 1.75, depth: .9, height: .78 },
];
export const roomBodies: Record<RoomKind, { radius: number; height: number; mass: number }> = roomItemDefinitions;
export const legacyRoomLayout: RoomLayout = {
  id: "legacy-room-v1", surfaces: roomSurfaces, obstacles: roomObstacles,
  bounds: { minX: -2.25, maxX: 2.25, minZ: -1.65, maxZ: 1.85 },
  objectScale: 1, characterRadius: .3, characterHeight: 1.45, legacyPlacement: true,
  walls: [
    { id: "wall-back", name: "Back wall", x: 0, z: -1.985, yaw: 0, width: 5.1, minY: .2, maxY: 3.5,
      exclusions: [{ u: .15, y: .3, width: 1.4, height: 2.75 }, { u: 1.65, y: 1.45, width: .75, height: 1.05 }] },
    { id: "wall-left", name: "Left wall", x: -2.485, z: .15, yaw: Math.PI / 2, width: 4.5, minY: .2, maxY: 3.5 },
  ],
  standingCandidates: [1.4, .65, 1.05].flatMap(z => [-.65, .65, 0, -1.2, 1.2].map(x => ({ x, z, yaw: 0 }))),
};
// Match makeRoomObject's full silhouette separately from its supported base.
// Legacy saves keep their original placement and collision dimensions.
const authoredBodies: Partial<Record<RoomKind, { radius: number; supportRadius: number; height: number }>> = {
  lamp: { radius: .35, supportRadius: .27, height: .93 },
  plant: { radius: .44, supportRadius: .25, height: 1.22 },
  stool: { radius: .37, supportRadius: .33, height: .6 },
};
export function roomBody(kind: RoomKind, layout = legacyRoomLayout) {
  const original = roomBodies[kind];
  const body = !layout.legacyPlacement && authoredBodies[kind] || { ...original, supportRadius: original.radius };
  return { mass: original.mass, radius: body.radius * layout.objectScale,
    supportRadius: body.supportRadius * layout.objectScale, height: body.height * layout.objectScale };
}
/** Centre positions whose base is supported and whose silhouette stays in the room/bay. */
export function surfacePlacementBounds(kind: RoomKind, surface: RoomSurface, layout = legacyRoomLayout) {
  const body = roomBody(kind, layout);
  const radius = surface.id === "floor" ? body.radius : body.supportRadius;
  const bounds = { minX: surface.x - surface.width / 2 + radius, maxX: surface.x + surface.width / 2 - radius,
    minZ: surface.z - surface.depth / 2 + radius, maxZ: surface.z + surface.depth / 2 - radius };
  if (!layout.legacyPlacement) {
    bounds.minX = Math.max(bounds.minX, layout.bounds.minX + body.radius);
    bounds.maxX = Math.min(bounds.maxX, layout.bounds.maxX - body.radius);
    bounds.minZ = Math.max(bounds.minZ, layout.bounds.minZ + body.radius);
    bounds.maxZ = Math.min(bounds.maxZ, layout.bounds.maxZ - body.radius);
  }
  if (surface.clearance) {
    const space = surface.clearance;
    bounds.minX = Math.max(bounds.minX, space.x - space.width / 2 + body.radius);
    bounds.maxX = Math.min(bounds.maxX, space.x + space.width / 2 - body.radius);
    bounds.minZ = Math.max(bounds.minZ, space.z - space.depth / 2 + body.radius);
    bounds.maxZ = Math.min(bounds.maxZ, space.z + space.depth / 2 - body.radius);
  }
  return bounds;
}
export const surfaceFor = (item: Pick<RoomItem, "surface">, layout = legacyRoomLayout) => layout.surfaces.find(surface => surface.id === (item.surface ?? "floor")) ?? layout.surfaces[0];
export const wallFor = (item: Pick<RoomItem, "surface">, layout = legacyRoomLayout) => layout.walls?.find(wall => wall.id === item.surface);
export const itemSupportName = (item: Pick<RoomItem, "surface">, layout = legacyRoomLayout) => wallFor(item, layout)?.name ?? surfaceFor(item, layout).name;
export function wallPosition(wall: RoomWall, u: number, y: number) {
  return { x: wall.x + Math.cos(wall.yaw) * u, z: wall.z - Math.sin(wall.yaw) * u, y, rotation: wall.yaw, surface: wall.id };
}
export function wallHorizontal(item: Pick<RoomItem, "x" | "z">, wall: RoomWall) {
  return (item.x - wall.x) * Math.cos(wall.yaw) - (item.z - wall.z) * Math.sin(wall.yaw);
}
/** Axis-aligned volume for collisions, camera clearance and navigation. Wall decor stays thin. */
export function itemVolume(item: RoomItem, layout = legacyRoomLayout) {
  const body = roomBody(item.kind, layout), definition = roomItemDefinitions[item.kind];
  const wall = wallFor(item, layout);
  const width = definition.wallWidth !== undefined ? definition.wallWidth * layout.objectScale : body.radius * 2;
  const depth = (definition.wallDepth ?? .1) * layout.objectScale;
  const c = wall ? Math.abs(Math.cos(wall.yaw)) : 0, s = wall ? Math.abs(Math.sin(wall.yaw)) : 0;
  return { x: item.x + (wall ? Math.sin(wall.yaw) * depth / 2 : 0), z: item.z + (wall ? Math.cos(wall.yaw) * depth / 2 : 0),
    y: itemSurfaceHeight(item, layout), width: wall ? width * c + depth * s : body.radius * 2,
    depth: wall ? width * s + depth * c : body.radius * 2, height: body.height };
}
function volumesOverlap(a: RoomObstacle | ReturnType<typeof itemVolume>, b: RoomObstacle | ReturnType<typeof itemVolume>, clearance = 0) {
  return (a.y ?? 0) < (b.y ?? 0) + b.height && (a.y ?? 0) + a.height > (b.y ?? 0)
    && Math.abs(a.x - b.x) < (a.width + b.width) / 2 + clearance
    && Math.abs(a.z - b.z) < (a.depth + b.depth) / 2 + clearance;
}
function wallPlacementError(item: RoomItem, items: RoomItem[], layout: RoomLayout): string | null {
  const wall = wallFor(item, layout), definition = roomItemDefinitions[item.kind];
  if (definition.placement !== "wall") return "This item cannot be placed on a wall.";
  if (!wall) return "Choose a wall in this room.";
  if (item.y === undefined || !Number.isFinite(item.y)) return "Choose a height on the wall.";
  const u = wallHorizontal(item, wall), expected = wallPosition(wall, u, item.y);
  if (Math.hypot(item.x - expected.x, item.z - expected.z) > .001 || Math.abs(shortestAngle(item.rotation - wall.yaw)) > .001) return "Place this item flat against the wall.";
  const width = definition.wallWidth! * layout.objectScale, height = definition.height * layout.objectScale;
  if (Math.abs(u) + width / 2 > wall.width / 2 || item.y < wall.minY || item.y + height > wall.maxY) return "Keep the whole item on the wall.";
  if (wall.exclusions?.some(area => Math.abs(u - area.u) < (width + area.width) / 2 + .02 && item.y! < area.y + area.height && item.y! + height > area.y)) return "Keep windows, doors and fixed decor clear.";
  const volume = itemVolume(item, layout);
  if (layout.obstacles.some(obstacle => volumesOverlap(volume, obstacle, .015))) return "There's furniture here.";
  if (items.some(other => other.id !== item.id && volumesOverlap(volume, itemVolume(other, layout), .04))) return "Leave some space between objects.";
  return null;
}
export function floorHeightAt(point: { x: number; z: number }, layout = legacyRoomLayout, radius = 0) {
  const baseFloor = layout.surfaces.find(surface => surface.id === "floor")?.y ?? 0;
  let height = baseFloor;
  let depression = false;
  for (const patch of layout.floorPatches ?? []) {
    const dx = Math.max(0, Math.abs(point.x - patch.x) - patch.width / 2);
    const dz = Math.max(0, Math.abs(point.z - patch.z) - patch.depth / 2);
    if (dx * dx + dz * dz <= radius * radius) {
      if (patch.y < baseFloor) {
        height = Math.min(height, patch.y);
        depression = true;
      } else if (!depression) {
        height = Math.max(height, patch.y);
      }
    }
  }
  return height;
}
export function itemSurfaceHeight(item: Pick<RoomItem, "kind" | "x" | "y" | "z" | "surface">, layout = legacyRoomLayout) {
  if (isWallSurface(item.surface)) return item.y ?? 0;
  const surface = surfaceFor(item, layout);
  return surface.id === "floor" ? floorHeightAt(item, layout, roomBody(item.kind, layout).supportRadius) : surface.y;
}
export function roomPositionClear(items: readonly RoomItem[], position: { x: number; z: number }, layout = legacyRoomLayout) {
  const radius = layout.characterRadius, bounds = layout.bounds;
  if (![position.x, position.z].every(Number.isFinite) || position.x < bounds.minX + radius || position.x > bounds.maxX - radius || position.z < bounds.minZ + radius || position.z > bounds.maxZ - radius) return false;
  if (layout.obstacles.some(obstacle => (obstacle.y ?? 0) < layout.characterHeight && Math.abs(position.x - obstacle.x) <= obstacle.width / 2 + radius && Math.abs(position.z - obstacle.z) <= obstacle.depth / 2 + radius)) return false;
  return !items.some(item => {
    if (isWallSurface(item.surface)) {
      const volume = itemVolume(item, layout);
      return volume.y < layout.characterHeight && Math.abs(position.x - volume.x) < volume.width / 2 + radius && Math.abs(position.z - volume.z) < volume.depth / 2 + radius;
    }
    const reach = roomBody(item.kind, layout).radius + radius + .08;
    if (layout.legacyPlacement) return (item.surface ?? "floor") === "floor" && Math.hypot(position.x - item.x, position.z - item.z) < reach;
    return itemSurfaceHeight(item, layout) < layout.characterHeight && Math.abs(position.x - item.x) <= reach && Math.abs(position.z - item.z) <= reach;
  });
}
/** Shared by personalized scenes and call placement. */
export function roomStandingPoints(items: readonly RoomItem[], layout = legacyRoomLayout) {
  const points: { x: number; z: number; yaw: number }[] = [];
  for (const point of layout.standingCandidates) {
    if (roomPositionClear(items, point, layout) && points.every(other => Math.hypot(other.x - point.x, other.z - point.z) >= layout.characterRadius * 2 + .02)) points.push({ ...point });
  }
  return points;
}
export function onSurface(item: RoomItem, surface: RoomSurface, margin = 0, layout = legacyRoomLayout) {
  const body = roomBody(item.kind, layout);
  const radius = (surface.id === "floor" ? body.radius : body.supportRadius) + margin;
  return Math.abs(item.x - surface.x) <= surface.width / 2 - radius && Math.abs(item.z - surface.z) <= surface.depth / 2 - radius;
}
export function itemPlacementError(item: RoomItem, items: RoomItem[], collisionClearance = 0, layout = legacyRoomLayout): string | null {
  if (!isRoomItemKind(item.kind) || ![item.x, item.z, item.rotation].every(Number.isFinite)) return "This object has an invalid position.";
  if (isWallSurface(item.surface)) return wallPlacementError(item, items, layout);
  if (roomItemDefinitions[item.kind].placement === "wall") return "This item belongs on a wall.";
  if (isPaidRoomItem(item.kind) && !item.surface) return "Choose a room surface.";
  if (item.y !== undefined) return "This item's height is set by its support.";
  if (item.surface && !layout.surfaces.some(surface => surface.id === item.surface)) return "Choose a room surface.";
  const surface = surfaceFor(item, layout);
  const surfaceY = itemSurfaceHeight(item, layout);
  const body = roomBody(item.kind, layout);
  const radius = body.radius + collisionClearance;
  if (!surface.kinds.includes(item.kind)) return "This item belongs on the floor.";
  const legacyBounds = layout.legacyPlacement && !item.surface && Math.abs(item.x) <= 1.85 && item.z >= -1.15 && item.z <= 1.6;
  // The grid uses these inset bounds. Its extra clearance covers obstacles
  // between sample centres, not another edge inset.
  if (!legacyBounds && !onSurface(item, surface, 0, layout)) return "Move the object onto a surface.";
  if (!layout.legacyPlacement) {
    const bounds = surfacePlacementBounds(item.kind, surface, layout);
    if (item.x < bounds.minX || item.x > bounds.maxX || item.z < bounds.minZ || item.z > bounds.maxZ) return "Move the object into the clear space.";
    if (surface.clearance && body.height > surface.clearance.height) return "This object is too tall for this surface.";
  }
  // Older saved rooms had only a ground plane. Preserve their data until moved.
  if ((!layout.legacyPlacement || item.surface === "floor") && layout.obstacles.some(obstacle => {
    if (surface.supports?.includes(obstacle.id)) return false;
    const bottom = obstacle.y ?? 0;
    return surfaceY < bottom + obstacle.height && surfaceY + body.height > bottom && Math.abs(item.x - obstacle.x) < obstacle.width / 2 + radius && Math.abs(item.z - obstacle.z) < obstacle.depth / 2 + radius;
  })) return "There's furniture here.";
  if (items.some(other => {
    if (other.id === item.id || !roomBodies[other.kind]) return false;
    if (isWallSurface(other.surface)) return volumesOverlap(itemVolume(item, layout), itemVolume(other, layout), .04);
    const otherY = itemSurfaceHeight(other, layout);
    const otherBody = roomBody(other.kind, layout);
    const verticalOverlap = surfaceY < otherY + otherBody.height && surfaceY + body.height > otherY;
    const spacing = layout.legacyPlacement && !item.surface && !other.surface ? .65 : radius + otherBody.radius + .08;
    return verticalOverlap && Math.hypot(item.x - other.x, item.z - other.z) < spacing;
  })) return "Leave some space between objects.";
  return null;
}
export function placementError(items: RoomItem[], layout = legacyRoomLayout): string | null {
  if (items.length > 8) return "The room fits up to eight objects.";
  if (new Set(items.map(item => item.id)).size !== items.length) return "Room objects must have unique IDs.";
  for (const item of items) { const error = itemPlacementError(item, items, 0, layout); if (error) return error; }
  return null;
}
export function resolvePlacement(item: RoomItem, items: RoomItem[], magnet = .28, previousAnchor?: { x: number; z: number }, layout = legacyRoomLayout) : RoomPlacement {
  if (isWallSurface(item.surface) || roomItemDefinitions[item.kind]?.placement === "wall") {
    const reason = itemPlacementError(item, items, 0, layout);
    return { item, snapped: false, valid: !reason, reason };
  }
  const surface = surfaceFor(item, layout);
  const raw = { ...item, surface: surface.id };
  // Hysteresis keeps a held object from flickering between snap points.
  const heldAnchor = previousAnchor && Math.hypot(raw.x - previousAnchor.x, raw.z - previousAnchor.z) < magnet * 1.55 ? previousAnchor : undefined;
  const anchor = heldAnchor ?? surface.anchors.reduce<{ x: number; z: number } | undefined>((best, point) => {
    const distance = Math.hypot(raw.x - point.x, raw.z - point.z);
    if (distance > magnet || itemPlacementError({ ...raw, ...point }, items, 0, layout)) return best;
    return !best || distance < Math.hypot(raw.x - best.x, raw.z - best.z) ? point : best;
  }, undefined);
  const candidate = anchor ? { ...raw, ...anchor } : raw;
  const reason = itemPlacementError(candidate, items, 0, layout);
  return { item: candidate, snapped: !!anchor && !reason, valid: !reason, reason };
}
export function rotationSnap(angle: number, previous?: number) {
  const step = Math.PI / 4;
  if (previous !== undefined && Math.abs(shortestAngle(angle - previous)) < .19) return previous;
  const target = Math.round(angle / step) * step;
  return Math.abs(angle - target) < .12 ? target : angle;
}
export function shortestAngle(angle: number) { return Math.atan2(Math.sin(angle), Math.cos(angle)); }
export function availablePlacement(kind: RoomKind, items: RoomItem[], preferred?: RoomSurfaceId, layout = legacyRoomLayout): RoomItem | null {
  if (roomItemDefinitions[kind].placement === "wall") {
    for (const wall of layout.walls ?? []) {
      if (preferred && preferred !== wall.id) continue;
      const height = roomBody(kind, layout).height;
      const ys = [Math.max(wall.minY, 1.65 - height / 2), ...Array.from({ length: 15 }, (_, i) => wall.minY + i * .2)];
      for (const y of ys) for (let u = -wall.width / 2; u <= wall.width / 2; u += .15) {
        const item = { id: "new-room-object", kind, ...wallPosition(wall, u, y) };
        if (!itemPlacementError(item, items, 0, layout)) return item;
      }
    }
    return null;
  }
  const order = preferred ? layout.surfaces.filter(surface => surface.id === preferred) : [...layout.surfaces].sort((a, b) => Number(b.id === (kind === "lamp" ? "console" : "floor")) - Number(a.id === (kind === "lamp" ? "console" : "floor")));
  const lighting = kind === "lamp" || kind === "floor-lamp" || kind === "desk-lamp" ? { lit: true } : {};
  for (const surface of order) {
    if (!surface.kinds.includes(kind)) continue;
    for (const anchor of surface.anchors) {
      const item: RoomItem = { id: "new-room-object", kind, ...anchor, rotation: 0, surface: surface.id, ...lighting };
      if (!itemPlacementError(item, items, 0, layout)) return item;
    }
    // Anchors are convenient defaults, not the only usable space in the room.
    const bounds = surfacePlacementBounds(kind, surface, layout);
    const step = Math.max(.12, .25 * layout.objectScale);
    for (let z = bounds.minZ; z <= bounds.maxZ; z += step) for (let x = bounds.minX; x <= bounds.maxX; x += step) {
      const item: RoomItem = { id: "new-room-object", kind, x, z, rotation: 0, surface: surface.id, ...lighting };
      if (!itemPlacementError(item, items, 0, layout)) return item;
    }
  }
  return null;
}

export type SpringValue = { value: number; velocity: number };
/** Semi-implicit spring integration with bounded substeps. No free furniture fling. */
export function stepSpring(spring: SpringValue, target: number, seconds: number, mass: number, held = false) {
  const stiffness = held ? 1100 : 240;
  const damping = 2 * Math.sqrt(stiffness * mass) * (held ? .9 : 1);
  const steps = Math.max(1, Math.ceil(seconds * 120));
  const dt = Math.min(seconds / steps, 1 / 120);
  for (let i = 0; i < steps; i++) {
    spring.velocity += ((target - spring.value) * stiffness - spring.velocity * damping) / mass * dt;
    spring.value += spring.velocity * dt;
  }
  if (Math.abs(spring.velocity) < .001 && Math.abs(spring.value - target) < .0005) { spring.value = target; spring.velocity = 0; }
}
