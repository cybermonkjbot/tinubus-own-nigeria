import type { CompanionLibrary, PersonalRoom, CompanionFurniture } from "./companion";
import type { CompanionHomeDesign } from "./companionHomeDesign";
import { roomLayoutFor } from "./companionRoomLayouts";
import { placementError, surfacePlacementBounds } from "./companionPlacement";
import { companionStations } from "./companionLife";
import { companionHomeDoor } from "./companionHomeDoor";
import { navigableStandingPoints, planRoomRoute, roomNavigationFor, routeSegmentClear } from "./companionNavigation";

export type CompanionHomeProgress = { seed: number; successfulReturns: number; baseRoomSlots: number };
export type CompanionHomeProgressView = { seed: number; successfulReturns: number; roomCapacity: number; nextRoomAt: number | null };
export const companionHomeMilestones = [2, 5, 10, 18, 28, 40, 55, 75, 100, 130, 165] as const;

export function companionHomeSeed(value: string): number {
  let seed = 2166136261;
  for (let i = 0; i < value.length; i++) seed = Math.imul(seed ^ value.charCodeAt(i), 16777619);
  seed = Math.imul(seed ^ seed >>> 16, 0x85ebca6b);
  return (seed ^ seed >>> 13) >>> 0;
}

export function companionHomeProgressView(progress: CompanionHomeProgress): CompanionHomeProgressView {
  const earned = companionHomeMilestones.filter(target => progress.successfulReturns >= target).length;
  const roomCapacity = Math.min(12, progress.baseRoomSlots + earned);
  return { seed: progress.seed, successfulReturns: progress.successfulReturns, roomCapacity,
    nextRoomAt: roomCapacity === 12 ? null : companionHomeMilestones[earned] ?? null };
}

export function validCompanionHomeProgressView(value: unknown): value is CompanionHomeProgressView {
  if (!value || typeof value !== "object") return false;
  const v = value as CompanionHomeProgressView;
  return Number.isSafeInteger(v.seed) && v.seed >= 0 && v.seed <= 0xffffffff
    && Number.isSafeInteger(v.successfulReturns) && v.successfulReturns >= 0
    && Number.isSafeInteger(v.roomCapacity) && v.roomCapacity >= 1 && v.roomCapacity <= 12
    && (v.nextRoomAt === null || Number.isSafeInteger(v.nextRoomAt) && v.nextRoomAt > v.successfulReturns);
}

export function companionHomeProgressLabel(progress: CompanionHomeProgressView): string {
  return progress.nextRoomAt === null ? "All room slots earned"
    : `${progress.successfulReturns}/${progress.nextRoomAt} returns with earned haul for the next room`;
}

export function companionHomeDesignFor(seed: number, id: string, layoutId: PersonalRoom["layoutId"]): CompanionHomeDesign | undefined {
  if (!layoutId) return undefined;
  const value = companionHomeSeed(`${seed}:${id}:${layoutId}`);
  return { version: 1, turns: ([0, 1, 2, 3] as const)[value % 4],
    extension: layoutId === "modern-kitchen-v01" || layoutId === "bathroom-v01" ? ([0, .6, 1.2] as const)[(value >>> 4) % 3] : 0 };
}

/** Generates once per saved room ID. Reopening a home never regenerates its furnishings. */
export function createCompanionHomeRoom(seed: number, id: string, name: string, layoutId: NonNullable<PersonalRoom["layoutId"]>): PersonalRoom {
  let randomState = companionHomeSeed(`${seed}:${id}:furnishings`);
  const random = () => { randomState = (Math.imul(randomState, 1664525) + 1013904223) >>> 0; return randomState / 0x100000000; };
  const room: PersonalRoom = { id, name, environmentId: "studio", layoutId, homeDesign: companionHomeDesignFor(seed, id, layoutId),
    palette: (["rose", "sage", "midnight"] as const)[Math.floor(random() * 3)], light: "daylight", sharing: "private", items: [] };
  const layout = roomLayoutFor(room);
  const stations = companionStations(room);
  const baseline = roomNavigationFor(room), standing = navigableStandingPoints(room);
  if (!standing.length) return room;
  const start = [standing[0].x, baseline.floorY, standing[0].z] as const;
  // Reserve the authored care and exit routes once, then use segment checks for
  // candidate decor instead of rerunning the pathfinder for every placement.
  const routes = [...stations.map(station => planRoomRoute(baseline, start, station.approach)),
    planRoomRoute(baseline, start, companionHomeDoor(room).point)];
  if (routes.some(route => !route)) return room;
  // Prefer existing measured support bays; floor props must preserve every care route and the exit.
  for (let attempt = 0; attempt < 32 && room.items.length < 3; attempt++) {
    const kind: CompanionFurniture["kind"] = random() < .5 ? "plant" : "lamp";
    const surface = layout.surfaces[Math.floor(random() * layout.surfaces.length)];
    if (!surface.kinds.includes(kind)) continue;
    const bounds = surfacePlacementBounds(kind, surface, layout);
    if (bounds.minX > bounds.maxX || bounds.minZ > bounds.maxZ) continue;
    const item: CompanionFurniture = { id: `home-decor-${room.items.length}`, kind, surface: surface.id,
      x: bounds.minX + random() * (bounds.maxX - bounds.minX), z: bounds.minZ + random() * (bounds.maxZ - bounds.minZ),
      rotation: random() * Math.PI * 2, lit: true };
    const candidate = { ...room, items: [...room.items, item] };
    if (placementError(candidate.items, layout)) continue;
    const navigation = roomNavigationFor(candidate);
    if (routes.some(route => !route?.every((point, index) => routeSegmentClear(navigation, index ? route[index - 1] : start, point)))) continue;
    room.items.push(item);
  }
  return room;
}

export function companionStarterRooms(seed: number): PersonalRoom[] {
  const rooms = [createCompanionHomeRoom(seed, "home", "Bedroom", "pink-bedroom-v02"),
    createCompanionHomeRoom(seed, "home-bathroom", "Bathroom", "bathroom-v01")];
  if (seed % 3 === 0) rooms.push(createCompanionHomeRoom(seed, "home-living", "Living room", "pat-studio-v01"));
  return rooms;
}

/** Legacy libraries retain every room and their original expansion baseline. */
export function initialCompanionHomeProgress(seed: number, library: CompanionLibrary): CompanionHomeProgress {
  return { seed, successfulReturns: 0, baseRoomSlots: library.rooms.length };
}
