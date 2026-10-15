import design from "../../assets/venues/pool-courtyard-v1.json";
import type { ScenePoint } from "./companionNavigation";
import type { CompanionPlaceFixture, CompanionPlaceGeneration, CompanionPlaceLayout } from "./companionPlaces";

/** The photo-led arrangement stays canonical. Seeds identify instances, not a
 * mirrored floor plan. Geometry and collision share this authored source. */
export function generatePoolCourtyardLayout(input: CompanionPlaceGeneration): CompanionPlaceLayout {
  if (input.kind !== "pool-courtyard") throw new Error("Use the pool courtyard place kind.");
  const point = (value: readonly number[]): ScenePoint => [value[0], value[1], value[2]];
  const id = `${input.placeId}:lagos-places-v1:${design.id}:${encodeURIComponent(input.seed)}`;
  const fixtures: CompanionPlaceFixture[] = design.footprints.map(value => ({
    id: value.id, kind: "cover", material: "stone", yaw: 0,
    point: [value.x, 0, value.z], size: [value.width, value.height, value.depth],
    collision: { id: value.id, minX: value.x - value.width / 2, maxX: value.x + value.width / 2,
      minZ: value.z - value.depth / 2, maxZ: value.z + value.depth / 2, height: value.height },
  }));
  return {
    id, placeId: input.placeId, kind: input.kind, label: design.label, seed: input.seed,
    generatorVersion: "lagos-places-v1", assetVersion: design.id,
    navigation: { revision: `${id}:${design.id}`, floorY: 0, radius: .24, gridSize: .3,
      bounds: { minX: -design.size[0] / 2, maxX: design.size[0] / 2, minZ: -design.size[2] / 2, maxZ: design.size[2] / 2 },
      obstacles: fixtures.flatMap(value => value.collision ? [value.collision] : []) },
    fixtures, assets: [],
    entrances: [{ id: "world-entry", label: "Civic Street", point: point(design.entrance.point),
      outside: point(design.entrance.outside), width: design.entrance.width, role: "world", access: "place-membership" }],
    spawns: design.spawns.map(value => ({ point: point(value), yaw: Math.PI })),
    recoverySpawns: [], detentionSpawns: [], services: [], npcs: [], zones: [], homeRooms: [],
    cameras: { wide: { eye: point(design.cameras.wide.eye), target: point(design.cameras.wide.target) },
      close: { eye: point(design.cameras.close.eye), target: point(design.cameras.close.target) } },
    theme: { ...design.theme },
    qualityGaps: ["Photo-led proportions are inferred, not surveyed. Native and web visual inspection remains pending.",
      "The pool and upper terrace are scenery. Walking stays on the ground-level deck; no swimming, stair traversal or bar service is attached."],
  };
}
