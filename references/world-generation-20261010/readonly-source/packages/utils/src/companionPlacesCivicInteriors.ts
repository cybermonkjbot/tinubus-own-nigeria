import design from "../../assets/venues/civic-interiors-v1.json";
import type { ScenePoint } from "./companionNavigation";
import type { CompanionPlaceFixture, CompanionPlaceGeneration, CompanionPlaceLayout } from "./companionPlaces";

/** Both floors share exterior scale, with independent level navigation. */
export function generateCivicInteriorLayout(input: CompanionPlaceGeneration): CompanionPlaceLayout {
  const floor = design.floors.find(value => value.id === input.placeId);
  if (input.kind !== "civic-interior" || !floor) throw new Error("Choose an authored Civic Street floor.");
  // Each admitted floor is a level room instance. Keep source assembly height
  // in Blender; convert runtime poses to the established room-local floor.
  const point = (value: readonly number[]): ScenePoint => [value[0], value[1] - floor.floorY, value[2]];
  const id = `${input.placeId}:lagos-places-v1:${design.id}:${encodeURIComponent(input.seed)}`;
  const fixtures: CompanionPlaceFixture[] = floor.footprints.map(value => ({
    id: value.id, kind: "cover", material: "stone", yaw: 0,
    point: [value.x, 0, value.z], size: [value.width, value.height, value.depth],
    collision: { id: value.id, minX: value.x - value.width / 2, maxX: value.x + value.width / 2,
      minZ: value.z - value.depth / 2, maxZ: value.z + value.depth / 2, height: value.height },
  }));
  return {
    id, placeId: input.placeId, kind: input.kind, label: floor.label, seed: input.seed,
    generatorVersion: "lagos-places-v1", assetVersion: design.id,
    navigation: { revision: `${id}:${design.id}`, floorY: 0, radius: .24, gridSize: .25,
      bounds: { ...floor.bounds }, obstacles: fixtures.flatMap(value => value.collision ? [value.collision] : []) },
    fixtures, assets: [],
    entrances: [{ id: "world-entry", label: "Civic Street", point: point(floor.entrance.point),
      outside: point(floor.entrance.outside), width: floor.entrance.width, role: "world", access: "place-membership" }],
    spawns: floor.spawns.map(value => ({ point: point(value), yaw: Math.PI })),
    recoverySpawns: [], detentionSpawns: [], services: [], npcs: [], zones: [], homeRooms: [],
    cameras: { wide: { eye: point(floor.cameras.wide.eye), target: point(floor.cameras.wide.target) },
      close: { eye: point(floor.cameras.close.eye), target: point(floor.cameras.close.target) } },
    theme: { ...floor.theme },
    qualityGaps: ["The interiors are original designs inferred from the exterior, not surveyed interiors. Native/web visual review remains pending.",
      "Separate Civic Street entrances admit each floor. Direct floor-to-floor and pool doorways need the existing world transition owner; furnishings do not create food, paid bar or care services."],
  };
}
