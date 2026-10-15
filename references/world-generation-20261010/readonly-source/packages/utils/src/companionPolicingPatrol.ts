import { routeSegmentClear, type RoomNavigation, type ScenePoint } from "./companionNavigation";
import type { CompanionWorldInstance } from "./companionWorld";
import type { CompanionEnforcementAgency } from "./companionPolicing";

export type CompanionPolicePatrol = { id: string; label: string; agency?: CompanionEnforcementAgency; route: readonly ScenePoint[]; speed: number; offsetMs: number; yaw?: number };
/** The existing road checkpoint. Gunfire can dispatch its officers from here. */
export function companionPolicePatrols(instance: CompanionWorldInstance): CompanionPolicePatrol[] {
  if (instance.placeId !== "town-square") return [];
  const nav = instance.navigation;
  const point: ScenePoint = [-6.5, nav.floorY, -10];
  if (!routeSegmentClear(nav, point, point)) return [];
  return [{ id: "town-square:police-beat", label: "Police checkpoint officer", agency: "police",
    route: [point], speed: 1.2, offsetMs: 0, yaw: -Math.PI / 2 }];
}
/** Shared start time, not join time. No local sampling can authorize an arrest. */
export function sampleCompanionPolicePatrol(patrol: CompanionPolicePatrol, sharedStartedAt: number, now: number, navigation: RoomNavigation): ScenePoint | null {
  if (!Number.isFinite(now) || !Number.isFinite(sharedStartedAt) || now < sharedStartedAt || patrol.route.length < 1 || patrol.route.length > 65) return null;
  if (patrol.route.length === 1) return routeSegmentClear(navigation, patrol.route[0], patrol.route[0]) ? patrol.route[0] : null;
  const lengths = patrol.route.slice(1).map((point, index) => Math.hypot(point[0] - patrol.route[index][0], point[2] - patrol.route[index][2]));
  const total = lengths.reduce((sum, length) => sum + length, 0);
  if (!total || !Number.isFinite(patrol.speed) || patrol.speed <= 0) return null;
  let remaining = ((now - sharedStartedAt + patrol.offsetMs) / 1000 * patrol.speed) % total;
  for (let index = 0; index < lengths.length; index++) {
    if (remaining > lengths[index]) { remaining -= lengths[index]; continue; }
    const from = patrol.route[index], to = patrol.route[index + 1], ratio = lengths[index] ? remaining / lengths[index] : 0;
    const point: ScenePoint = [from[0] + (to[0] - from[0]) * ratio, navigation.floorY, from[2] + (to[2] - from[2]) * ratio];
    return routeSegmentClear(navigation, point, point) ? point : null;
  }
  return patrol.route[0];
}
