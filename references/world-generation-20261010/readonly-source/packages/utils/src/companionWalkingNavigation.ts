import { isCompanionCity } from "./companionCity";
import type { NavigationObstacle, RoomNavigation } from "./companionNavigation";
import { townSquareNavigation, townSquareObstacles } from "./companionTownSquare";
import { companionStreetCrashFootprint } from "./companionStreetCrashes";
import type { CompanionStreetCrashTrafficView } from "./companionStreetCrashTraffic";

// These are movement reservations in the retained art, not fixed masonry.
// Leave immutable saved layouts intact. Apply the same pedestrian policy in
// the renderer, prediction and authority; actual bodies supply live blockers.
const reservations = townSquareObstacles.filter(box => box.id.startsWith("pothole-")
  || ["cap-npc-sweep", "shirt-npc", "vest-npc-sweep"].includes(box.id)
  || ["danfo", "taxi", "parked car"].includes(box.id)
    && (box.minX + box.maxX) / 2 >= -14 && (box.minX + box.maxX) / 2 <= -6
    && (box.minZ + box.maxZ) / 2 >= -46 && (box.minZ + box.maxZ) / 2 <= 29);
const sameBox = (a: NavigationObstacle, b: NavigationObstacle) => !a.yaw && !a.polygon
  && Math.abs(a.minX - b.minX) < 1e-6 && Math.abs(a.maxX - b.maxX) < 1e-6
  && Math.abs(a.minZ - b.minZ) < 1e-6 && Math.abs(a.maxZ - b.maxZ) < 1e-6;
const fixed = new WeakMap<RoomNavigation, RoomNavigation>();

export function companionWalkingNavigation(navigation: RoomNavigation,
  traffic?: CompanionStreetCrashTrafficView | null): RoomNavigation {
  if (!isCompanionCity(navigation) && navigation.revision !== townSquareNavigation.revision) return navigation;
  let base = fixed.get(navigation);
  if (!base) {
    base = { ...navigation, obstacles: navigation.obstacles.filter(box => !reservations.some(reserved => sameBox(box, reserved))) };
    fixed.set(navigation, base);
  }
  if (!traffic || traffic.scope.layoutRevision !== navigation.revision) return base;
  return { ...base, obstacles: [...base.obstacles, ...traffic.vehicles.map(vehicle => companionStreetCrashFootprint(vehicle.pose))] };
}

export function companionWalkingBody(body: { x: number; z: number; yaw: number; halfWidth: number; halfLength: number }): NavigationObstacle {
  return { minX: body.x - body.halfWidth, maxX: body.x + body.halfWidth,
    minZ: body.z - body.halfLength, maxZ: body.z + body.halfLength, yaw: body.yaw };
}

/** Floor picking may hit the shallow road bowl instead of the flat route plane. */
export function companionWalkablePothole(x: number, y: number, z: number): boolean {
  return y >= -.16 && y <= .02 && townSquareObstacles.some(box => box.id.startsWith("pothole-")
    && x >= box.minX && x <= box.maxX && z >= box.minZ && z <= box.maxZ);
}
