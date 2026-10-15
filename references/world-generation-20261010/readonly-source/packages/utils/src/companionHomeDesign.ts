import type { PersonalRoom } from "./companion";
import type { RoomLayout, RoomObstacle } from "./companionPlacement";

/** Versioned additions to a saved room. Missing designs retain the authored coordinates. */
export type CompanionHomeDesign = { version: 1; turns: 0 | 1 | 2 | 3; extension: 0 | 0.6 | 1.2 };
type DesignedRoom = Pick<PersonalRoom, "environmentId" | "layoutId" | "homeDesign">;
type Point = readonly [number, number, number];

export function validCompanionHomeDesign(room: { environmentId: unknown; layoutId?: unknown; homeDesign?: unknown }): boolean {
  const design = room.homeDesign;
  if (design === undefined) return true;
  if (!design || typeof design !== "object" || Array.isArray(design)) return false;
  const value = design as Record<string, unknown>;
  return room.environmentId === "studio" && room.layoutId !== undefined && value.version === 1
    && [0, 1, 2, 3].includes(value.turns as number) && [0, .6, 1.2].includes(value.extension as number)
    && (value.extension === 0 || room.layoutId === "modern-kitchen-v01" || room.layoutId === "bathroom-v01");
}

export const companionHomeYaw = (room: Pick<PersonalRoom, "homeDesign">) => (room.homeDesign?.turns ?? 0) * Math.PI / 2;

/** Exact quarter turns avoid floating point seams in wall and obstacle bounds. */
export function companionHomePoint(room: Pick<PersonalRoom, "homeDesign">, point: Point): [number, number, number] {
  const [x, y, z] = point;
  switch (room.homeDesign?.turns) {
    case 1: return [z, y, -x];
    case 2: return [-x, y, -z];
    case 3: return [-z, y, x];
    default: return [x, y, z];
  }
}

export function companionHomeBounds<T extends { minX: number; maxX: number; minZ: number; maxZ: number }>(room: Pick<PersonalRoom, "homeDesign">, bounds: T): T {
  const a = companionHomePoint(room, [bounds.minX, 0, bounds.minZ]);
  const b = companionHomePoint(room, [bounds.maxX, 0, bounds.maxZ]);
  return { ...bounds, minX: Math.min(a[0], b[0]), maxX: Math.max(a[0], b[0]), minZ: Math.min(a[2], b[2]), maxZ: Math.max(a[2], b[2]) };
}

export function companionHomeBox<T extends Pick<RoomObstacle, "x" | "z" | "width" | "depth">>(room: Pick<PersonalRoom, "homeDesign">, box: T): T {
  const [x, , z] = companionHomePoint(room, [box.x, 0, box.z]);
  return { ...box, x, z, width: (room.homeDesign?.turns ?? 0) % 2 ? box.depth : box.width,
    depth: (room.homeDesign?.turns ?? 0) % 2 ? box.width : box.depth };
}

/** Only the empty front strip of the kitchen/bathroom grows. Furniture retains its scale. */
export function companionHomeShellZ(room: DesignedRoom, z: number): number {
  const extension = room.homeDesign?.extension ?? 0;
  return z > 1.9 ? z + (z - 1.9) / .7 * extension : z;
}

export function companionHomeLayout(room: DesignedRoom, base: RoomLayout): RoomLayout {
  if (!room.homeDesign) return base;
  const extension = room.homeDesign.extension, yaw = companionHomeYaw(room);
  const at = (point: { x: number; z: number }) => {
    const [x, , z] = companionHomePoint(room, [point.x, 0, point.z]); return { x, z };
  };
  const standing = [...base.standingCandidates];
  if (extension) for (const x of [-1.6, -.8, 0, .8, 1.6]) standing.push({ x, z: 2 + extension / 2, yaw: Math.PI });
  return {
    ...base, id: `${base.id}:home-v1:${room.homeDesign.turns}:${extension}`,
    bounds: companionHomeBounds(room, { ...base.bounds, maxZ: base.bounds.maxZ + extension }),
    obstacles: base.obstacles.map(box => companionHomeBox(room, box)),
    floorPatches: base.floorPatches?.map(box => companionHomeBox(room, box)),
    standingCandidates: standing.map(point => ({ ...point, ...at(point), yaw: point.yaw + yaw })),
    surfaces: base.surfaces.map(surface => {
      const expanded = surface.id === "floor" ? { ...surface, z: surface.z + extension / 2, depth: surface.depth + extension } : surface;
      return { ...companionHomeBox(room, expanded), anchors: surface.anchors.map(at),
        ...(surface.clearance ? { clearance: companionHomeBox(room, surface.clearance) } : {}) };
    }),
    walls: base.walls?.map(wall => ({ ...wall,
      ...at(wall.id === "wall-left" && extension ? { x: wall.x, z: wall.z + extension / 2 } : wall),
      width: wall.width + (wall.id === "wall-left" ? extension : 0), yaw: wall.yaw + yaw,
    })),
  };
}
