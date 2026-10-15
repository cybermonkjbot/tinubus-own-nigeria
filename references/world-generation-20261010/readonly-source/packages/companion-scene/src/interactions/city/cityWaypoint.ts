import type { CompanionWorldSnapshot } from "../../../../../packages/utils/src/companionWorld";
import { isCompanionCity } from "../../../../../packages/utils/src/companionCity";

/** A local map pin, never a movement command or a shared destination. */
export type CityWaypoint = { id: string; label: string; x: number; z: number };

export function cityWaypointScope(world?: CompanionWorldSnapshot) {
  const player = world?.player, instance = world?.instance;
  return player && instance && player.instanceId === instance.id && player.status === "present"
    && player.health > 0 && isCompanionCity(instance.navigation)
    ? `${player.userId}:${player.characterId}:${player.gameRoomId}:${instance.id}:${instance.navigation.revision}` : null;
}

export function cityWaypointDistance(distance: number) {
  return distance < 1000 ? `${Math.round(distance)} m` : `${(distance / 1000).toFixed(1)} km`;
}

export function cityWaypointReached(distance: number) { return distance <= 4; }
